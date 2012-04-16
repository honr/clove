#include "clove-common.h"

// For OSX
#ifndef _GNU_SOURCE

size_t
strnlen (const char *s, size_t len)
{
  size_t i;
  for (i = 0; i < len && *s; i++, s++);
  return i;
}

char *
strndup (char const *s, size_t n)
{
  size_t len = strnlen (s, n);	 // Or n-1 ?
  char *t;

  if ((t = malloc (len + 1)) == NULL)
    return NULL;

  t[len] = '\0';
  return memcpy (t, s, len);
}

#endif

struct sockaddr_gen
addr_unix (int type, const char *sockpath)
{
  struct sockaddr_gen a;
  struct sockaddr_un *addr =
    (struct sockaddr_un *) malloc (sizeof (struct sockaddr_un));
  // printf ("path: %s\n", sockpath);
  addr->sun_family = AF_UNIX;

  strcpy (addr->sun_path, sockpath);
  if (sockpath[0] == '@')  // We assume an abstract path was intended.
    {
      addr->sun_path[0] = 0;
      /* We consider the null byte at the end of sockpath NOT part
         of the name. */
      a.len = strlen (sockpath) + sizeof (addr->sun_family);
    }
  else
    {
      a.len = strlen (sockpath) + 1 + sizeof (addr->sun_family);
    }

  a.addr = (struct sockaddr *) addr;
  a.domain = AF_UNIX;
  a.type = type;
  a.protocol = 0;
  return a;
}

int
sock_bind (struct sockaddr_gen a, int force_bind)
{
  int sock;

  if ((sock = socket (a.domain, a.type, a.protocol)) < 0)
    {
      perror ("socket");
      exit (2);
    }

  if (bind (sock, a.addr, a.len) != 0)
    {
      if (force_bind)
	{
	  perror ("bind");
	  exit (2);
	}
      else
	{ // perror ("bind");
	  exit (0);
	}
    }

  if (a.type & SOCK_STREAM)
    {
      if (listen (sock, LISTEN_BACKLOG) != 0)
	{
	  perror ("listen");
	  exit (2);
	}
    }

  return sock;
}

int
sock_connect (struct sockaddr_gen a)
{
  int sock;

  if ((sock = socket (a.domain, a.type, a.protocol)) < 0)
    {
      perror ("socket");
      // exit(2);
      return sock;
    }

  if (a.type & SOCK_STREAM)
    {
      if (connect (sock, a.addr, a.len) != 0)
	{ // perror ("connect");
	  // TODO: throw exception.
	  close (sock);
	  return -1;
	  // exit(2);
	}
    }

  return sock;
}

int
sock_addr_bind (int type, char *sockpath, int force_bind)
{
  struct sockaddr_gen a = addr_unix (type, sockpath);
  return sock_bind (a, force_bind);
}

int
sock_addr_connect (int type, char *sockpath)
{
  struct sockaddr_gen a = addr_unix (type, sockpath);
  return sock_connect (a);
}

// TODO: Pass credentials. The receiving end should simply abort 
// communications if the client's uid is not the same as the service's.

int
unix_sendmsgf (int sock, void *buf, int buf_len,
	       int *fds, int num_fds, int flags)
{
  struct iovec iov1 = {.iov_base = buf,.iov_len = buf_len };
  int cmsg_buf_size = CMSG_SPACE (sizeof (int) * num_fds);
  char cmsg_buf[cmsg_buf_size];

  struct msghdr msgh = {.msg_name = NULL,
    .msg_namelen = 0,
    .msg_iov = &iov1,
    .msg_iovlen = 1,
    .msg_control = cmsg_buf,
    .msg_controllen = cmsg_buf_size,
    .msg_flags = 0
  };

  struct cmsghdr *cmsg = CMSG_FIRSTHDR (&msgh);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN (sizeof (int) * num_fds);
  memcpy (CMSG_DATA (cmsg), fds, sizeof (int) * num_fds);
  msgh.msg_controllen = cmsg->cmsg_len;
  return sendmsg (sock, &msgh, flags);	
  // TODO: Free msgh, cmsg_buf.
}

int
unix_recvmsgf (int sock, void *buf, int buf_len,
	       int *fds, int *p_num_fds, int flags)
{
  struct iovec iov1 = {.iov_base = buf,.iov_len = buf_len };
  int cmsg_buf_size = CMSG_SPACE (sizeof (int) * (*p_num_fds));
  char cmsg_buf[cmsg_buf_size];

  struct msghdr msgh = {.msg_name = NULL,
    .msg_namelen = 0,
    .msg_iov = &iov1,
    .msg_iovlen = 1,
    .msg_control = cmsg_buf,
    .msg_controllen = cmsg_buf_size,
    .msg_flags = 0
  };

  int ret;
  if ((ret = recvmsg (sock, &msgh, flags)) > 0)
    {
      struct cmsghdr *cmsg;
      for (cmsg = CMSG_FIRSTHDR (&msgh);
	   cmsg; cmsg = CMSG_NXTHDR (&msgh, cmsg))
	{
	  if ((SOL_SOCKET == cmsg->cmsg_level) &&
	      (SCM_RIGHTS == cmsg->cmsg_type))
	    {
	      size_t fds_size = cmsg->cmsg_len - CMSG_LEN (0);	// this might be wrong
	      if (fds_size > sizeof (int) * (*p_num_fds))
		{
		  fprintf (stderr,
			   "tried to send a large number of items in cmsg. abort.\n");
		  exit (1);
		}
	      memcpy (fds, CMSG_DATA (cmsg), fds_size);
	      *p_num_fds = fds_size / sizeof (int);
	      break;
	    }
	}
    }
  else
    {
      fprintf (stderr, "errno: %d\n", errno);
      perror ("recvmsg");
    }

  return ret;
}
