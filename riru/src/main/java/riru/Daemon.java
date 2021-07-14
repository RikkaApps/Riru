package riru;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.SELinux;
import android.os.SystemProperties;
import android.util.Log;

import java.io.File;
import java.util.List;
import java.util.Locale;

import riru.resource.Strings;

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

        DaemonUtils.setIsLoaded(false, false);
        DaemonUtils.setIsLoaded(true, false);
        DaemonUtils.getLoadedModules(false).clear();

        Log.i(TAG, "Zygote is probably dead, restart rirud socket...");
        serverThread.restartServer();

        Log.i(TAG, "Zygote is probably dead, reset native bridge to " + RIRU_LOADER + "...");
        DaemonUtils.resetNativeBridgeProp(RIRU_LOADER);

        Log.i(TAG, "Zygote is probably dead, delete existing /dev/riru folders...");
        DaemonUtils.deleteDevFolder();

        handler.post(() -> startWait(true, false));
    }

    private void onRiruNotLoaded(boolean allowRestart, boolean isFirst) {
        Log.w(TAG, "Riru is not loaded.");

        boolean filesMounted = true;
        if (DaemonUtils.has64Bit()) {
            filesMounted = new File("/system/lib64/libriruloader.so").exists();
        }
        if (DaemonUtils.has32Bit()) {
            filesMounted &= new File("/system/lib/libriruloader.so").exists();
        }

        if (!filesMounted) {
            DaemonUtils.writeStatus(Strings.get(Strings.files_not_mounted));
            return;
        }

        if (DaemonUtils.hasSELinux() && SELinux.isSELinuxEnabled() && SELinux.isSELinuxEnforced()
                && (SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "file", "relabelfrom")
                || SELinux.checkSELinuxAccess("u:r:init:s0", "u:object_r:system_file:s0", "dir", "relabelfrom"))) {
            DaemonUtils.writeStatus(Strings.get(Strings.bad_selinux_rule));
            return;
        }

        if (!"libriruloader.so".equals(SystemProperties.get("ro.dalvik.vm.native.bridge"))) {
            DaemonUtils.writeStatus(Strings.get(Strings.bad_prop));
            return;
        }

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
    }

    private void onRiruLoad() {
        Log.i(TAG, "Riru loaded, reset native bridge to " + DaemonUtils.getOriginalNativeBridge() + "...");
        DaemonUtils.resetNativeBridgeProp(DaemonUtils.getOriginalNativeBridge());

        Log.i(TAG, "Riru loaded, stop rirud socket...");
        serverThread.stopServer();

        try {
            systemServerBinder.linkToDeath(this, 0);
        } catch (RemoteException e) {
            Log.w(TAG, "linkToDeath", e);
        }

        List<String> loadedModules = DaemonUtils.getLoadedModules(DaemonUtils.has64Bit());

        StringBuilder sb = new StringBuilder();
        if (loadedModules.isEmpty()) {
            sb.append("empty");
        } else {
            sb.append(loadedModules.get(0));
            for (int i = 1; i < loadedModules.size(); i++) {
                sb.append(", ");
                sb.append(loadedModules.get(i));
            }
        }

        DaemonUtils.writeStatus(String.format(Locale.ENGLISH, Strings.get(Strings.loaded), loadedModules.size(), sb));
    }

    private void startWait(boolean allowRestart, boolean isFirst) {
        systemServerBinder = DaemonUtils.waitForSystemService(name);

        if (!DaemonUtils.isLoaded(DaemonUtils.has64Bit())) {
            onRiruNotLoaded(allowRestart, isFirst);
        } else {
            onRiruLoad();
        }
    }

    public static void main(String[] args) {
        DaemonUtils.init(args);
        DaemonUtils.killParentProcess();
        DaemonUtils.writeStatus("\uD83E\uDD14 app_process launched");
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
