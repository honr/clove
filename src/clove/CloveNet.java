package clove;

public class CloveNet {
  public static native int accept (int sockfd);
  public static native byte[] read (int fd, int maxlen);
  // public static native int write (int fd, byte[] data);
  public static native int close (int fd);
  public static native int fsync (int fd);

  public static native java.io.FileDescriptor[] unix_recv_fds  (int sockfd);
  public static native int sock_addr_bind (int socktype, byte[] sockpath, int force_bind);
  
  static { System.loadLibrary("CloveNet"); }
}
