package riru;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

public class Daemon implements IBinder.DeathRecipient {

    public static final String TAG = "RiruDaemon";

    private static final String SERVICE_FOR_TEST = "activity";
    private static final String RIRU_LOADER = "libriruloader.so";

    private final Handler handler;
    private final String name;
    private final DaemonSocketServerThread serverThread;

    private IBinder systemServerBinder;

    public Daemon() {
        this.handler = new Handler(Looper.myLooper());
        this.serverThread = new DaemonSocketServerThread();
        this.name = SERVICE_FOR_TEST;

        serverThread.start();
        handler.post(() -> startWait(true, true));
    }

    @Override
    public void binderDied() {
        systemServerBinder.unlinkToDeath(this, 0);
        systemServerBinder = null;

        Log.i(TAG, "Zygote is probably dead, restart rirud socket...");
        serverThread.restartServer();

        Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
        DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

        Log.i(TAG, "Zygote is probably dead, delete existing /dev/riru folders...");
        DaemonUtils.deleteDevFolder();

        handler.post(() -> startWait(true, false));
    }

    private void startWait(boolean allowRestart, boolean isFirst) {
        systemServerBinder = DaemonUtils.waitForSystemService(name);

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
                        DaemonUtils.resetProperty("ctl.restart", "zygote_secondary");
                    } else {
                        DaemonUtils.resetProperty("ctl.restart", "zygote");
                    }
                    startWait(false, false);
                });
            }
            return;
        }

        Log.i(TAG, "Riru loaded, reset native bridge to " + DaemonUtils.getOriginalNativeBridge() + "...");
        DaemonUtils.resetNativeBridgeProp(DaemonUtils.getOriginalNativeBridge());

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        serverThread.stopServer();

        try {
            systemServerBinder.linkToDeath(this, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }
    }

    public static void main(String[] args) {
        DaemonUtils.killParentProcess();
        int magiskVersionCode = DaemonUtils.getMagiskVersionCode();
        String magiskTmpfsPath = DaemonUtils.getMagiskTmpfsPath();

        Log.i(TAG, "Magisk version is " + magiskVersionCode);
        Log.i(TAG, "Magisk tmpfs path is " + magiskTmpfsPath);
        Log.i(TAG, "Original native bridge is " + DaemonUtils.getOriginalNativeBridge());
        Log.i(TAG, "Dev random is " + DaemonUtils.getDevRandom());

        if (DaemonUtils.hasSELinux()) {
            if (DaemonUtils.setSocketCreateContext("u:r:zygote:s0")) {
                Log.i(TAG, "Set socket context to u:r:zygote:s0");
            } else {
                Log.w(TAG, "Failed to set socket context");
            }
        }

        Looper.prepare();
        new Daemon();
        Looper.loop();
    }
}
