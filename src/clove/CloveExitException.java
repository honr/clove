package clove;

public class CloveExitException extends SecurityException
{
  private int status;

  public CloveExitException(int status)
  {
    super("exit-exception: status " + status);
    this.status = status;
  }

  public CloveExitException(String msg, int status)
  {
    super(msg);
    this.status = status;
  }

  public int getStatus()
  {
    return status;
  }
}
