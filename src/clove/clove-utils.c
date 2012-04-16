#include "clove-common.h"
#include "clove-utils.h"

inline char *
strlcpy_p (char *dest, const char *src, const char *dest_limit)
/* If dest is NULL do nothing and just return NULL.  Otherwise,
   prepend the character `S' and copy the string from src to dest, up
   to a certain point in dest (the memory location of the limit is
   passed to the function).  If '\0' does not occur within that range
   (src string is too long), return NULL.
   Also, if src is NULL, only a 0 is appended at the end of dest. */
{
  int n = dest_limit - dest - 1;
  if (dest && (n > 1))
    {
      if (src)
        {
          *(dest++) = 'S';
          dest = memccpy (dest, src, 0, n);
        }
      else  // Only append a 0 at the dest.
        {
          *(dest++) = 0;
        }
      return dest;
    }
  else
    {
      return NULL;
    }
}

inline char *
str_beginswith (char *haystack, char *needle)
{
  register char *cur1;
  register char *cur2;
  for (cur1 = haystack, cur2 = needle;
       (*cur1) && (*cur2) && (*cur1 == *cur2); cur1++, cur2++)
    {
    }
  if ((*cur2) == '\0')
    {
      return cur1;
    }
  else
    {
      return NULL;
    }
}

struct str_list *
str_list_cons (char *str, struct str_list *lst)
{
  struct str_list *head =
    (struct str_list *) malloc (sizeof (struct str_list));
  head->str = str;
  head->next = lst;
  return head;
}

void
str_list_nconcat (struct str_list **lst, struct str_list *tail)
{
  if (*lst)
    {
      struct str_list *nxt = *lst;
      while (nxt->next)
        {
          nxt = nxt->next;
        }
      nxt->next = tail;
    }
  else
    {
      *lst = tail;
    }
}

struct str_list *
str_list_from_vec (char *vec[], int beg, int end)
{
  struct str_list *lst = NULL;
  for (; end >= beg; end--)
    {
      lst = str_list_cons (vec[end], lst);
    }
  return lst;
}

// Currently str_list_pop leaks, since the address to the next is not freed.
char *
str_list_pop (struct str_list **lst)
{
  if (lst && (*lst) && (*lst)->str)
    {
      char *ret = (*lst)->str;
      (*lst) = (*lst)->next;
      return ret;
    }
  else
    {
      return NULL;
    }
}

void
str_list_nreverse (struct str_list **lst)
{
  if (lst)
    {
      struct str_list *nxt;
      struct str_list *prv = NULL;
      while (true)
        {
          nxt = (*lst)->next;
          (*lst)->next = prv;
          prv = (*lst);
          if (nxt)
            {
              (*lst) = nxt;
            }
          else
            {
              break;
            }
        }
    }
}

int
str_list_count (struct str_list *lst)
{
  int count;
  for (count = 0; lst; lst = lst->next, count++)
    {
    }
  return count;
}

void
str_list_free (struct str_list *lst)
{
  struct str_list *nxt;
  while (lst)
    {
      nxt = lst->next;
      free (lst->str);
      free (lst);
      lst = nxt;
    }
}

struct str_list *
str_split (char *str, char *delims)
{
  struct str_list *head = NULL;
  char *running = strdup (str);
  char *token;
  while ((token = strsep (&running, delims)))
    {
      if (*token)
        {
          head = str_list_cons (token, head);
        }
    }
  return head;
}

struct str_list *
str_split_n (char *str, int limit, char *delims)
{
  struct str_list *head = NULL;
  char *running = strdup (str);
  char *token;
  while ((limit > 1) && (token = strsep (&running, delims)))
    {
      if (*token)
        {
          head = str_list_cons (token, head);
          limit--;
        }
    }
  if (*running)
    {
      head = str_list_cons (running, head);
    }
  return head;
}

struct str_list *
str_split_qe (char *buf, size_t buf_size)
{
  char *buf_cur, *bufout_cur, c, state, action, quote;
  const char STATE_ESCAPE = 2;
  const char STATE_TOKEN = 1;
  const char STATE_SPACE = 0;
  char *bufout = malloc (buf_size);
  for (buf_cur = buf, bufout_cur = bufout, state = STATE_SPACE, quote = 0;
       *buf_cur; buf_cur++)
    {
      c = *buf_cur;
      action = 0;

      // State Transitions:
      if (state & STATE_ESCAPE)
        {
          state ^= STATE_ESCAPE;
        }
      else if (c == '\\')
        {
          state = STATE_TOKEN | STATE_ESCAPE;
          action = 'f';
        }
      else if (state == STATE_SPACE)
        {
          if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
            {
              action = 'p';
            }  // Pass.
          else if ((c == '"') || (c == '\''))
            {
              state = STATE_TOKEN;
              action = 'p';
              quote = c;
            }
          else
            {
              state = STATE_TOKEN;
            }
        }  // Hold head.
      else if (state == STATE_TOKEN)
        {
          if (quote)
            {
              if (c == quote)
                {
                  state = STATE_SPACE;
                  action = 'c';
                  quote = 0;
                }
            }
          else
            {
              if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
                {
                  state = STATE_SPACE;
                  action = 'c';
                }
              else if ((c == '"') || (c == '\''))
                {
                  state = STATE_TOKEN;
                  action = 'c';
                  quote = c;
                }
            }
        }

      if ((action == 'c') || (action == 'f'))  // Copy or flush.
        {
          memcpy (bufout_cur, buf, buf_cur - buf);
          bufout_cur += buf_cur - buf;
          buf = buf_cur;
          if (action == 'c')  // Copy.
            {
              *bufout_cur = 0;  // NULL terminate.
              bufout_cur++;
            }
        }
      if (action)
        {
          buf++;
        }
    }

  // Termination:
  if (state & STATE_ESCAPE)
    {
      fprintf (stderr, "Error: Tried to escape end-of-line.\n"
               "       Reading next line is not implemented, yet.\n");
    }  // TODO: read next line.
  else if (state == STATE_TOKEN)
    {
      if (quote)
        {
          fprintf (stderr,
                   "Error: Tried to extend quotation to the next line.\n"
                   "       Reading next line is not implemented, yet.\n");
        }
      else
        {
          memcpy (bufout_cur, buf, buf_cur - buf);
          bufout_cur += buf_cur - buf;
          buf = buf_cur + 1;  // Redundant.
          *bufout_cur = 0;  // NULL terminate.
          bufout_cur++;
        }
    }
  // End.

  *bufout_cur = 0;  // NULL terminate.
  return str_list_from_pack (&bufout, bufout + buf_size);
}

void *
str_list_to_pack (char **buf_cur, const char *buf_lim, struct str_list *lst)
{
  char *buf = *buf_cur;
  char *cur;
  for (cur = str_list_pop (&lst); cur; cur = str_list_pop (&lst))
    {
      buf = strlcpy_p (buf, cur, buf_lim);
    }
  buf = strlcpy_p (buf, NULL, buf_lim);
  *buf_cur = buf;
  return buf;
}

struct str_list *
str_list_from_pack (char **buf_cur, const char *buf_lim)
{
  char *buf = *buf_cur;
  int l;
  struct str_list *lst = NULL;
  for (; *buf;)
    {
      if ((l = strnlen (buf, buf_lim - buf)) < buf_lim - buf)
        {
          lst = str_list_cons (buf, lst);
          buf += l + 1;
        }  // Skip the string and its NULL.
      else
        {
          break;
        }
    }
  *buf_cur = buf + 1;
  // str_list_nreverse (&lst);
  return lst;
}

char *
str_concat (char *s1, char *s2)
{
  int size_1 = strlen (s1);
  int size_2 = strlen (s2);
  char *res = (char *) malloc (size_1 + size_2 + 1);
  char *resp = res;
  memcpy (resp, s1, size_1);
  resp += size_1;
  memcpy (resp, s2, size_2);
  resp += size_2;
  *resp = 0;
  return res;
}

char *
str_replace (char *s, char *from, char *to)
{
  int size_from = strlen (from);
  int size_to = strlen (to);
  int size_s = strlen (s);
  int i;
  char *cur;
  // Pass 1: Find the number of occurances, to find the size of the result.
  for (cur = strstr (s, from), i = 0;
       cur && *cur; cur = strstr (cur + size_from, from), i++);
  char *res = (char *) malloc (size_s + (size_to - size_from) * i + 1);
  char *resp = res;
  // Pass 2: Copy.
  char *prev;
  for (prev = s, cur = strstr (prev, from);
       cur; prev = cur + size_from, cur = strstr (prev, from))
    {
      memcpy (resp, prev, cur - prev);
      resp += cur - prev;
      memcpy (resp, to, size_to);
      resp += size_to;
    }
  strcpy (resp, prev);

  return res;
}

char *
expand_file_name (char *filename)
{
  return str_replace (filename, "~", getenv ("HOME"));
}

// sprintf (path, "%s/.local", getenv ("HOME"));

struct service
service_init (char *service_name)
{
  struct service s;
  char *prefix = expand_file_name (RTPREFIX);
  s.name = service_name;
  s.confpath = malloc (PATH_MAX);
  s.binpath = malloc (PATH_MAX);
  s.sockpath = malloc (PATH_MAX);

  if (strcmp (s.name, "broker") == 0)
    {
      sprintf (s.confpath, "%s/share/clove/clove.conf", prefix);
      s.binpath = NULL;
      sprintf (s.sockpath, "%s/broker", expand_file_name (RUNPATH));
    }
  else
    {
      sprintf (s.confpath, "%s/share/clove/clove-%s.conf", prefix, s.name);
      sprintf (s.binpath, "%s/share/clove/clove-%s", prefix, s.name);
      sprintf (s.sockpath, "%s/clove-%s", expand_file_name (RUNPATH), s.name);
    }

  s.confpath_last_mtime = 0;
  s.pid = -1;
  s.sock = -1;
  return s;
}

char *
service_call (struct service srv, char **default_envp)
{
  int pipefd[2];
  if (pipe (pipefd) != 0)
    {
      perror ("pipe");
    }
  int child;
  if ((child = fork ()) == 0)
    {
      close (pipefd[0]);
      close (1);
      if (dup2 (pipefd[1], 1) == -1)
        {
          perror ("dup2");
        }
      // char* service_argv_dummy[] = { NULL, "hello", "world", NULL };
      char *service_argv_dummy[] = { NULL, NULL };
      char **service_argv = service_argv_dummy;
      service_argv[0] = srv.binpath;
      char **service_envp = default_envp;
      // TODO: Devise ways to:
      //       - start,
      //       - stop,
      //       - restart,
      //       - status
      //       on a service.
      if (access (srv.confpath, F_OK) == 0)
        {
          struct serviceconf *sc = parse_conf_file (srv.confpath);
          service_envp = envp_dup_update_or_add
            (service_envp,
             str_list_cons (str_concat ("CLOVESOCKET=", srv.sockpath), NULL));
          service_envp = envp_dup_update_or_add (service_envp, sc->envs);
          // TODO: Make sure this is working properly.
          //       Apparently the order of a duplicate env vars is different
          //       between Darwin and Linux.
          if (sc->interpretter)
            {
              service_argv[0] = srv.binpath;
              service_argv = argv_dup_add (service_argv, sc->interpretter);
              char *interpretter = service_argv[0];
              execve (interpretter, service_argv, service_envp);
              exit (3);
            }
        }

      if (execve (srv.binpath, service_argv, service_envp) == -1)
        {
          perror ("execve");
          exit (3);
        }
    }
  close (pipefd[1]);
  printf ("waiting for %s ...\n", srv.name);
  // TODO: have a time out.
  char *buf = malloc (128);  // TODO: Fix hardcoded size.
  if (read (pipefd[0], buf, 127) < 0)  // TODO: Fix hardcoded size.
    {
      perror ("(read) could not communicate with the service");
      exit (1);
    }
  buf[127] = 0;  // TODO: Fix hardcoded size.
  printf ("%s says: %s", srv.name, buf);
  close (pipefd[0]);
  return buf;
}

struct serviceconf *
parse_conf_file (char *filepath)
/* TODO:
     cur_uname = uname ();
     cur_uname_m = uname ("-m");
     archmatcher="(^$(uname) $(uname -m))|(^$(uname)/)|(^/)|(^[^/]+=)|(^[^/]+$)"

   TODO: Have platform dependent interpretters.

   TODO: Split key and value, and perform ~~ (or $THIS) replacement.
*/
{
  FILE *file;
  char line_storage[CONFLINE_MAX];

  struct serviceconf *sc = (struct serviceconf *) malloc
    (sizeof (struct serviceconf));
  sc->interpretter = NULL;
  sc->envs = NULL;

  struct utsname unm;
  uname (&unm);

  char *whitespacechars = " \f\n\r\t\v";
  if ((file = fopen (filepath, "r")))
    {
      while (fgets (line_storage, CONFLINE_MAX, file))
        {
          char *line = strndup (line_storage, strcspn (line_storage, "\n"));
          if (line[0] == '/')
            {
              line = line + 1;  // Move beyond the '/'.
              char *platform_specifier;
              platform_specifier = strsep (&line, "/");
              // fprintf (stderr, "specifier: %s\n", platform_specifier);
              if ((*platform_specifier) &&
                  ((platform_specifier = str_beginswith
                    (platform_specifier, unm.sysname)) == NULL))
                { // fprintf (stderr, "sysname does not match\n");
                  continue;
                }
              if (*platform_specifier)
                {
                  platform_specifier++;  // Move past delimeter.
                }
              if ((*platform_specifier) &&
                  ((platform_specifier = str_beginswith
                    (platform_specifier, unm.machine)) == NULL))
                { // fprintf (stderr, "machine does not match\n");
                  continue;
                }
            }

          /* If the line starts with a '/', there must be another '/'
             somewhere after that.  Take the string between the two
             '/'s as platform specifier, and drop the line unless the
             specifier matches current platform.  Removed. */

          // printf ("%s, %s\n", line, str_beginswith (line, "#!"));

          if (sc->interpretter == NULL)
            {
              char *line1 = str_beginswith (line, "#!");
              if (line1)
                {
                  sc->interpretter = str_split
                    (str_replace (line1, "~", getenv ("HOME")),
                     whitespacechars);
                  fprintf (stderr, "%s\n", line);       // printout accepted line
                  continue;
                }
            }

          // Skip leading whitespace, take until '#'.
          line += strspn (line, whitespacechars);
          line = strsep (&line, "#");

          // line_eff = str_replace (line, "~~", former_value_of_key);
          // Move the above to envp_dup_update_or_add.
          line = str_replace (line, "~", getenv ("HOME"));
          if (line && *line)    // not empty
            {
              fprintf (stderr, "%s\n", line);  // Printout accepted line.
              sc->envs = str_list_cons (line, sc->envs);
            }
        }
      fclose (file);
    }
  return sc;
}

char **
envp_dup_update_or_add (char **envp, struct str_list *extraenvs)
{                               // find the number of envp key-vals.
  if (extraenvs == NULL)
    {
      return envp;
    }
  // Otherwise, we need to make a new envp.

  int envp_count = 0;
  char **cur1;
  char **cur2;
  struct str_list *cur3;
  for (cur1 = envp; *cur1; cur1++, envp_count++);
  for (cur3 = extraenvs; cur3; cur3 = cur3->next, envp_count++);

  char **envp_new = (char **) malloc ((envp_count + 2) * (sizeof (char *)));

  char *curstr;
  for (cur1 = envp, cur2 = envp_new; *cur1; cur1++, cur2++)
    {
      for (cur3 = extraenvs; cur3; cur3 = cur3->next)
        {
          if (((curstr = cur3->str) != NULL) &&
              (str_beginswith (*cur1,
                               strndup (curstr, strcspn (curstr, "=") + 1))
               != NULL))
            {
              *cur2 = curstr;
              // fprintf (stderr, "updated: %s\n", *cur2);
              cur3->str = NULL;
            }
          else
            {
              *cur2 = *cur1;
            }
        }
    }

  for (cur3 = extraenvs; cur3; cur3 = cur3->next)
    {
      if ((curstr = cur3->str) != NULL)
        {
          *cur2 = curstr;
          // fprintf (stderr, "added: %s\n", *cur2);
          cur2++;
        }
    }
  *cur2 = NULL;
  return envp_new;
}

char **
argv_dup_add (char **oldargv, struct str_list *prefixargv)
{
  int count_old, count;
  struct str_list *cur;
  for (count_old = 0; oldargv[count_old]; count_old++)
    {
    }
  count = count_old + str_list_count (prefixargv);
  char **newargv = (char **) malloc ((count + 1) * sizeof (char *));
  for (; count_old >= 0; count_old--, count--)
    {
      newargv[count] = oldargv[count_old];
    }
  for (cur = prefixargv; cur; count--, cur = cur->next)
    {
      newargv[count] = cur->str;
    }
  return newargv;
}

int
makeancesdirs (char *path)
{
  if (*path)
    {
      char *p, *q, *path_dup = strndup (path, PATH_MAX);
      for (p = path_dup + 1, q = strchr (p, '/'); q; q = strchr (p, '/'))
        {
          *q = 0;
          mkdir (path_dup, 0700);  // TODO: Fix hardcoded value.
          // TOOD: Make sure the mode 0777 or 01777 is OK for all platforms.
          *q = '/';
          p = q + 1;
        }
      free (path_dup);
      return 0;
    }  // Success.
  {
    return -1;
  }
}  // Path was empty failed.

void
sigaction_inst (int signum, void (*handler) (int))
{
  struct sigaction new_action, old_action;

  new_action.sa_handler = handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (signum, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (signum, &new_action, NULL);
}
