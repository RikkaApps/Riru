package android.os;

public class SELinux {

    public static final native boolean isSELinuxEnabled();

    public static final native boolean isSELinuxEnforced();

    public static final native boolean checkSELinuxAccess(String scon, String tcon, String tclass, String perm);

    public static final native String getFileContext(String path);

    public static final native boolean setFileContext(String path, String context);

    public static final native String getPidContext(int pid);
}
