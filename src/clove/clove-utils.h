#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

// ?
#include <signal.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
// #include <error.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <limits.h>

#include <sys/file.h>
#include <fcntl.h>

// #include <readline/readline.h>
// #include <readline/history.h>
// #include <pthread.h>

#define true 1
#define false 0

#define RED(x)    "\033[31m" x "\033[00m"
#define GREEN(x)  "\033[32m" x "\033[00m"
#define YELLOW(x) "\033[33m" x "\033[00m"
#define BLUE(x)   "\033[34m" x "\033[00m"

#define CONFLINE_MAX 8192
// maximum size of each line in the config file.

#define DEBUG

#ifdef DEBUG
#define DBG(format, args...) fprintf (stderr, RED ("%s:%-5d %s> ") format "\n", __FILE__, __LINE__, timespec_to_str (gettimeofday_ts ()), args)

struct timespec
gettimeofday_ts ();

char *
timespec_to_str (struct timespec ts);
#endif // DEBUG

#ifndef _GNU_SOURCE
size_t
strnlen(const char *s, size_t len);

char*
strndup (char const *s, size_t n);
#endif // _GNU_SOURCE

inline char*
str_beginswith (char* haystack, char* needle);

struct str_list {
  char* str;
  struct str_list* next;
};

struct str_list*
str_list_cons (char* str, struct str_list* lst);

void
str_list_nconcat (struct str_list** lst, struct str_list* tail);

struct str_list*
str_list_from_vec (char* vec[], int beg, int end);

char*
str_list_pop (struct str_list** lst);

void
str_list_nreverse (struct str_list** lst);

int
str_list_count (struct str_list* lst);

void
str_list_free (struct str_list* lst);

struct str_list*
str_split (char* str, char* delims);
/* destructively split string into a string list.  
   str cannot be const (or non-writable).  empty tokens are
   suppressed. */

struct str_list*
str_split_n (char* str, int limit, char* delims);
/* same as str_split_n, except there is a limit to number of
   delimitions. */

char*
str_replace (char* s, char* from, char* to);
// create a new string with all instances of from replace to to.

inline char*
strlcpy_p (char* dest, const char* src, const char* dest_limit);
/* If dest is NULL do nothing and just return NULL.  Otherwise, copy
   the string from src to dest, up to a certain point in dest (the
   memory location of the limit is passed to the function).  If '\0'
   does not occur within that range (src string is too long), return
   NULL.
   Also, if src is NULL, only a 0 is appended at the end of dest. */

char*
rtprefix ();
/* real time prefix. Could be "" or "$HOME/.local" or some such. */

struct remote_fds {
  int in;
  int out;
  int err;
};

#define broker_message_length 8192
#define sockpath_length 256
#define args_max 256
#define envs_max 256

#define LISTEN_BACKLOG 64

struct sockaddr_gen {
  int domain;
  int type;
  int protocol;
  struct sockaddr* addr;
  socklen_t len;
};

struct sockaddr_gen
addr_unix (int type, const char* sockpath);

// int sock_bind (struct sockaddr_gen a, int force_bind);

// int sock_connect (struct sockaddr_gen a);

int
sock_addr_bind (int type, char* sockpath, int force_bind);

int
sock_addr_connect (int type, char* sockpath);

int
unix_send_fds (int sock, struct remote_fds iofds);

int
unix_recv_fds (int sock, struct remote_fds* iofds_p);

struct service
  { char* name;
    char* confpath; /* the default path to the config file */
    time_t confpath_last_mtime;
    char* binpath; /* the default path to the executable */
    char* sockpath;
    int pid;
    int sock; };

struct serviceconf
  { struct str_list* interpretter;
    struct str_list* envs; };

char*
service_socket_path_dir ();

struct service
service_init (char* service_name);

char*
service_call (struct service srv, char** default_envp);

struct serviceconf*
parse_conf_file (char* filepath);

char**
envp_dup_update_or_add (char** envp, struct str_list* extraenvs);

char**
argv_dup_add (char** oldargv, struct str_list* prefixargv);

int
makeancesdirs (char* path);
/* Input should not have .. or . path components. */

void
sigaction_inst (int signum, void (*handler)(int));
