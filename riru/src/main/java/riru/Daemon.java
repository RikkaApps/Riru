package riru;

import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.util.Log;

import androidx.annotation.Keep;

import java.io.File;

import riru.core.BuildConfig;

/**
 * A "daemon" that controls native bridge prop and "rirud" socket.
 */
public class Daemon {

    public static final String TAG = "rirud_java";

    private static final String SERVICE_FOR_TEST = "activity";
    private static final String RIRU_LOADER = "libriruloader.so";

    private final Handler handler;
    private final String name;
    private final String originalNativeBridge;
    private final int rirudPid;

    public Daemon(String name, String originalNativeBridge, int rirudPid) {
        this.handler = new Handler(Looper.myLooper());
        this.name = name;
        this.originalNativeBridge = originalNativeBridge;
        this.rirudPid = rirudPid;

        handler.post(() -> startWait(false));
    }

    private void startWait(boolean allowRestart) {
        IBinder binder = DaemonUtils.waitForSystemService(name);

        if (!DaemonUtils.isRiruLoaded()) {
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
        DaemonUtils.resetNativeBridgeProp(originalNativeBridge);

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        DaemonUtils.stopSocket(rirudPid);

        try {
            binder.linkToDeath(() -> {
                Log.i(TAG, "Zygote is probably dead, restart rirud socket...");
                DaemonUtils.startSocket(rirudPid);

                Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
                DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

                handler.post(() -> startWait(true));
            }, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }
    }

    @Keep
    public static void main(String[] args) {
        if (!new File("/data/adb/riru/enable_hide").exists() && !BuildConfig.DEBUG) {
            Log.i(TAG, "Hide is not enabled.");
            System.exit(0);
            return;
        }

        String originalNativeBridge = DaemonUtils.readOriginalNativeBridge();
        Log.i(TAG, "readOriginalNativeBridge: " + originalNativeBridge);

        if (originalNativeBridge == null) {
            originalNativeBridge = "0";
        }

        int rirudPid = DaemonUtils.findNativeDaemonPid();
        Log.i(TAG, "rirud: " + rirudPid);

        Looper.prepare();
        new Daemon(SERVICE_FOR_TEST, originalNativeBridge, rirudPid);
        Looper.loop();
    }
}
