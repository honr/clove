#include "clove-common.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sys/utsname.h>
#include <limits.h>

#include <signal.h>             // ?

#define CONFLINE_MAX 8192
// Maximum size of each line in the config file.

char *RTPREFIX;  // Has to be defined in your code. e.g.: = "~/.local";
char *RUNPATH;   // Same, e.g.: = "~/.local/var/run/clove";

char *str_beginswith (char *haystack, char *needle);

struct str_list
{
  char *str;
  struct str_list *next;
};

struct str_list *str_list_cons (char *str, struct str_list *lst);

void str_list_nconcat (struct str_list **lst, struct str_list *tail);

struct str_list *str_list_from_vec (char *vec[], int beg, int end);

char *str_list_pop (struct str_list **lst);

void str_list_nreverse (struct str_list **lst);

int str_list_count (struct str_list *lst);

void str_list_free (struct str_list *lst);

struct str_list *str_split (char *str, char *delims);
/* Destructively split string into a str_list.
   Str cannot be const (or non-writable).  Empty tokens are suppressed. */

struct str_list *str_split_n (char *str, int limit, char *delims);
/* Same as str_split, except there is a limit to number of delimitions. */

struct str_list *str_split_qe (char *buf, size_t buf_size);
/* Split buf of maximum size buf_size into a str_list.
   Empty terms are not allowed (terminates the str_list at that
   point).  Quotations (single and double quote characters) and also
   escaping (using backslash) are respected. */

void *str_list_to_pack (char **buf_cur, const char *buf_lim,
                        struct str_list *lst);
/* Place the str_list lst in the buffer. Places a NULL character after
   each string. Two consecutive NULLs show the end of pack. buf_lim
   points to right after the end of buffer, beyond which we never
   write. buf_cur will be modified to point to right after the pack in
   the buffer.
   You can use the idiom: buf_cur = buf; buf_lim = buf + buffer_size;

   None of the strings should be empty.
   TODO: relax this restriction. use the format ~aaa0~bbb0~ccc00. */

struct str_list *str_list_from_pack (char **buf_cur, const char *buf_lim);
/* Create a str_list from a pack in the input buffer.
   buf_cur will be updated to point to right after the pack in the
   buffer.  buf_lim shows the extent right before which we read the
   input buffer. */

char *str_concat (char *s1, char *s2);
/* Return concatenation of s1 and s2. Modify none. */

char *str_replace (char *s, char *from, char *to);
/* Create a new string with all instances of from replace to to. */

char *strlcpy_p (char *dest, const char *src, const char *dest_limit);
/* If dest is NULL do nothing and just return NULL.  Otherwise, copy
   the string from src to dest, up to a certain point in dest (the
   memory location of the limit is passed to the function).  If '\0'
   does not occur within that range (src string is too long), return
   NULL.
   Also, if src is NULL, only a 0 is appended at the end of dest. */

char *expand_file_name (char *filename);
/* Replace ~ with $HOME */

#define sockpath_length 256
#define args_max 256
#define envs_max 256

struct service
{
  char *name;
  char *confpath;  // The default path to the config file.
  time_t confpath_last_mtime;
  char *binpath;  // The default path to the executable.
  char *sockpath;
  int pid;
  int sock;
};

struct serviceconf
{
  struct str_list *interpretter;
  struct str_list *envs;
};

char *service_socket_path_dir ();

struct service service_init (char *service_name);

char *service_call (struct service srv, char **default_envp);

struct serviceconf *parse_conf_file (char *filepath);

char **envp_dup_update_or_add (char **envp, struct str_list *extraenvs);

char **argv_dup_add (char **oldargv, struct str_list *prefixargv);

int makeancesdirs (char *path);
/* Input should not have ".." or "." path components. */

void sigaction_inst (int signum, void (*handler) (int));
