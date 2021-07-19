package riru;

import android.os.SELinux;

public class Installer {

    private static void checkSELinux() {
        if (DaemonUtils.hasSELinux() && SELinux.isSELinuxEnabled() && SELinux.isSELinuxEnforced()
                && (SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom")
                || SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom"))) {
            System.exit(1);
        }
    }

    public static void main(String[] args) {
        for (String arg : args) {
            if ("--check-selinux".equals(arg)) {
                checkSELinux();
                return;
            }
        }
    }
}
