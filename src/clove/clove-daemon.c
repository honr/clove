#include "clove-utils.h"
#include <getopt.h>
#include <dirent.h>
#include <syslog.h>
#include <unistd.h>

/* TODO: clean up BROKER_MESSAGE_LENGTH and other hard coded string
   lengthes.

   TODO: daemon mode should actually fork and become daemon. use
   syslog for logs.
*/

const char *clove_version = "0.0.2";

/* defined in clove-utils.h */
extern char *RTPREFIX;
extern char *RUNPATH;

int
usage (int n)
{
  printf ("usage:\n"
	  " clove-daemon [--force-wipe] [--extraenv <extra env>] [--runpath <run path>] [--foreground]\n"
	  " clove-daemon --help\n"
	  " clove-daemon --version\n");
  exit (n);
}

int running = true;

void
sighup_handler (int signum)
{
  switch (signum)
    {
    case SIGKILL:
    case SIGHUP:
    case SIGINT:
      running = 0;
    }
}

int
main (int argc, char *argv[], char **envp)
{
  static struct option long_options[] = {
    {"foreground", 0, 0, 'D'},
    {"extraenv", 1, 0, 'x'},
    {"runpath", 1, 0, 'p'},
    {"force-wipe", 0, 0, 'w'},
    {"version", 0, 0, 'v'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
  };

  int c;
  int option_index = 0;
  struct str_list *extraenvs = NULL;
  RTPREFIX = "~/.local";
  /* RUNPATH = str_concat ("/tmp/clove-", getenv ("USER")); */
  RUNPATH = malloc (128);
  sprintf (RUNPATH, "/tmp/clove-%d", geteuid ());
  struct service broker;
  int do_not_fork = false;
  int force_wipe_sockpaths = false;

  while (((c = getopt_long (argc, argv, "x:p:wDhv",
			    long_options, &option_index)) != -1))
    {
      switch (c)
	{
	case 'x':
	  extraenvs = str_list_cons (optarg, extraenvs);
	  break;
	case 'p':
	  RUNPATH = optarg;
	  break;
	case 'w':
	  force_wipe_sockpaths = true;
	  break;
	case 'D':
	  do_not_fork = true;
	  break;
	case 'h':
	  usage (0);
	case 'v':
	  printf ("Clove version: %s\n", clove_version);
	  exit (0);
	default:
	  usage (1);
	}
    }

  broker = service_init ("broker");

  // try to communicate with the existing broker if
  // broker.sockpath exists.  If the broker responds, simply
  // print out its pid and quit.
  if ((!force_wipe_sockpaths) &&
      (broker.sock = sock_addr_connect (SOCK_STREAM, broker.sockpath)) != -1)
    {
      char buf[BROKER_MESSAGE_LENGTH];
      buf[0] = 0;		// means remote command, not a service.
      strcpy (buf + 1, "ping");
      if (write (broker.sock, buf, strlen (buf + 1) + 2) < 0)
	{
	  perror ("(write) Could not talk to the broker");
	  exit (1);
	}
      if (read (broker.sock, buf, BROKER_MESSAGE_LENGTH - 1) > 0)
	{
	  buf[BROKER_MESSAGE_LENGTH - 1] = 0;
	  printf ("a broker exists with pid: %s\n", buf);
	  close (broker.sock);
	  exit (0);
	}
      else
	{
	  close (broker.sock);
	}
    }
  unlink (broker.sockpath);	// maybe wipe dead socket path

  // wipe service socket paths.
  {
    char *dp_fullpath = (char *) malloc (PATH_MAX);
    strcpy (dp_fullpath, expand_file_name (RUNPATH));
    DIR *dirp;
    if ((dirp = opendir (dp_fullpath)) != NULL)
      {
	char *dp_fullpath_suf = dp_fullpath + strlen (dp_fullpath);
	*dp_fullpath_suf = '/';
	dp_fullpath_suf++;
	*dp_fullpath_suf = '\0';
	struct dirent *dp = NULL;

	while ((dp = readdir (dirp)) != NULL)
	  // isn't readdir_r better?
	  {
	    if (str_beginswith (dp->d_name, "clove-"))
	      {
		strcpy (dp_fullpath_suf, dp->d_name);
		if (!force_wipe_sockpaths)
		  {
		    int service_sock;
		    if ((service_sock = sock_addr_connect (SOCK_STREAM, dp_fullpath)) != -1)	// a single connect and then close is used for probing.
		      {
			printf ("seems to be alive `%s'\n", dp_fullpath);
			close (service_sock);
		      }
		    else
		      {
			printf ("does not response. removing `%s'\n",
				dp_fullpath);
			unlink (dp_fullpath);
		      }
		  }
		else
		  {
		    printf ("removing `%s'\n", dp_fullpath);
		    unlink (dp_fullpath);
		  }
	      }
	  }
	free (dp_fullpath);
	closedir (dirp);
      }
  }

  // read default service config
  char **default_envp = envp;
  default_envp = envp_dup_update_or_add
    (default_envp, parse_conf_file (broker.confpath)->envs);

  default_envp = envp_dup_update_or_add (default_envp, extraenvs);
  // TODO: wipe dead services in another thread.

  // create sockpath parent directory:
  makeancesdirs (broker.sockpath);

  broker.sock = sock_addr_bind (SOCK_STREAM, broker.sockpath, true);
  // or: SOCK_STREAM | SOCK_CLOEXEC;

  // int main_process_pgid = getpgid (0);

  sigaction_inst (SIGHUP, sighup_handler);

  // daemonize
  if (!do_not_fork)
    {
      {
	int temp_i = fork ();
	if (temp_i < 0)
	  exit (1);		/* fork error */
	if (temp_i > 0)
	  exit (0);
      }				/* parent exits */

      if (setsid () == -1)
	{
	  perror ("setsid");
	  exit (1);
	}
    }

  openlog ("Clove", LOG_PID, LOG_DAEMON);
  char *current_hostname = malloc (4096);	// TODO: fix hardcoded size
  gethostname (current_hostname, 4096);	// TODO: fix hardcoded size
  syslog (LOG_INFO, "Connection from host %s", current_hostname);
  syslog (LOG_ALERT, "Init complete.");
  // TOOD: log the rest of stderr, too.

  int main_process_pid = getpid ();

  while (running)
    {
      int sock_a;
      if ((sock_a = accept (broker.sock, NULL, NULL)) == -1)
	{
	  perror ("accept");
	  exit (2);
	}

      int client_handler_pid;
      if ((client_handler_pid = fork ()) == 0)
	{
	  close (broker.sock);
	  close (0);

	  char buf[BROKER_MESSAGE_LENGTH];
	  if (read (sock_a, buf, BROKER_MESSAGE_LENGTH) < 0)
	    {
	      perror ("(read) Could not read from the client");
	      exit (1);
	    }
	  if (buf[0] == 0)	// remote command
	    {
	      char *remote_command = strndup (buf + 1, 222);	// TODO: fix hardcoded size
	      if (strcmp (remote_command, "quit") == 0)
		{
		  kill (main_process_pid, SIGHUP);
		}
	      else if (strcmp (remote_command, "stats") == 0)
		{
		  printf ("I should show the number of current "
			  "subprocesses.\nNot implemented yet.\n");
		}
	      else if (strcmp (remote_command, "ping") == 0)
		{
		  char *reply = malloc (128);	// TODO: fix hardcoded size
		  sprintf (reply, "%d", main_process_pid);
		  if (write (sock_a, reply, strlen (reply)) < 0)
		    {
		      perror
			("(write) Did not receive a reply from the client");
		      exit (1);
		    }
		}
	    }
	  else			// service
	    {
	      char *service_name = strndup (buf, 222);	// TODO: fix hardcoded size

	      /*  TODO: see what happens to process group when the
		  parent exits.  At the beginning read about former
		  processes and a killer for each to the specific
		  signal handler. Add the option -Q to kill all
		  current and former children. */

	      struct service srv = service_init (service_name);

	      // TODO: Check if service listens to its socket.  Find a
	      // better way to check if the service is alive.
	      if (access (srv.sockpath, F_OK) != 0)
		{
		  service_call (srv, default_envp);
		}

	      if (write (sock_a, srv.sockpath, strlen (srv.sockpath)) < 0)
		{
		  perror ("(write) Could not talk to the service");
		  exit (1);
		}
	    }			// TODO: check return val.
	  close (sock_a);
	  exit (0);
	}
      int status;
      waitpid (client_handler_pid, &status, 0);
      close (sock_a);
    }
  close (broker.sock);
  unlink (broker.sockpath);
  closelog ();

  return 0;
}
