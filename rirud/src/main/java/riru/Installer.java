package riru;

import android.os.SELinux;

public class Installer {

    private static void checkSELinux() {
        if (!DaemonUtils.hasSELinux()) {
            System.out.println("- This device does not have SELinux");
            return;
        }
        if (!SELinux.isSELinuxEnabled()) {
            System.out.println("- SELinux is disabled");
            return;
        }
        if (!SELinux.isSELinuxEnforced()) {
            System.out.println("! SELinux is permissive");
            System.out.println("! If your ROM runs Android 10+ and has SELinux permissive (it will disable seccomp), ANY app can escalate itself to uid 0, this is extremely dangerous");
            System.out.println("- Just a kind reminder, continue installation...");
            return;
        }

        boolean file = SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom");
        boolean dir = SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom");

        if (file || dir) {
            System.out.println("!!!!!!!!! PLEASE READ !!!!!!!!!!");
            if (file) {
                System.out.println("! Your ROM allows init to relabel Magisk module files");
            }
            if (dir) {
                System.out.println("! Your ROM allows init to relabel Magisk module files");
            }
            System.out.println("- Riru will try to reset the context of modules files, but not guaranteed to always work");
        } else {
            System.out.println("- No problem found");
        }
    }

    public static void main(String[] args) {
        for (String arg : args) {
            if ("--check-selinux".equals(arg)) {
                System.out.println("- Start checks...");
                checkSELinux();
                System.exit(0);
                return;
            }
        }
    }
}
