#include "clove_CloveNet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
// #include <error.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <fcntl.h>

struct remote_fds {
  int in;
  int out;
  int err;
};

#define LISTEN_BACKLOG 64

struct sockaddr_gen {
  int domain;
  int type;
  int protocol;
  struct sockaddr* addr;
  socklen_t len;
};

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
    int cmsg_buf_size = 128; // CMSG_SPACE(sizeof(iofds)) // TODO: fix hardcoded size
    char cmsg_buf[cmsg_buf_size];

    char msg[128]; // TODO: fix hardcoded size
    iov.iov_base = msg;
    iov.iov_len  = 7;  // "message" // TODO: fix hardcoded size

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
    /* else
       { fprintf (stderr, "errno: %d\n", errno);
         perror ("recvmsg"); } */

    return ret; }



////////////////////////////////////////////////////////////

/*
 * Class:     CloveNet
 * Method:    accept
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_accept
  (JNIEnv *env, jclass cls, jint sock)
  { return accept (sock, NULL, NULL); }

/*
 * Class:     CloveNet
 * Method:    read
 * Signature: (II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_clove_CloveNet_read
  (JNIEnv *env, jclass cls, jint fd, jint count)
  { jbyte* buf = malloc (count);
    int len;
    if ((len = read (fd, buf, count)) <= 0)
      { perror ("read"); 
	return NULL;
	// what to do here?
      }
    else
      { jbyteArray rawDataCopy = (*env)->NewByteArray (env, len);
	(*env)->SetByteArrayRegion (env, rawDataCopy, 0, len, buf);
	return rawDataCopy; }} 

/*
 * Class:     CloveNet
 * Method:    close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_close
  (JNIEnv *env, jclass cls, jint fd)
  { return close (fd); }

/*
 * Class:     clove_CloveNet
 * Method:    fsync
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_fsync
  (JNIEnv *env, jclass cls, jint fd)
  { return fsync (fd); }

/*
 * Class:     clove_CloveNet
 * Method:    unix_recv_fds
 * Signature: (I)[Ljava/io/FileDescriptor;
 */
JNIEXPORT jobjectArray JNICALL Java_clove_CloveNet_unix_1recv_1fds
(JNIEnv *env, jclass cls, jint sock)
//unix_recv_fds (int sock, struct remote_fds* iofds_p);
  { struct remote_fds fds;
    int len;
    if ((len = unix_recv_fds (sock, &fds)) <= 0)
      { // perror ("unix_recv_fds");
	// what to do here? returning NULL is probably enough.
	return NULL; }
    else
      { jfieldID field_fd;
	jmethodID const_fdesc;
	jclass class_fdesc;

	if ((class_fdesc = (*env)->FindClass 
	     (env, "java/io/FileDescriptor")) == NULL)
	  { return NULL; }

	if ((field_fd = (*env)->GetFieldID 
	     (env, class_fdesc, "fd", "I")) == NULL) 
	  { return NULL; }

	if ((const_fdesc = (*env)->GetMethodID 
	     (env, class_fdesc, "<init>", "()V")) == NULL)
	  { return NULL; }
	
	jobjectArray ret = (*env)->NewObjectArray (env, 3, class_fdesc, NULL);
	int i;
	for (i = 0; i < 3; i ++)
	  { // construct a new FileDescriptor
	    jobject ret0 = (*env)->NewObject (env, class_fdesc, const_fdesc);
	    // poke the "fd" field with the file descriptor
	    (*env)->SetIntField(env, ret0, field_fd, ((int *) &fds)[i]);
	    (*env)->SetObjectArrayElement (env, ret, i, ret0); }
	return ret; }}

/*
 * Class:     CloveNet
 * Method:    sock_addr_bind
 * Signature: (I[BI)[I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_sock_1addr_1bind
  (JNIEnv *env, jclass cls, jint socktype, jbyteArray sockpath, jint force_bind)
  { int buf_len = (*env)->GetArrayLength(env, sockpath) + 1;
    jbyte buf[buf_len];
    (*env)->GetByteArrayRegion (env, sockpath, 0, buf_len - 1, buf);
    buf[buf_len - 1] = 0;
    return sock_addr_bind (socktype, (char*) buf, force_bind); }
/*  is it correct to cast jbyte to char? */
