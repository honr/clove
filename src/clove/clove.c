#include "clove-utils.h"
#include <getopt.h>
#include <dirent.h>
#include <syslog.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

/* TODO: clean up broker_message_length and other hard coded string
   lengthes.

   TODO: daemon mode should actually fork and become daemon. use
   syslog for logs.
   */

const char* clove_version = "0.0.2";

/* defined in clove-utils.h */
extern char* RTPREFIX;
extern char* RUNPATH;

int usage (int n)
  { printf ("usage:\n"
            " clove [--force-wipe|--extraenv <extra env>|--runpath <run path>] <rest args>\n"
            " <rest args> can be the following forms\n"
            " clove --remote <command issued to the clove daemon>\n"
            " clove --help\n"
            " clove --version\n"
            " clove --list # list the clove socket paths\n"
            " clove --client <clove service> [<client commands>]\n"
            "       # issue commands to the given service, possibly starting it.\n");
    exit (n); }

int main (int argc, char* argv[], char** envp)
  { static struct option long_options[] =
      {{"list",       0, 0, 'l'},
       {"extraenv",   1, 0, 'x'},
       {"runpath",    1, 0, 'p'},
       {"force-wipe", 0, 0, 'w'},
       {"remote",     1, 0, 'r'},
       {"client",     1, 0, 'c'},
       {"version",    0, 0, 'v'},
       {"help",       0, 0, 'h'},
       {0, 0, 0, 0}};

    int c;
    int option_index = 0;
    struct str_list* extraenvs = NULL;
    RTPREFIX = "~/.local";
    /* RUNPATH = str_concat ("/tmp/clove-", getenv ("USER")); */
    RUNPATH = malloc (128);
    sprintf (RUNPATH, "/tmp/clove-%d", geteuid ());
    struct service broker;
    struct str_list* remote_args = NULL;

    int force_wipe_sockpaths = false;
    int mode = 0;
    int opt_keep_processing = true;
    if (argc <= 1)
      { mode = 'i';
        opt_keep_processing = false; }
    else if ((argv[1][0] != '-') && (argv[1][0] != ':') && (argv[1][0] != '+'))
      // jump right into client mode if no option is provided.
      { opt_keep_processing = false; }

    while (opt_keep_processing &&
           ((c = getopt_long (argc, argv, "x:p:wlr:c:hv",
                              long_options, &option_index)) != -1))
      { switch (c)
          { case 'x':
              extraenvs = str_list_cons (optarg, extraenvs);
              break;
	    case 'p':
              RUNPATH = optarg;
              break;
            case 'w':
              force_wipe_sockpaths = true;
              break;
            case 'l':
              mode = c;
              // info = optarg;
              break;
            case 'c': // send a message to the remote service, and exit.
            case 'r': // send a message to the broker, and exit.
            case 'i': // start an interactive session (repl) with the service.
              mode = c;
              remote_args = str_list_from_vec (argv, optind - 1, argc - 1);
              opt_keep_processing = false;
              break;
            case 'h':
              usage (0);
            case 'v':
              printf ("Clove version: %s\n", clove_version);
              exit (0);
            default:
              usage (1); }}

    broker = service_init ("broker");

    if (mode == 0)
      { mode = 'c';

        FILE *file_in;
        if ((file_in = fopen (argv[1], "r")) == NULL)
          { perror ("fopen #! file");
            fprintf (stderr, "supplied file: `%s'\n", argv[1]);
            exit (1); }
        char *argbuf = malloc (4096); // TODO: fix hardcoded size
        char *argline;
        if (! fgets (argbuf, 4096, file_in)) // discard the first line. TODO: fix hardcoded size
	  { exit (1); }
        if (! fgets (argbuf, 4096, file_in)) // TODO: fix hardcoded size
	  { exit (1); }
        if ((argline = strstr (argbuf, "-*- |")))
          { argline += 5; } // skip over it
        else if ((argline = strstr (argbuf, "|")))
          { argline += 1; } // skip over it
        else
          { fprintf (stderr,
                     "The second line in file:\n   `%s'\n"
                     "should indicate the service using `|'.\nFor exmaple:\n"
                     "#!/usr/bin/env clove\n;; | clojure\n\n", argv[1]);
            exit (1); }
        argline += strspn (argline, " \t"); // skip past whitespace
        argline = strsep (&argline, "\n\r"); // remove trailing newline
        struct str_list* remote_args_tmp = str_split_qe (argline, 4096); // TODO: fix hardcoded size
        str_list_nreverse (&remote_args_tmp);
        str_list_nconcat (&remote_args, remote_args_tmp);
        str_list_nconcat (&remote_args, str_list_from_vec (argv, 1, argc - 1));

        fclose (file_in); }

    switch (mode)
      { case 'l':
          { char* rgv[] = { "ls", "-lA", expand_file_name (RUNPATH), NULL };
            execve ("/bin/ls", rgv, NULL); }
          break;

        case 'r':
          { char buf[broker_message_length];
            struct str_list* cur;
            for (cur = remote_args;
                 cur;
                 cur = cur->next)
              { memccpy (buf + 1, cur->str, 0, broker_message_length - 2);
                buf[0] = 0; // means remote command, not a service.
                broker.sock = sock_addr_connect (SOCK_STREAM, broker.sockpath);
                write (broker.sock, buf, strlen (cur->str) + 2); // TODO: check return val.
                if (read (broker.sock, buf, broker_message_length - 1) > 0)
                  { buf[broker_message_length - 1] = 0;
                    printf ("%s\n", buf); }
                close (broker.sock); }}
          break;

        case 'c':
          { broker.sock = sock_addr_connect (SOCK_STREAM, broker.sockpath);

            char buf[broker_message_length];
            struct service srv;

            { int ret;
              int message_length;
              char* remote_service = str_list_pop (&remote_args);

              buf[broker_message_length - 1] = 0;
              strcpy (buf, remote_service);
              message_length = strlen (buf) + 1;
              srv = service_init (remote_service);

              ret = write (broker.sock, buf, message_length);
              ret = read (broker.sock, buf, broker_message_length);
              if (ret <= 0)
                { fprintf (stderr, "Read: %d bytes. Probably OK. Terminating.\n",
                           ret);
                  exit (0); }

              close (broker.sock); }

            /* server unix path should be obtained now. */
            memccpy (srv.sockpath, buf, 0, 127); // TODO: fix hardcoded size
            srv.sockpath[127] = 0; // TODO: fix hardcoded size
            srv.sock = sock_addr_connect (SOCK_STREAM, srv.sockpath);

            struct remote_fds iofds =
              { .in     = 0,
                .out    = 1,
                .err    = 2 };

            if (unix_send_fds (srv.sock, iofds) < 0)
              { perror ("unix_send_fds"); }

            char* buf_cur = buf;
            char* buf_lim = buf + broker_message_length;
            str_list_to_pack (&buf_cur, buf_lim, remote_args);

            { int envs_size = 0;
              char** envs;
              for (envs = envp; (*envs); envs ++)
                { envs_size += strlen (*envs);
                  buf_cur = strlcpy_p (buf_cur, *envs, buf_lim); }
              buf_cur = strlcpy_p (buf_cur, NULL, buf_lim);

              if (! buf_cur)
                { printf ("environment too long (%d bytes)\n", envs_size);
                  exit (3); }}

            // printf ("buf_cur - buf %ld\n", buf_cur - buf);
            write (srv.sock, buf, buf_cur - buf); // TODO: check return val.

            // TODO: also, communicate with the server out-of-band, for
            //       instance, ask for completion.

            read (srv.sock, buf, broker_message_length); // TODO: check return val.
	  }
          break;

        case 'i':
          { fprintf (stderr,
                     "Interactive mode\nNot implemented yet. Sorry.\n"); }
          break;
        default:
          { exit (1); }}
    return 0; }
