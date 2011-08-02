package clove;

import  java.security.Permission;
import  java.io.PrintStream;

/**
 * Security manager which does nothing other than trap
 * checkExit, or delegate all non-deprecated methods to
 * a base manager.
 */
public class CloveSecurityManager extends SecurityManager
{
  private static final ThreadLocal EXIT = new InheritableThreadLocal ();
  final SecurityManager base;

  public CloveSecurityManager (SecurityManager base)
  {
    this.base = base;
  }

  public void checkExit (int status)
  {
    if (base != null)
      {
	base.checkExit (status);
      }

    final PrintStream exit = (PrintStream) EXIT.get();

    if (exit != null)
      {
	exit.println (status);
      }

    throw new CloveExitException (status);
  }

  public void checkPermission (Permission perm)
  {
    if (base != null)
      {
	base.checkPermission (perm);
      }
  }

  public void checkPermission (Permission perm, Object context)
  {
    if (base != null)
      {
	base.checkPermission (perm, context);
      }
  }

  public static void setExit (PrintStream exit)
  {
    EXIT.set (exit);
  }
}
