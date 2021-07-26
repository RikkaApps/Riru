package android.os;

public class SELinux {

    public static final native boolean isSELinuxEnabled();

    public static final native boolean isSELinuxEnforced();

    public static final native boolean checkSELinuxAccess(String scon, String tcon, String tclass, String perm);

    public static final native String getFileContext(String path);
}
