package riru;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.SELinux;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Keep;

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

    public Daemon(String name, String originalNativeBridge) {
        this.handler = new Handler(Looper.myLooper());
        this.name = name;
        this.originalNativeBridge = originalNativeBridge;

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
                    if (DaemonUtils.has64Bit() && DaemonUtils.has32Bit()) {
                        // Only devices with both 32-bit and 64-bit support have zygote_secondary
                        SystemProperties.set("ctl.restart", "zygote_secondary");
                    } else {
                        SystemProperties.set("ctl.restart", "zygote");
                    }
                    startWait(false, false);
                });
            }
            return;
        }

        Log.i(TAG, "Riru loaded, reset native bridge to " + originalNativeBridge + "...");
        DaemonUtils.resetNativeBridgeProp(originalNativeBridge);

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        DaemonUtils.stopSocket(DaemonUtils.findNativeDaemonPid());

        try {
            binder.linkToDeath(() -> {
                Log.i(TAG, "Zygote is probably dead, delete existing /dev/riru folders...");
                DaemonUtils.deleteDevFolder();

                Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
                DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

                Log.i(TAG, "Zygote is probably dead, restart rirud socket...");
                DaemonUtils.startSocket(DaemonUtils.findNativeDaemonPid());

                handler.post(() -> startWait(true, false));
            }, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }
    }

    private static void checkSELinux() {
        if (SELinux.isSELinuxEnabled() && SELinux.isSELinuxEnforced()
                && (SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom")
                || SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom"))) {
            System.exit(1);
        }
    }

    @Keep
    public static void main(String[] args) {
        for (String arg : args) {
            if ("--check-selinux".equals(arg)) {
                checkSELinux();
                return;
            }
        }

        String originalNativeBridge = DaemonUtils.readOriginalNativeBridge();
        Log.i(TAG, "Original native bridge is " + originalNativeBridge);

        if (TextUtils.isEmpty(originalNativeBridge)) {
            originalNativeBridge = "0";
        }

        Looper.prepare();
        new Daemon(SERVICE_FOR_TEST, originalNativeBridge);
        Looper.loop();
    }
}
