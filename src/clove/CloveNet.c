#include "clove_CloveNet.h"
#include "clove-common.h"

/*
 * Class:     CloveNet
 * Method:    accept
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_accept
(JNIEnv * env, jclass cls, jint sock)
{
  return accept (sock, NULL, NULL);
}

/*
 * Class:     CloveNet
 * Method:    read
 * Signature: (II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_clove_CloveNet_read
(JNIEnv * env, jclass cls, jint fd, jint count)
{
  jbyte *buf = malloc (count);
  int len;
  if ((len = read (fd, buf, count)) <= 0)
    {
      perror ("read");
      return NULL;
      // what to do here?
    }
  else
    {
      jbyteArray rawDataCopy = (*env)->NewByteArray (env, len);
      (*env)->SetByteArrayRegion (env, rawDataCopy, 0, len, buf);
      return rawDataCopy;
    }
}

/*
 * Class:     CloveNet
 * Method:    close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_close
(JNIEnv * env, jclass cls, jint fd)
{
  return close (fd);
}

/*
 * Class:     clove_CloveNet
 * Method:    fsync
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_fsync
(JNIEnv * env, jclass cls, jint fd)
{
  return fsync (fd);
}

/*
 * Class:     clove_CloveNet
 * Method:    unix_recvmsgf
 * Signature: (II)Lclove/CloveNet/Message;
 */
JNIEXPORT jobject JNICALL Java_clove_CloveNet_unix_1recvmsgf
(JNIEnv * env, jclass cls, jint sock, jint num_fds, jint flags)
{
  int fds[num_fds];
  int buf_len = BROKER_MESSAGE_LENGTH;	// max message size

  jclass clove_CloveNet_Message_CLASS;
  jfieldID clove_CloveNet_Message_fds_FIELD;
  jfieldID clove_CloveNet_Message_buf_FIELD;
  jfieldID clove_CloveNet_Message_retval_FIELD;
  jmethodID clove_CloveNet_Message_init_METHOD;
  jclass java_io_FileDescriptor_CLASS;
  jfieldID java_io_FileDescriptor_fd_FIELD;
  jmethodID java_io_FileDescriptor_init_METHOD;

  if (!((clove_CloveNet_Message_CLASS = (*env)->FindClass
	 (env, "clove/CloveNet$Message")) &&
	(clove_CloveNet_Message_fds_FIELD = (*env)->GetFieldID
	 (env, clove_CloveNet_Message_CLASS, "fds",
	  "[Ljava/io/FileDescriptor;"))
	&& (clove_CloveNet_Message_buf_FIELD =
	    (*env)->GetFieldID (env, clove_CloveNet_Message_CLASS, "buf",
				"[B"))
	&& (clove_CloveNet_Message_retval_FIELD =
	    (*env)->GetFieldID (env, clove_CloveNet_Message_CLASS, "retval",
				"I"))
	&& (clove_CloveNet_Message_init_METHOD =
	    (*env)->GetMethodID (env, clove_CloveNet_Message_CLASS, "<init>",
				 "()V"))
	&& (java_io_FileDescriptor_CLASS =
	    (*env)->FindClass (env, "java/io/FileDescriptor"))
	&& (java_io_FileDescriptor_fd_FIELD =
	    (*env)->GetFieldID (env, java_io_FileDescriptor_CLASS, "fd", "I"))
	&& (java_io_FileDescriptor_init_METHOD =
	    (*env)->GetMethodID (env, java_io_FileDescriptor_CLASS, "<init>",
				 "()V"))))
    {
      return NULL;
    }

  jobject obj = (*env)->NewObject (env, clove_CloveNet_Message_CLASS,
				   clove_CloveNet_Message_init_METHOD);
  jobjectArray fds_obj =
    (*env)->NewObjectArray (env, num_fds, java_io_FileDescriptor_CLASS, NULL);
  jbyteArray buf_obj = (*env)->NewByteArray (env, buf_len);
  (*env)->SetObjectField (env, obj, clove_CloveNet_Message_fds_FIELD,
			  fds_obj);
  (*env)->SetObjectField (env, obj, clove_CloveNet_Message_buf_FIELD,
			  buf_obj);
  jbyte *buf = (*env)->GetByteArrayElements (env, buf_obj, NULL);
  int len = unix_recvmsgf (sock, buf, buf_len, fds, &num_fds, flags);
  (*env)->ReleaseByteArrayElements (env, buf_obj, buf, 0);
  if (len <= 0)
    {				// perror ("unix_recvmsgf");
      // what to do here? returning NULL is probably enough.
      return NULL;
    }
  else
    {
      (*env)->SetIntField (env, obj, clove_CloveNet_Message_retval_FIELD,
			   len);

      int i;
      for (i = 0; i < num_fds; i++)
	{			// construct a new FileDescriptor
	  jobject fd_obj =
	    (*env)->NewObject (env, java_io_FileDescriptor_CLASS,
			       java_io_FileDescriptor_init_METHOD);
	  // poke the "fd" field with the file descriptor
	  (*env)->SetIntField (env, fd_obj, java_io_FileDescriptor_fd_FIELD,
			       fds[i]);
	  (*env)->SetObjectArrayElement (env, fds_obj, i, fd_obj);
	}

      return obj;
    }
}

/* /\* */
/*  * Class:     clove_CloveNet */
/*  * Method:    unix_sendmsgf */
/*  * Signature: (ILclove/CloveNet/Message;I)[Ljava/io/FileDescriptor; */
/*  *\/ */
/* JNIEXPORT jobjectArray JNICALL Java_clove_CloveNet_unix_1sendmsgf */
/*   (JNIEnv *, jclass, jint, jobject, jint); */


/*
 * Class:     CloveNet
 * Method:    sock_addr_bind
 * Signature: (I[BI)[I
 */
JNIEXPORT jint JNICALL Java_clove_CloveNet_sock_1addr_1bind
(JNIEnv * env, jclass cls, jint socktype, jbyteArray sockpath,
 jint force_bind)
{
  int buf_len = (*env)->GetArrayLength (env, sockpath) + 1;
  jbyte buf[buf_len];
  (*env)->GetByteArrayRegion (env, sockpath, 0, buf_len - 1, buf);
  buf[buf_len - 1] = 0;
  return sock_addr_bind (socktype, (char *) buf, force_bind);
}

/*  is it correct to cast jbyte to char? */
