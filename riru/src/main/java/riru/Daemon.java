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
    private final boolean enableHide;

    public Daemon(String name, String originalNativeBridge, boolean enableHide) {
        this.handler = new Handler(Looper.myLooper());
        this.name = name;
        this.originalNativeBridge = originalNativeBridge;
        this.enableHide = enableHide;

        handler.post(() -> startWait(true, true));
    }

    private void startWait(boolean allowRestart, boolean isFirst) {
        IBinder binder = DaemonUtils.waitForSystemService(name);

        if (!DaemonUtils.isRiruLoaded()) {
            Log.w(TAG, "Riru is not loaded.");

            if (isFirst) {
                Log.w(TAG, "https://github.com/RikkaApps/Riru/issues/154#issuecomment-739128851");
            }

            if (allowRestart) {
                handler.post(() -> {
                    Log.w(TAG, "Restarting zygote...");
                    if (Build.SUPPORTED_64_BIT_ABIS.length > 0) {
                        SystemProperties.set("ctl.restart", "zygote_secondary");
                    } else {
                        SystemProperties.set("ctl.restart", "zygote");
                    }
                    startWait(false, false);
                });
            }
            return;
        }

        if (!enableHide) {
            Log.i(TAG, "Exit because Riru is loaded and hide is disabled.");
            System.exit(0);
            return;
        }

        Log.i(TAG, "Riru loaded, reset native bridge to " + originalNativeBridge + "...");
        DaemonUtils.resetNativeBridgeProp(originalNativeBridge);

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        DaemonUtils.stopSocket(DaemonUtils.findNativeDaemonPid());

        try {
            binder.linkToDeath(() -> {
                DaemonUtils.deleteDevFolder();

                Log.i(TAG, "Zygote is probably dead, restart rirud socket...");
                DaemonUtils.startSocket(DaemonUtils.findNativeDaemonPid());

                Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
                DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

                handler.post(() -> startWait(true, false));
            }, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }
    }

    @Keep
    public static void main(String[] args) {
        boolean enableHide = new File("/data/adb/riru/enable_hide").exists() || BuildConfig.DEBUG;
        Log.i(TAG, enableHide ? "Hide is enabled" : "Hide is not enabled.");

        String originalNativeBridge = DaemonUtils.readOriginalNativeBridge();
        Log.i(TAG, "Original native bridge is " + originalNativeBridge);

        if (originalNativeBridge == null) {
            originalNativeBridge = "0";
        }

        Looper.prepare();
        new Daemon(SERVICE_FOR_TEST, originalNativeBridge, enableHide);
        Looper.loop();
    }
}
