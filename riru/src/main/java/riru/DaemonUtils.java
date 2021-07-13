package riru;

import static riru.Daemon.TAG;

import android.os.Build;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.InterruptedIOException;
import java.security.SecureRandom;
import java.util.Locale;

public class DaemonUtils {

    private static Boolean has32Bit = null, has64Bit = null;
    private static String originalNativeBridge;
    private static String devRandom;
    private static int magiskVersionCode = -1;
    private static String magiskTmpfsPath;

    static {
        originalNativeBridge = SystemProperties.get("ro.dalvik.vm.native.bridge");

        if (TextUtils.isEmpty(originalNativeBridge)) {
            originalNativeBridge = "0";
        }
    }

    public static void init(String[] args) {
        magiskVersionCode = Integer.parseInt(args[0]);
        magiskTmpfsPath = args[1];
    }

    public static void killParentProcess() {
        int ppid = Os.getppid();
        try {
            Os.kill(ppid, OsConstants.SIGKILL);
            Log.i(TAG, "Killed parent process " + ppid);
        } catch (ErrnoException e) {
            Log.e(TAG, "Failed kill parent process " + ppid);
        }
    }

    public static void writeStatus(String status) {
        File prop = new File("module.prop");
        try (BufferedReader br = new BufferedReader(new FileReader(prop))) {
            StringBuilder sb = new StringBuilder();
            String line = br.readLine();
            while (line != null) {
                sb.append(line.replaceFirst("^description=(\\[.*]\\s*)?", "description=[" + status + "] "));
                sb.append(System.lineSeparator());
                line = br.readLine();
            }
            String content = sb.toString();
            try (DataOutputStream os = new DataOutputStream(new FileOutputStream(prop, false))) {
                os.write(content.getBytes());
            }
        } catch (Throwable e) {
            Log.e(TAG, "fail to write status", e);
        }
    }

    public static boolean has32Bit() {
        if (has32Bit == null) {
            has32Bit = Build.SUPPORTED_32_BIT_ABIS.length > 0;
        }
        return has32Bit;
    }

    public static boolean has64Bit() {
        if (has64Bit == null) {
            has64Bit = Build.SUPPORTED_64_BIT_ABIS.length > 0;
        }
        return has64Bit;
    }

    public static String getOriginalNativeBridge() {
        return originalNativeBridge;
    }

    public static void resetNativeBridgeProp(String value) {
        resetProperty("ro.dalvik.vm.native.bridge", value);
    }

    public static void resetProperty(String key, String value) {
        exec("resetprop", key, value);
    }

    public static void exec(String... command) {
        ProcessBuilder pb = new ProcessBuilder(command);
        try {
            Process process = pb.start();
            int code = process.waitFor();
            Log.i(TAG, "Exec " + command[0] + " exited with " + code);
        } catch (Throwable e) {
            Log.w(TAG, "Exec " + command[0], e);
        }
    }

    public static IBinder waitForSystemService(String name) {
        IBinder binder;
        do {
            binder = ServiceManager.getService(name);
            if (binder != null && binder.pingBinder()) {
                return binder;
            }

            Log.i(TAG, "Service " + name + " not found, wait 1s...");
            try {
                //noinspection BusyWait
                Thread.sleep(1000);
            } catch (InterruptedException ignored) {
            }
        } while (true);
    }

    public static File getRiruDevFile() {
        String devRandom = getDevRandom();
        if (devRandom == null) {
            return null;
        }

        if (has64Bit()) {
            return new File("/dev/riru64_" + devRandom);
        } else {
            return new File("/dev/riru_" + devRandom);
        }
    }

    public static boolean isRiruLoaded() {
        File file = getRiruDevFile();
        return file != null && file.exists();
    }

    private static boolean deleteDir(File file) {
        boolean res = true;
        File[] files = file.listFiles();
        if (files != null) {
            for (File f : files) {
                res &= deleteDir(f);
            }
        }
        return res & file.delete();
    }

    public static void deleteDevFolder() {
        String devRandom = getDevRandom();
        if (devRandom == null) {
            return;
        }

        File file;

        file = new File("/dev/riru_" + devRandom);
        Log.i(TAG, "Attempt to delete " + file + "...");
        if (!deleteDir(file)) {
            file.renameTo(new File("/dev/riru_" + devRandom + "_" + System.currentTimeMillis()));
        } else {
            Log.i(TAG, "Deleted " + file);
        }

        file = new File("/dev/riru64_" + devRandom);
        Log.i(TAG, "Attempt to delete " + file + "...");
        if (!deleteDir(file)) {
            file.renameTo(new File("/dev/riru_" + devRandom + "_" + System.currentTimeMillis()));
        } else {
            Log.i(TAG, "Deleted " + file + ".");
        }
    }


    public static int getMagiskVersionCode() {
        if (magiskVersionCode != -1) {
            return magiskVersionCode;
        }

        try {
            ProcessBuilder ps = new ProcessBuilder("magisk", "-V");
            ps.redirectErrorStream(true);
            Process pr = ps.start();

            BufferedReader in = new BufferedReader(new InputStreamReader(pr.getInputStream()));
            String line = in.readLine();
            Log.i(TAG, "Exec magisk -V: " + line);
            magiskVersionCode = Integer.parseInt(line);
            pr.waitFor();
            in.close();
            return magiskVersionCode;
        } catch (Throwable e) {
            Log.w(TAG, "Exec magisk -V", e);
            return -1;
        }
    }

    public static String getMagiskTmpfsPath() {
        if (magiskTmpfsPath != null) {
            return magiskTmpfsPath;
        }

        if (getMagiskVersionCode() < 21000) {
            return "/sbin";
        }

        try {
            ProcessBuilder ps = new ProcessBuilder("magisk", "--path");
            ps.redirectErrorStream(true);
            Process pr = ps.start();

            BufferedReader in = new BufferedReader(new InputStreamReader(pr.getInputStream()));
            magiskTmpfsPath = in.readLine();
            Log.i(TAG, "Exec magisk --path: " + magiskTmpfsPath);
            pr.waitFor();
            in.close();
            return magiskTmpfsPath;
        } catch (Throwable e) {
            Log.w(TAG, "Exec magisk --path", e);
            return "";
        }
    }

    public static boolean hasSELinux() {
        return new File("/system/lib/libselinux.so").exists()
                || new File("/system/lib64/libselinux.so").exists();
    }

    public static boolean setSocketCreateContext(String context) {
        FileDescriptor fd = null;
        try {
            fd = Os.open("/proc/thread-self/attr/sockcreate", OsConstants.O_RDWR, 0);
        } catch (ErrnoException e) {
            if (e.errno == OsConstants.ENOENT) {
                int tid = Os.gettid();
                try {
                    fd = Os.open(String.format(Locale.ENGLISH, "/proc/self/task/%d/attr/sockcreate", tid), OsConstants.O_RDWR, 0);
                } catch (ErrnoException ignored) {
                }
            }
        }

        if (fd == null) {
            return false;
        }

        byte[] bytes;
        int length;
        int remaining;
        if (!TextUtils.isEmpty(context)) {
            byte[] stringBytes = context.getBytes();
            bytes = new byte[stringBytes.length + 1];
            System.arraycopy(stringBytes, 0, bytes, 0, stringBytes.length);
            bytes[stringBytes.length] = '\0';

            length = bytes.length;
            remaining = bytes.length;
        } else {
            bytes = null;
            length = 0;
            remaining = 0;
        }

        do {
            try {
                remaining -= Os.write(fd, bytes, length - remaining, remaining);
                if (remaining <= 0) {
                    break;
                }
            } catch (ErrnoException e) {
                break;
            } catch (InterruptedIOException e) {
                remaining -= e.bytesTransferred;
            }
        } while (true);

        try {
            Os.close(fd);
        } catch (ErrnoException e) {
            e.printStackTrace();
        }
        return true;
    }

    public static String getDevRandom() {
        if (devRandom != null) {
            return devRandom;
        }

        File dir = new File("/data/adb/riru");
        if (!dir.exists()) {
            //noinspection ResultOfMethodCallIgnored
            dir.mkdir();
        }

        File file = new File(dir, "dev_random");

        if (file.exists()) {
            try (FileInputStream in = new FileInputStream(file)) {
                byte[] buffer = new byte[8192];
                if (in.read(buffer, 0, 8192) > 0) {
                    devRandom = new String(buffer).trim();
                    Log.i(TAG, "Read dev random " + devRandom);
                    return devRandom;
                }
            } catch (IOException e) {
                Log.w(TAG, "Read dev random", e);
            }
        }

        String charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        SecureRandom rnd = new SecureRandom();
        StringBuilder sb = new StringBuilder(7);
        for (int i = 0; i < 7; i++) {
            sb.append(charset.charAt(rnd.nextInt(charset.length())));
        }

        devRandom = sb.toString();
        Log.i(TAG, "Generated dev random " + devRandom);

        try (FileOutputStream out = new FileOutputStream(file)) {
            out.write(devRandom.getBytes());
        } catch (IOException e) {
            Log.w(TAG, "Write dev random", e);
        }

        return devRandom;
    }
}
