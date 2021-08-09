package riru;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.SELinux;
import android.os.SystemProperties;
import android.util.Log;

import java.io.File;

import riru.rirud.R;

public class Daemon implements IBinder.DeathRecipient {

    public static final String TAG = "RiruDaemon";

    private static final String SERVICE_FOR_TEST = "activity";
    private static final String RIRU_LOADER = "libriruloader.so";

    private final Handler handler = new Handler(Looper.myLooper());
    private final DaemonSocketServerThread serverThread = new DaemonSocketServerThread();

    private boolean allowRestart = true;

    private IBinder systemServerBinder;

    public Daemon() {
        // prevent zygote died after system server starts but before onRiruLoaded called
        synchronized (serverThread) {
            serverThread.start();
            handler.post(() -> startWait(true));
        }
    }

    @Override
    public void binderDied() {
        systemServerBinder.unlinkToDeath(this, 0);
        systemServerBinder = null;

        DaemonUtils.clearLoadedProcess();
        DaemonUtils.getLoadedModules().clear();

        DaemonUtils.writeStatus(R.string.zygote_dead);

        Log.i(TAG, "Zygote is probably dead, restart rirud socket...");

        Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
        DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

        Log.i(TAG, "Zygote is probably dead, delete existing /dev/riru folders...");
        DaemonUtils.deleteDevFolder();

        synchronized (serverThread) {
            serverThread.restartServer();
            handler.post(() -> startWait(false));
        }
    }

    private void onRiruNotLoaded(boolean isFirst) {
        Log.w(TAG, "Riru is not loaded.");

        if (DaemonUtils.hasIncorrectFileContext()) {
            DaemonUtils.writeStatus(R.string.bad_file_context);
            return;
        }

        boolean filesMounted = true;
        if (DaemonUtils.has64Bit()) {
            filesMounted = new File("/system/lib64/libriruloader.so").exists();
        }
        if (DaemonUtils.has32Bit()) {
            filesMounted &= new File("/system/lib/libriruloader.so").exists();
        }

        if (!filesMounted) {
            DaemonUtils.writeStatus(R.string.files_not_mounted);
            return;
        }

        if (DaemonUtils.hasSELinux() && SELinux.isSELinuxEnabled() && SELinux.isSELinuxEnforced()
                && (SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom")
                || SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom"))) {
            DaemonUtils.writeStatus(R.string.bad_selinux_rule);
            return;
        }

        if (!"libriruloader.so".equals(SystemProperties.get("ro.dalvik.vm.native.bridge"))) {
            DaemonUtils.writeStatus(R.string.bad_prop);
            return;
        }

        DaemonUtils.writeStatus(R.string.not_loaded);
        if (isFirst) {
            Log.w(TAG, "Magisk post-fs-data slow?");
        }

        if (allowRestart) {
            allowRestart = false;
            handler.post(() -> {
                Log.w(TAG, "Restarting zygote...");
                if (DaemonUtils.has64Bit() && DaemonUtils.has32Bit()) {
                    // Only devices with both 32-bit and 64-bit support have zygote_secondary
                    SystemProperties.set("ctl.restart", "zygote_secondary");
                } else {
                    SystemProperties.set("ctl.restart", "zygote");
                }
            });
        } else {
            Log.w(TAG, "Restarting zygote does not help");
        }
    }

    private void onRiruLoad() {
        allowRestart = true;
        Log.i(TAG, "Riru loaded, reset native bridge to " + DaemonUtils.getOriginalNativeBridge() + "...");
        DaemonUtils.resetNativeBridgeProp(DaemonUtils.getOriginalNativeBridge());

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        serverThread.stopServer();

        var loadedModules = DaemonUtils.getLoadedModules().toArray();

        StringBuilder sb = new StringBuilder();
        if (loadedModules.length == 0) {
            sb.append(DaemonUtils.res.getString(R.string.empty));
        } else {
            sb.append(loadedModules[0]);
            for (int i = 1; i < loadedModules.length; ++i) {
                sb.append(", ");
                sb.append(loadedModules[i]);
            }
        }

        DaemonUtils.writeStatus(R.string.loaded, loadedModules.length, sb);
    }

    private void startWait(boolean isFirst) {
        systemServerBinder = DaemonUtils.waitForSystemService(SERVICE_FOR_TEST);

        DaemonUtils.reloadLocale();

        try {
            systemServerBinder.linkToDeath(this, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }

        synchronized (serverThread) {
            if (!DaemonUtils.isLoaded()) {
                onRiruNotLoaded(isFirst);
            } else {
                onRiruLoad();
            }
        }
    }

    public static void main(String[] args) {
        DaemonUtils.init(args);
        DaemonUtils.killParentProcess();
        DaemonUtils.writeStatus(R.string.app_process);
        int magiskVersionCode = DaemonUtils.getMagiskVersionCode();
        String magiskTmpfsPath = DaemonUtils.getMagiskTmpfsPath();

        Log.i(TAG, "Magisk version is " + magiskVersionCode);
        Log.i(TAG, "Magisk tmpfs path is " + magiskTmpfsPath);
        Log.i(TAG, "Original native bridge is " + DaemonUtils.getOriginalNativeBridge());
        Log.i(TAG, "Dev random is " + DaemonUtils.getDevRandom());

        Looper.prepare();
        new Daemon();
        Looper.loop();
    }
}
