package riru;

import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Build;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;

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
import java.lang.reflect.Method;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import static riru.Daemon.TAG;

public class DaemonUtils {

    private static Boolean has32Bit = null, has64Bit = null;
    private static String originalNativeBridge;
    private static String devRandom;
    private static int magiskVersionCode = -1;
    private static String magiskTmpfsPath;
    private static final boolean[] loaded = new boolean[2];

    public static Resources res;

    private static int lastStatusId = 0;
    private static Object[] lastStatusArgs = new Object[0];

    private static final String LIB_PREFIX = "lib";
    private static final String RIRU_PREFIX = "libriru_";
    private static final String SO_SUFFIX = ".so";

    private static final Map<String, List<Pair<String, String>>> modules = new HashMap<>();
    private static final Map<String, List<Pair<String, String>>> modules64 = new HashMap<>();

    @SuppressWarnings("unchecked")
    private static final List<String>[] loadedModules = new List[]{new ArrayList<>(), new ArrayList<>()};

    static {
        originalNativeBridge = SystemProperties.get("ro.dalvik.vm.native.bridge");

        if (TextUtils.isEmpty(originalNativeBridge)) {
            originalNativeBridge = "0";
        }

        try {
            AssetManager am = AssetManager.class.newInstance();
            Method addAssetPath = AssetManager.class.getDeclaredMethod("addAssetPath", String.class);
            addAssetPath.setAccessible(true);
            // TODO: may use classpath
            addAssetPath.invoke(am, new File("rirud.apk").getAbsolutePath());
            res = new Resources(am, null, null);
        } catch (Throwable e) {
            Log.e(TAG, "load res", e);
        }

        try {
            if (has64Bit()) {
                collectModules(true);
            }
        } catch (Throwable e) {
            Log.e(TAG, "collect modules 64", e);
        }

        try {
            if (has32Bit()) {
                collectModules(false);
            }
        } catch (Throwable e) {
            Log.e(TAG, "collect modules 32", e);
        }
    }

    public static void init(String[] args) {
        magiskVersionCode = Integer.parseInt(args[0]);
        magiskTmpfsPath = args[1];
    }

    public static boolean isLoaded(boolean is64Bit) {
        return loaded[is64Bit ? 1 : 0];
    }

    public static void setIsLoaded(boolean is64Bit, boolean riruIsLoaded) {
        DaemonUtils.loaded[is64Bit ? 1 : 0] = riruIsLoaded;
    }

    public static List<String> getLoadedModules(boolean is64Bit) {
        return loadedModules[is64Bit ? 1 : 0];
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

    // from AndroidRuntime.cpp
    private static String readLocale() {
        String locale = SystemProperties.get("persist.sys.locale", "");
        if (!locale.isEmpty()) {
            return locale;
        }

        String language = SystemProperties.get("persist.sys.language", "");
        if (!language.isEmpty()) {
            String country = SystemProperties.get("persist.sys.country", "");
            String variant = SystemProperties.get("persist.sys.localevar", "");

            String out = language;
            if (!country.isEmpty()) {
                out = out + "-" + country;
            }

            if (!variant.isEmpty()) {
                out = out + "-" + variant;
            }

            return out;
        }

        String productLocale = SystemProperties.get("ro.product.locale", "");
        if (!productLocale.isEmpty()) {
            return productLocale;
        }

        // If persist.sys.locale and ro.product.locale are missing,
        // construct a locale value from the individual locale components.
        String productLanguage = SystemProperties.get("ro.product.locale.language", "en");
        String productRegion = SystemProperties.get("ro.product.locale.region", "US");

        return productLanguage + "-" + productRegion;
    }

    public static void reloadLocale() {
        Locale locale = Locale.forLanguageTag(readLocale());
        Locale.setDefault(locale);
        Configuration conf = res.getConfiguration();
        conf.setLocale(Locale.forLanguageTag(readLocale()));
        res.updateConfiguration(conf, res.getDisplayMetrics());
        writeStatus(lastStatusId, lastStatusArgs);
    }

    public static void writeStatus(int id, Object... args) {
        File prop = new File("module.prop");
        lastStatusId = id;
        lastStatusArgs = args;
        try (BufferedReader br = new BufferedReader(new FileReader(prop))) {
            StringBuilder sb = new StringBuilder();
            String line = br.readLine();
            while (line != null) {
                sb.append(line.replaceFirst("^description=(\\[.*]\\s*)?", "description=[ " + res.getString(id, args) + " ] "));
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

    private static void collectModules(boolean is64) {
        Map<String, List<Pair<String, String>>> m = is64 ? modules64 : modules;

        String riruLibPath = "riru/" + (is64 ? "lib64" : "lib");
        File[] magiskDirs = new File(DaemonUtils.getMagiskTmpfsPath(), ".magisk/modules").listFiles();
        if (magiskDirs == null) {
            return;
        }

        for (File magiskDir : magiskDirs) {
            if (new File(magiskDir, "remove").exists()
                    || new File(magiskDir, "disable").exists())
                continue;

            File libDir = new File(magiskDir, riruLibPath);
            if (!libDir.exists())
                continue;

            File[] libsFiles = libDir.listFiles();
            if (libsFiles == null) {
                continue;
            }

            List<Pair<String, String>> libs = new ArrayList<>();
            m.put(magiskDir.getAbsolutePath(), libs);

            for (File lib : libsFiles) {
                String name = lib.getName();
                String id = name;
                if (id.startsWith(RIRU_PREFIX)) id = id.substring(RIRU_PREFIX.length());
                else if (id.startsWith(LIB_PREFIX)) id = id.substring(LIB_PREFIX.length());
                if (id.endsWith(SO_SUFFIX)) id = id.substring(0, id.length() - 3);
                id = magiskDir.getName() + "@" + id;
                if (!name.endsWith(SO_SUFFIX))
                    lib = new File("/system/" + (is64 ? "lib64" : "lib"), name + SO_SUFFIX);

                libs.add(new Pair<>(id, lib.getAbsolutePath()));
            }
        }
    }

    public static Map<String, List<Pair<String, String>>> getModules(boolean is64) {
        return is64 ? modules64 : modules;
    }
}
