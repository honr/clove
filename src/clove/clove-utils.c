#include "clove-utils.h"

char* RTPREFIX = "~/.local";
// getenv ("RTPREFIX");

#ifdef DEBUG
struct timespec
gettimeofday_ts ()
  { struct timeval now_tv;
    gettimeofday (&now_tv, NULL);
    struct timespec now_ts = {now_tv.tv_sec, (long) now_tv.tv_usec * 1000};
    return now_ts; }

char *
timespec_to_str (struct timespec ts)
  { char * s = (char *) malloc (256);
    sprintf (s, "%ld.%ld", ts.tv_sec, ts.tv_nsec);
    return s; }
#endif // DEBUG

// for OSX
#ifndef _GNU_SOURCE

size_t strnlen(const char *s, size_t len)
  { size_t i;
    for (i=0; i < len && *s; i++, s++);
    return i; }

char* strndup (char const *s, size_t n)
  { size_t len = strnlen (s, n); // or n-1 ?
    char *t;

    if ((t = malloc (len + 1)) == NULL)
      return NULL;

    t[len] = '\0';
    return memcpy (t, s, len); }

#endif

inline char* strlcpy_p (char* dest, const char* src, const char* dest_limit)
/* If dest is NULL do nothing and just return NULL.  Otherwise, copy
   the string from src to dest, up to a certain point in dest (the
   memory location of the limit is passed to the function).  If '\0'
   does not occur within that range (src string is too long), return
   NULL.
   Also, if src is NULL, only a 0 is appended at the end of dest. */
  { char* ret = dest;
    int n = dest_limit - dest - 1;
    if (dest && (n > 0))
      { if (src)
	  { ret = memccpy (dest, src, 0, n); }
	else // only append a 0 at the dest.
	  { *(ret++) = 0; }}
    return ret; }

inline char* str_beginswith (char* haystack, char* needle)
  { register char* cur1;
    register char* cur2;
    for (cur1 = haystack, cur2 = needle;
	 (*cur1) && (*cur2) && (*cur1 == *cur2);
	 cur1++, cur2++)
      {}
    if ((*cur2) == '\0')
      { return cur1; }
    else
      { return NULL; }}

struct str_list* str_list_cons (char* str, struct str_list* lst)
  { struct str_list* head =
      (struct str_list*) malloc (sizeof (struct str_list));
    head->str  = str;
    head->next = lst;
    return head; }

void
str_list_nconcat (struct str_list** lst, struct str_list* tail)
  { if (*lst)
      { struct str_list* nxt = *lst;
	while (nxt->next)
	  { nxt = nxt->next; }
	nxt->next = tail; }
    else
      { *lst = tail; }}

struct str_list*
str_list_from_vec (char* vec[], int beg, int end)
  { struct str_list* lst = NULL;
    for (; end >= beg; end--)
      { lst = str_list_cons (vec[end], lst); }
    return lst; }

char* str_list_pop (struct str_list** lst)
  { if (lst && (*lst) && (*lst)->str)
      { char* ret = (*lst)->str;
	(*lst) = (*lst)->next;
	return ret; }
    else
      { return NULL; }}
// Currently pop leaks, since the address to the next is not freed.

void str_list_nreverse (struct str_list** lst)
  { if (lst)
      { struct str_list* nxt;
	struct str_list* prv = NULL;
	while (true) 
	  { nxt = (*lst)->next;
	    (*lst)->next = prv; 
	    prv = (*lst);
	    if (nxt) 
	      { (*lst) = nxt; }
	    else
	      { break; }}}}

int str_list_count (struct str_list* lst)
  { int count;
    for (count = 0; lst; lst = lst->next, count ++)
      {}
    return count; }

void str_list_free (struct str_list* lst)
  { struct str_list* nxt;
    while (lst)
      { nxt = lst->next;
	free (lst->str);
	free (lst);
	lst = nxt; }}

struct str_list* str_split (char* str, char* delims)
  { struct str_list* head = NULL;
    char* running = strdup (str);
    char* token;
    while ((token = strsep (&running, delims)))
      { if (*token)
	  { head = str_list_cons (token, head); }}
    return head; }

struct str_list* str_split_n (char* str, int limit, char* delims)
  { struct str_list* head = NULL;
    char* running = strdup (str);
    char* token;
    while ((limit > 1) && (token = strsep (&running, delims)))
      { if (*token)
	  { head = str_list_cons (token, head);
	    limit --; }}
    if (*running)
      { head = str_list_cons (running, head); }
    return head; }

char* str_replace (char* s, char* from, char* to)
  { int size_from = strlen (from);
    int size_to = strlen (to);
    int size_s = strlen (s);
    int i;
    char* cur;
    // pass 1: find the number of occurances, to find the size of the
    //         result.
    for (cur = strstr (s, from), i = 0;
	 cur && *cur;
	 cur = strstr(cur + size_from, from), i++);
    char* res = (char*) malloc (size_s + (size_to - size_from) * i + 1);
    char* resp = res;
    // pass 2: copy.
    char* prev;
    for (prev = s, cur = strstr (prev, from);
	 cur;
	 prev = cur + size_from, cur = strstr (prev, from))
      { memcpy (resp, prev, cur - prev);
	resp += cur - prev;
	memcpy (resp, to, size_to);
	resp += size_to; }
    strcpy (resp, prev);

    return res; }

char* rtprefix ()
  { return str_replace (RTPREFIX, "~", getenv ("HOME")); }
// sprintf (path, "%s/.local", getenv ("HOME"));

struct sockaddr_gen addr_unix (int type, const char* sockpath)
  { struct sockaddr_gen a;
    struct sockaddr_un* addr = (struct sockaddr_un*) malloc (sizeof (struct sockaddr_un));
    // printf ("path: %s\n", sockpath);
    addr->sun_family = AF_UNIX;

    strcpy (addr->sun_path, sockpath);
    if (sockpath[0] == '@') // we assume an abstract path was intended
      { addr->sun_path[0] = 0;
	/* We consider the null byte at the end of sockpath NOT part
	   of the name */
	a.len = strlen (sockpath) + sizeof (addr->sun_family); }
    else
      { a.len = strlen (sockpath) + 1 + sizeof (addr->sun_family); }

    a.addr = (struct sockaddr *) addr;
    a.domain = AF_UNIX;
    a.type = type;
    a.protocol = 0;
    return a; }

int sock_bind (struct sockaddr_gen a, int force_bind)
  { int sock;

    if ((sock = socket (a.domain, a.type, a.protocol)) < 0)
      { perror ("socket");
        exit(2); }

    if (bind (sock, a.addr, a.len) != 0)
      { if (force_bind)
	  { perror ("bind");
	    exit (2); }
	else
	  { // perror ("bind");
	    exit (0); }}

    if (a.type & SOCK_STREAM)
      { if (listen (sock, LISTEN_BACKLOG) != 0)
	  { perror ("listen");
	    exit(2); }}

    return sock; }

int sock_connect (struct sockaddr_gen a)
  { int sock;

    if ((sock = socket (a.domain, a.type, a.protocol)) < 0)
      { perror ("socket");
        // exit(2);
	return sock; }

    if (a.type & SOCK_STREAM)
      { if (connect (sock, a.addr, a.len) != 0)
	  { // perror ("connect");
	    // TODO: throw exception
	    close (sock);
	    return -1;
	    // exit(2);
	  }}

    return sock; }

int sock_addr_bind (int type, char* sockpath, int force_bind)
  { struct sockaddr_gen a = addr_unix (type, sockpath);
    return sock_bind (a, force_bind); }

int sock_addr_connect (int type, char* sockpath)
  { struct sockaddr_gen a = addr_unix (type, sockpath);
    return sock_connect (a); }

/* TODO: pass credentials. the receiving end should simply abort
         communications if the client's uid is not the same as the
         service's. */

int unix_send_fds (int sock, struct remote_fds iofds)
  { char cmsg_buf[CMSG_SPACE(sizeof(iofds))];
    char *m = "message";
    struct iovec iov = { m, strlen(m) };

    struct msghdr msgh =
      { .msg_name       = NULL,
        .msg_namelen    = 0,
        .msg_iov        = &iov,
        .msg_iovlen     = 1,
        .msg_control    = &cmsg_buf,
        .msg_controllen = sizeof (cmsg_buf), // CMSG_LEN(sizeof(iofds));
        .msg_flags      = 0 };

    struct cmsghdr *cmsg;
    cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(iofds));

    memcpy(CMSG_DATA(cmsg), &iofds, sizeof(iofds));
    msgh.msg_controllen = cmsg->cmsg_len;
    return sendmsg(sock, &msgh, 0); } // MSG_NOSIGNAL

int unix_recv_fds(int sock, struct remote_fds* iofds_p)
  { // TODO: check buffer sizes (cmsg_buf_size, msg, iofds);
    struct iovec iov;
    int cmsg_buf_size = 128; // CMSG_SPACE(sizeof(iofds))
    char cmsg_buf[cmsg_buf_size];

    char msg[128];
    iov.iov_base = msg;
    iov.iov_len  = 7;  // "message"

    struct msghdr msgh =
      { .msg_name       = NULL,
        .msg_namelen    = 0,
        .msg_iov        = &iov,
        .msg_iovlen     = 1,
        .msg_control    = cmsg_buf,
        .msg_controllen = sizeof (cmsg_buf),
	.msg_flags      = 0 };

    int ret = 0;
    int ret0;
    if ((ret0 = recvmsg (sock, &msgh, 0)) > 0)
      { struct cmsghdr* cmsg = NULL;
	// *iofds_p = NULL;
        for (cmsg  = CMSG_FIRSTHDR(&msgh);
             cmsg != NULL;
             cmsg  = CMSG_NXTHDR(&msgh, cmsg))
          { if ((SOL_SOCKET == cmsg->cmsg_level) &&
                (SCM_RIGHTS == cmsg->cmsg_type))
              { // *iofds_p = (struct remote_fds*) malloc (sizeof (struct remote_fds));
		memcpy (iofds_p, CMSG_DATA(cmsg), sizeof (struct remote_fds));
		ret = ret0;
                // TODO: study: (cmsg->cmsg_len) > sizeof (struct remote_fds)
                break; }}

        if (cmsg == NULL)
          { fprintf (stderr, "control header is NULL\n"); }}
    else
      { fprintf (stderr, "errno: %d\n", errno);
        perror ("recvmsg"); }

    return ret; }

char* service_socket_path_dir ()
  { char* path = malloc (256); // FIX HARDCODED
    sprintf (path, "%s/var/run/clove", rtprefix ());
    return path; }

struct service
service_init (char* service_name)
  { struct service s;
    char* prefix = rtprefix ();
    s.name = service_name;
    s.confpath = malloc (256); // FIX HARDCODED
    s.binpath  = malloc (256); // FIX HARDCODED
    s.sockpath = malloc (256); // FIX HARDCODED

    if (strcmp (s.name, "broker") == 0)
      { sprintf (s.confpath, "%s/etc/clove.conf", prefix);
	s.binpath = NULL;
	sprintf (s.sockpath, "%s/broker", service_socket_path_dir ()); }
    else
      { sprintf (s.confpath, "%s/etc/clove-%s.conf", prefix, s.name);
	sprintf (s.binpath,  "%s/bin/clove-%s", prefix, s.name);
	sprintf (s.sockpath, "%s/clove-%s", service_socket_path_dir (), s.name); }

    s.confpath_last_mtime = 0;
    s.pid = -1;
    s.sock = -1;
    return s; }

char*
service_call (struct service srv, char** default_envp)
  { int pipefd[2];
    if (pipe (pipefd) != 0)
      { perror ("pipe"); }
    int child;
    if ((child = fork ()) == 0)
      { close (pipefd[0]);
	close (1);
	if (dup2 (pipefd[1], 1) == -1)
	  { perror ("dup2"); }
	// char* service_argv_dummy[] = { NULL, "hello", "world", NULL };
	char* service_argv_dummy[] = { NULL, NULL };
	char** service_argv = service_argv_dummy;
	service_argv[0] = srv.binpath;
	char** service_envp = default_envp;
	// TODO: devise ways to:
	//   - start, stop, restart, status on a service
	if (access (srv.confpath, F_OK) == 0)
	  { struct serviceconf* sc = parse_conf_file (srv.confpath);
	    service_envp = envp_dup_update_or_add (service_envp, sc->envs);
	    // TODO: make sure this is working properly.  Apparently
	    //       the order of a duplicate env vars is different
	    //       between Darwin and Linux.
	    if (sc->interpretter)
	      { service_argv[0] = srv.binpath;
		service_argv = argv_dup_add (service_argv, sc->interpretter);
		char* interpretter = service_argv[0];
		execve (interpretter, service_argv, service_envp);
	        exit (3); }}
	
	if (execve (srv.binpath, service_argv, service_envp) == -1)
	  { perror ("execve");
	    exit (3); }}
    close (pipefd [1]);
    printf ("waiting for %s ...\n", srv.name);
    // TODO: have a time out
    char* buf = malloc (128);
    read (pipefd[0], buf, 127);
    buf[127]=0;
    printf ("%s says: %s", srv.name, buf);
    close (pipefd[0]);
    return buf; }

struct serviceconf* parse_conf_file (char* filepath)
/* TODO: 
   cur_uname = uname ();
   cur_uname_m = uname ("-m");
   archmatcher="(^$(uname) $(uname -m))|(^$(uname)/)|(^/)|(^[^/]+=)|(^[^/]+$)"
   
   TODO: have platform dependent interpretters.

   TODO: split key and value, and perform ~~ (or $THIS) replacement.
 */
  { FILE* file;
    char line_storage[8192]; // TODO: fix hardcoded

    struct serviceconf* sc = (struct serviceconf*) malloc 
      (sizeof (struct serviceconf));
    sc->interpretter = NULL;
    sc->envs = NULL;
    
    struct utsname unm;
    uname (&unm);
    
    char* whitespacechars = " \f\n\r\t\v";
    if ((file = fopen (filepath, "r")))
      { while (fgets (line_storage, 8192, file))
	  { char* line = strndup (line_storage, strcspn (line_storage, "\n"));
	    if (line[0] == '/')
	      { line = line + 1; 	/* move beyond the '/' */
		char* platform_specifier;
		platform_specifier = strsep (&line, "/");
		// fprintf (stderr, "specifier: %s\n", platform_specifier);
		if ((*platform_specifier) && 
		    ((platform_specifier = str_beginswith 
		      (platform_specifier, unm.sysname)) == NULL))
		  { // fprintf (stderr, "sysname does not match\n");
		    continue; }
		if (*platform_specifier)
		  { platform_specifier ++; /* move past delimeter */ }
		if ((*platform_specifier) &&
		    ((platform_specifier = str_beginswith 
		      (platform_specifier, unm.machine)) == NULL))
		  { // fprintf (stderr, "machine does not match\n");
		    continue; }}
	    
	    /* if the line starts with a '/', there must be another
	       '/' somewhere after that.  take the string between the
	       two '/'s as platform specifier, and drop the line
	       unless the specifier matches current platform.
	       removed. */

	    // printf ("%s, %s\n", line, str_beginswith (line, "#!"));

	    if (sc->interpretter == NULL)
	      { char* line1 = str_beginswith (line, "#!");
		if (line1)
		  { sc->interpretter = str_split 
		      (str_replace (line1, "~", getenv ("HOME")), 
		       whitespacechars);
		    fprintf (stderr, "%s\n", line); // printout accepted line
		    continue; }}

	    /* skip leading whitespace, take until '#' */
	    line += strspn (line, whitespacechars);
	    line = strsep (&line, "#");

	    // line_eff = str_replace (line, "~~", former_value_of_key); 
	    // move the above to envp_dup_update_or_add
	    line = str_replace (line, "~", getenv ("HOME"));
	    if (line && *line) // not empty
	      { fprintf (stderr, "%s\n", line); // printout accepted line
		sc->envs = str_list_cons (line, sc->envs); }}
	fclose (file); }
    return sc; }

char** envp_dup_update_or_add (char** envp, struct str_list* extraenvs)
  { // find the number of envp key-vals.
    if (extraenvs == NULL)
      { return envp; }
    // otherwise, we need to make a new envp.

    int envp_count = 0;
    char** cur1; char** cur2; struct str_list* cur3;
    for (cur1 = envp; *cur1; cur1++, envp_count++);
    for (cur3 = extraenvs; cur3; cur3 = cur3->next, envp_count++);

    char** envp_new = (char**) malloc ((envp_count + 2) * (sizeof (char*)));

    char* curstr;
    for (cur1 = envp, cur2 = envp_new;
	 *cur1;
	 cur1 ++, cur2 ++)
      { for (cur3 = extraenvs; cur3; cur3 = cur3->next)
	  { if (((curstr = cur3->str) != NULL) &&
		(str_beginswith (*cur1, 
				 strndup (curstr, strcspn (curstr, "=") + 1))
		 != NULL))
	      { *cur2 = curstr;
		// fprintf (stderr, "updated: %s\n", *cur2);
		cur3->str = NULL; }
	    else
	      { *cur2 = *cur1; }}}

    for (cur3 = extraenvs; cur3; cur3 = cur3->next)
      { if ((curstr = cur3->str) != NULL)
	  { *cur2 = curstr;
	    // fprintf (stderr, "added: %s\n", *cur2);
	    cur2 ++; }}
    *cur2 = NULL;
    return envp_new; }

char**
argv_dup_add (char** oldargv, struct str_list* prefixargv)
  { int count_old, count;
    struct str_list* cur;
    for (count_old = 0; oldargv[count_old]; count_old ++)
      {}
    count = count_old + str_list_count (prefixargv);
    char** newargv = (char**) malloc ((count + 1) * sizeof (char*));
    for (; count_old >= 0; count_old--, count--)
      { newargv [count] = oldargv[count_old]; }
    for (cur = prefixargv; cur; count--, cur=cur->next)
      { newargv [count] = cur->str; }
    return newargv; }

int makeancesdirs (char* path)
  { if (*path)
      { char *p, *q, *path_dup = strndup (path, PATH_MAX);
	for (p = path_dup + 1, q = strchr (p, '/');
	     q;
	     q = strchr (p, '/'))
	  { *q = 0;
	    mkdir (path_dup, 01777);
	    // TOOD: make sure the mode 01777 is OK for all platforms.
	    *q = '/';
	    p = q + 1; }
	free (path_dup);
	return 0; } // success
    { return -1; }} // path was empty failed.
    
void sigaction_inst (int signum, void (*handler)(int))
  { struct sigaction new_action, old_action;

    new_action.sa_handler = handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction (signum, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
      sigaction (signum, &new_action, NULL); }
