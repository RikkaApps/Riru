package riru;

import android.os.IBinder;
import android.os.ServiceManager;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Objects;

public class DaemonUtils {

    public static String readOriginalNativeBridge() {
        try (BufferedReader br = new BufferedReader(new FileReader(new File("/data/adb/riru/native_bridge")))) {
            char[] buf = new char[4096];
            int size;
            if ((size = br.read(buf)) > 0) {
                return new String(buf, 0, size);
            }
        } catch (IOException e) {
            Log.w(Daemon.TAG, "Can't read native_bridge.", e);
        }
        return null;
    }

    public static void resetNativeBridgeProp(String value) {
        exec("resetprop", "ro.dalvik.vm.native.bridge", value);
    }

    public static void exec(String... command) {
        ProcessBuilder pb = new ProcessBuilder(command);
        try {
            Process process = pb.start();
            int code = process.waitFor();
            Log.i(Daemon.TAG, "Exec " + command[0] + " exited with " + code);
        } catch (Throwable e) {
            Log.w(Daemon.TAG, "Exec " + command[0], e);
        }
    }

    public static IBinder waitForSystemService(String name) {
        IBinder binder;
        do {
            binder = ServiceManager.getService(name);
            if (binder != null && binder.pingBinder()) {
                return binder;
            }

            Log.i(Daemon.TAG, "Service " + name + " not found, wait 1s...");
            try {
                //noinspection BusyWait
                Thread.sleep(1000);
            } catch (InterruptedException ignored) {
            }
        } while (true);
    }

    public static boolean isRiruLoaded() {
        String devRandom = null;
        try (BufferedReader br = new BufferedReader(new FileReader(new File("/data/adb/riru/dev_random")))) {
            char[] buf = new char[4096];
            int size;
            if ((size = br.read(buf)) > 0) {
                devRandom = new String(buf, 0, size);
            }
        } catch (IOException e) {
            Log.w(Daemon.TAG, "Can't read dev_random.", e);
        }

        if (devRandom == null) {
            return false;
        }

        return new File("/dev/riru_" + devRandom).exists();
    }

    public static int findNativeDaemonPid() {
        File proc = new File("/proc");

        String[] names = proc.list();
        if (names == null) return -1;

        for (String name : names) {
            if (!TextUtils.isDigitsOnly(name)) continue;

            try (BufferedReader br = new BufferedReader(new FileReader(new File(String.format("/proc/%s/cmdline", name))))) {
                String[] args = br.readLine().split("\0");
                if (args != null && args.length >= 1
                        && (Objects.equals("rirud", args[0]) || Objects.equals("/data/adb/riru/bin/rirud", args[0]))) {
                    Log.i(Daemon.TAG, "Found rirud " + name);
                    return Integer.parseInt(name);
                }
            } catch (Throwable ignored) {
                try (BufferedReader br = new BufferedReader(new FileReader(new File(String.format("/proc/%s/comm", name))))) {
                    String[] args = br.readLine().split("\0");
                    if (args != null && args.length >= 1
                            && (Objects.equals("rirud", args[0]) || Objects.equals("/data/adb/riru/bin/rirud", args[0]))) {
                        Log.i(Daemon.TAG, "Found rirud " + name);
                        return Integer.parseInt(name);
                    }
                } catch (Throwable ignored2) {
                }
            }
        }

        Log.w(Daemon.TAG, "Can't find rirud.");
        return -1;
    }

    public static void startSocket(int pid) {
        if (pid != -1) {
            try {
                Os.kill(pid, OsConstants.SIGUSR2);
            } catch (ErrnoException e) {
                Log.w(Daemon.TAG, Log.getStackTraceString(e));
            }
        }
    }

    public static void stopSocket(int pid) {
        if (pid != -1) {
            try {
                Os.kill(pid, OsConstants.SIGUSR1);
            } catch (ErrnoException e) {
                Log.w(Daemon.TAG, Log.getStackTraceString(e));
            }
        }
    }
}
