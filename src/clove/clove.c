#include "clove-utils.h"
#include <getopt.h>
#include <dirent.h>
#include <syslog.h>
#include <unistd.h>

/* TODO: clean up broker_message_length and other hard coded string
   lengthes.
   
   TODO: daemon mode should actually fork and become daemon. use
   syslog for logs.
   */

const char* clove_version = "0.0.1";

int usage (int n)
  { printf ("usage:\n"
	    " clove [--wipe-broker|--wipe-services|--extraenv <extra env>] <rest args>\n"
	    " <rest args> can be the following forms"
	    " clove --remote <command issued to the clove daemon>\n"
	    " clove --help\n"
	    " clove --version\n"
	    " clove --list # list the clove socket paths\n"
	    " clove --daemon # start a clove daemon\n"
	    " clove --client <clove service> [<client commands>]\n"
	    "       # issue commands to the given service, possibly starting it.\n");
    exit (n); }

int running = true;

void sighup_handler (int signum)
  { running = 0; }
  
int main (int argc, char* argv[], char** envp)
  { static struct option long_options[] =
      {{"daemon",  0, 0, 'd'},
       {"list",  0, 0, 'l'},
       {"extraenv",  1, 0, 'x'},
       {"force-wipe", 0, 0, 'w'},
       {"remote", 1, 0, 'r'},
       {"client", 1, 0, 'c'},
       {"version", 0, 0, 'v'},
       {"help", 0, 0, 'h'},
       {0, 0, 0, 0}};

    int c;
    int option_index = 0;
    struct str_list* extraenvs = NULL;
    struct service broker = service_init ("broker");
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
	   ((c = getopt_long (argc, argv, "x:wdlr:c:hv",
			      long_options, &option_index)) != -1))
      { switch (c)
	  { case 'x':
	      extraenvs = str_list_cons (optarg, extraenvs);
	      break;
	    case 'w':
	      force_wipe_sockpaths = true;
	      break;
	    case 'd':
	      mode = c;
	      break;
	    case 'l':
	      mode = c;
	      // info = optarg;
	      break;
	    case 'c': // send a message to the remote service, and exit.
	    case 'r': // send a message to the broker, and exit.
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

    if (mode == 0)
      { mode = 'c';
	
	FILE *file_in;
	if ((file_in = fopen (argv[1], "r")) == NULL)
	  { perror ("fopen #! file");
	    fprintf (stderr, "supplied file: `%s'\n", argv[1]);
	    exit (1); }
	char *argbuf = malloc (4096); // TODO: fix hardcoded size
	char *argline;
	fgets (argbuf, 4096, file_in); // discard the first line. TODO: fix hardcoded size
	fgets (argbuf, 4096, file_in); // TODO: fix hardcoded size

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
	struct str_list* remote_args_tmp = str_split (argline, " \t");
	str_list_nreverse (&remote_args_tmp);
	str_list_nconcat (&remote_args, remote_args_tmp);
	str_list_nconcat (&remote_args, str_list_from_vec (argv, 1, argc - 1));
	
	fclose (file_in); }

    if (mode == 'd')
      { // try to communicate with the existing broker if
	// broker.sockpath exists.  If the broker responds, simply
	// print out its pid and quit.
	if ((!force_wipe_sockpaths) &&
	    (broker.sock = sock_addr_connect (SOCK_STREAM, broker.sockpath))
	    != -1)
	  { char buf[broker_message_length];
	    buf[0] = 0; // means remote command, not a service.
	    strcpy (buf + 1, "ping");
	    write (broker.sock, buf, strlen (buf + 1) + 2);
	    if (read (broker.sock, buf, broker_message_length - 1) > 0)
	      { buf[broker_message_length - 1] = 0;
		printf ("a broker exists with pid: %s\n", buf);
		close (broker.sock);
		exit (0); }
	    else { close (broker.sock); }}
	unlink (broker.sockpath); // maybe wipe dead socket path
	
	// wipe service socket paths.
	{ char* dp_fullpath = (char*) malloc (PATH_MAX);
	  strcpy (dp_fullpath, service_socket_path_dir ());
	  DIR* dirp;
	  if ((dirp = opendir (dp_fullpath)) != NULL)
	    { char* dp_fullpath_suf = dp_fullpath + strlen (dp_fullpath);
	      *dp_fullpath_suf = '/';
	      dp_fullpath_suf ++;
	      *dp_fullpath_suf = '\0';
	      struct dirent * dp = NULL;

	      while ((dp = readdir (dirp)) != NULL) // isn't readdir_r better?
		{ if (str_beginswith (dp->d_name, "clove-")) 
		    { strcpy (dp_fullpath_suf, dp->d_name);
		      if (! force_wipe_sockpaths) 
			{ int service_sock;
			  if ((service_sock = sock_addr_connect (SOCK_STREAM, dp_fullpath))
			      != -1) // a single connect and then close is used for probing.
			    { printf ("seems to be alive `%s'\n", dp_fullpath);
			      close (service_sock); }
			  else 
			    { printf ("does not response. removing `%s'\n", dp_fullpath);
			      unlink (dp_fullpath); }}
		      else
			{ printf ("removing `%s'\n", dp_fullpath);
			  unlink (dp_fullpath); }}}
	      free (dp_fullpath);
	      closedir (dirp); }}
	
	// read default service config
	char** default_envp = envp;
	default_envp = envp_dup_update_or_add
	  (default_envp,
	   parse_conf_file (broker.confpath)->envs);

	default_envp = envp_dup_update_or_add (default_envp, extraenvs);
	// TODO: wipe dead services in another thread.

	// create sockpath parent directory:
	makeancesdirs (broker.sockpath);
	
	broker.sock = sock_addr_bind (SOCK_STREAM, broker.sockpath, true);
	// or: SOCK_STREAM | SOCK_CLOEXEC;

	// int main_process_pgid = getpgid (0);
	
	sigaction_inst (SIGHUP, sighup_handler);

	// daemonize
	{ int temp_i = fork();
	  if (temp_i < 0) exit(1);   /* fork error */
	  if (temp_i > 0) exit(0); } /* parent exits */
	
	if (setsid () == -1)
	  { perror ("setsid");
	    exit (1); }
	
	openlog ("Clove", LOG_PID, LOG_DAEMON);
	char* current_hostname = malloc (4096); // TODO: fix hardcoded size
	gethostname (current_hostname, 4096); // TODO: fix hardcoded size
	syslog (LOG_INFO, "Connection from host %s", current_hostname);
	syslog (LOG_ALERT, "Init complete.");
	// TOOD: log the rest of stderr, too.
	
	int main_process_pid = getpid ();

	while (running)
	  { int sock_a;
	    if ((sock_a = accept (broker.sock, NULL, NULL)) == -1)
	      { perror ("accept");
		exit (2); }
	    int client_handler_pid;
	    if ((client_handler_pid = fork ()) == 0)
	      { close (broker.sock);
		close (0);

		char buf[broker_message_length];
		read (sock_a, buf, broker_message_length);
		if (buf[0] == 0) // remote command
		  { char* remote_command = strndup (buf + 1, 222); // TODO: fix hardcoded size
		    if      (strcmp (remote_command, "quit") == 0)
		      { kill (main_process_pid, SIGHUP); }
		    else if (strcmp (remote_command, "stats") == 0)
		      { printf ("I should show the number of current "
				"subprocesses.\nNot implemented yet.\n"); }
		    else if (strcmp (remote_command, "ping") == 0)
		      { char* reply = malloc (128); // TODO: fix hardcoded size
			sprintf (reply, "%d", main_process_pid);
			write (sock_a, reply, strlen (reply)); }}
		else // service
		  { char* service_name = strndup (buf, 222); // TODO: fix hardcoded size

		    /*  TODO: see what happens to process group when the
			parent exits.  At the beginning read about former
			processes and a killer for each to the specific
			signal handler. Add the option -Q to kill all
			current and former children. */

		    struct service srv = service_init (service_name);

		    // TODO: Check if service listens to its socket.  Find a
		    // better way to check if the service is alive.
		    if (access (srv.sockpath, F_OK) != 0)
		      { service_call (srv, default_envp); }

		    write (sock_a, srv.sockpath, strlen (srv.sockpath)); }
		close (sock_a);
		exit (0); }
	    int status;
	    waitpid (client_handler_pid, &status, 0);
	    close (sock_a); }
	close (broker.sock);
	unlink (broker.sockpath);
	closelog(); }
    else if (mode == 'l')
      { char* rgv[] = { "ls", "-lA", service_socket_path_dir (), NULL };
	execve ("/bin/ls", rgv, NULL); }
    else if (mode == 'r')
      { char buf[broker_message_length];
	struct str_list* cur;
	for (cur = remote_args;
	     cur;
	     cur = cur->next)
	  { strncpy (buf + 1, cur->str, broker_message_length - 2);
	    buf[0] = 0; // means remote command, not a service.
	    broker.sock = sock_addr_connect (SOCK_STREAM, broker.sockpath);
	    write (broker.sock, buf, strlen (cur->str) + 2);
	    if (read (broker.sock, buf, broker_message_length - 1) > 0)
	      { buf[broker_message_length - 1] = 0;
		printf ("%s\n", buf); }
	    close (broker.sock); }}
    else if (mode == 'c')
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
	strncpy (srv.sockpath, buf, 127); // TODO: fix hardcoded size
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
	{ char* cur_arg;
	  for (cur_arg = str_list_pop (&remote_args);
	       cur_arg;
	       cur_arg = str_list_pop (&remote_args))
	    { buf_cur = strlcpy_p (buf_cur, cur_arg, buf_lim); }
	  buf_cur = strlcpy_p (buf_cur, NULL, buf_lim); }
	
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
	write (srv.sock, buf, buf_cur - buf);

	// TODO: also, communicate with the server out-of-band, for
	//       instance, ask for completion.

	read (srv.sock, buf, broker_message_length); }
    else if (mode == 'i')
      { fprintf (stderr, "Interactive mode\nNot implemented yet. Sorry.\n"); }
    else
      { exit (1); }
    return 0; }
