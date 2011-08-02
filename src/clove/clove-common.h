#ifndef CLOVE_COMMON_H
#define CLOVE_COMMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <fcntl.h>

#define true 1
#define false 0
#define LISTEN_BACKLOG 64
#define BROKER_MESSAGE_LENGTH 8192

#ifndef _GNU_SOURCE
size_t strnlen (const char *s, size_t len);

char *strndup (char const *s, size_t n);
#endif // _GNU_SOURCE

struct sockaddr_gen
{
  int domain;
  int type;
  int protocol;
  struct sockaddr *addr;
  socklen_t len;
};

struct sockaddr_gen addr_unix (int type, const char *sockpath);

// int sock_bind (struct sockaddr_gen a, int force_bind);

// int sock_connect (struct sockaddr_gen a);

int sock_addr_bind (int type, char *sockpath, int force_bind);

int sock_addr_connect (int type, char *sockpath);

int
unix_sendmsgf (int sock, void *buf, int buf_len, int *fds,
	       int num_fds, int flags);

int
unix_recvmsgf (int sock, void *buf, int buf_len,
	       int *fds, int *p_num_fds, int flags);

#endif
