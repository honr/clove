package clove;

public class CloveNet
{
  public static class Message
  {
    public java.io.FileDescriptor[] fds;
    public byte[] buf;
    public int retval;
  }
  public static native int accept (int sockfd);
  public static native byte[] read (int fd, int maxlen);
  // public static native int write (int fd, byte[] data);
  public static native int close (int fd);
  public static native int fsync (int fd);

  public static native int sock_addr_bind (int socktype, byte[] sockpath, int force_bind);
  public static native Message unix_recvmsgf (int sockfd, int max_num_fds, int flags);
  public static native int unix_sendmsgf (int sockfd, Message msg, int flags);

  static
  {
    System.loadLibrary("CloveNet");
  }
}
