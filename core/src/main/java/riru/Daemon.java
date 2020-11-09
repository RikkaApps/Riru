package riru;

import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.util.Log;

import androidx.annotation.Keep;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import riru.core.BuildConfig;

/**
 * A "daemon" that resets native bridge prop and reboot on zygote dead.
 */
public class Daemon {

    private static final String TAG = "rirud";

    private static final String SERVICE_FOR_TEST = "activity";
    private static final String RIRU_LOADER = "libriruloader.so";

    private final Handler handler;
    private final String name;
    private final String originalNativeBridge;

    public Daemon(String name, String originalNativeBridge) {
        this.handler = new Handler(Looper.myLooper());
        this.name = name;
        this.originalNativeBridge = originalNativeBridge;

        handler.post(() -> startWait(false));
    }

    private void startWait(boolean allowRestart) {
        IBinder binder = waitForSystemService(name);

        if (!isRiruLoaded()) {
            Log.w(TAG, "Riru is not loaded.");

            if (allowRestart) {
                handler.post(() -> {
                    Log.w(TAG, "Restarting zygote...");
                    if (Build.SUPPORTED_64_BIT_ABIS.length > 0) {
                        SystemProperties.set("ctl.restart", "zygote_secondary");
                    } else {
                        SystemProperties.set("ctl.restart", "zygote");
                    }
                    startWait(false);
                });
            }
            return;
        }

        Log.i(TAG, "Riru loaded, reset native bridge to " + originalNativeBridge + "...");
        resetNativeBridgeProp(originalNativeBridge);

        try {
            binder.linkToDeath(() -> {
                Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
                resetNativeBridgeProp(RIRU_LOADER);
                handler.post(() -> startWait(true));
            }, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }
    }

    @Keep
    public static void main(String[] args) {
        if (BuildConfig.DEBUG) {
            System.exit(0);
            return;
        }

        String originalNativeBridge = readOriginalNativeBridge();
        Log.i(TAG, "readOriginalNativeBridge: " + originalNativeBridge);

        if (originalNativeBridge == null) {
            originalNativeBridge = "0";
        }

        Looper.prepare();
        new Daemon(SERVICE_FOR_TEST, originalNativeBridge);
        Looper.loop();
    }

    private static String readOriginalNativeBridge() {
        try {
            BufferedReader br = new BufferedReader(new FileReader(new File("/data/adb/riru/native_bridge")));
            char[] buf = new char[4096];
            int size;
            if ((size = br.read(buf)) > 0) {
                return new String(buf, 0, size);
            }
        } catch (IOException e) {
            Log.w(TAG, "Can't read native_bridge.", e);
        }
        return null;
    }

    private static void resetNativeBridgeProp(String value) {
        exec("resetprop", "ro.dalvik.vm.native.bridge", value);
    }

    private static void exec(String... command) {
        ProcessBuilder pb = new ProcessBuilder(command);
        try {
            Process process = pb.start();
            int code = process.waitFor();
            Log.i(TAG, "Exec " + command[0] + " exited with " + code);
        } catch (Throwable e) {
            Log.w(TAG, "Exec " + command[0], e);
        }
    }

    private static IBinder waitForSystemService(String name) {
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

    private static boolean isRiruLoaded() {
        String devRandom = null;
        try {
            BufferedReader br = new BufferedReader(new FileReader(new File("/data/adb/riru/dev_random")));
            char[] buf = new char[4096];
            int size;
            if ((size = br.read(buf)) > 0) {
                devRandom = new String(buf, 0, size);
            }
        } catch (IOException e) {
            Log.w(TAG, "Can't read dev_random.", e);
        }

        if (devRandom == null) {
            return false;
        }

        return new File("/dev/riru_" + devRandom).exists();
    }
}
