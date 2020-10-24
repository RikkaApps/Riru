package riru;

import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import androidx.annotation.Keep;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * A "daemon" that reboot on zygote dead.
 */
public class Daemon {

    private static final String TAG = "rirud";
    private static final String SERVICE_FOR_TEST = "activity";

    private static void exec(String... command) {
        ProcessBuilder pb = new ProcessBuilder(command);
        try {
            Process process = pb.start();
            int code = process.waitFor();
            Log.i(TAG, command[0] + " exited with " + code);
        } catch (Throwable e) {
            Log.w("exec", e);
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

    private static void startWait(final String name, String originalNativeBridge) {
        IBinder binder = waitForSystemService(name);

        Log.i(TAG, "Zygote already started, reset prop to " + originalNativeBridge + "...");

        exec("resetprop", "ro.dalvik.vm.native.bridge", "0");

        try {
            binder.linkToDeath(new IBinder.DeathRecipient() {
                @Override
                public void binderDied() {
                    Log.i(TAG, "Zygote is possible dead, reboot to avoid problem.");
                    exec("/system/bin/reboot");
                }
            }, 0);
        } catch (RemoteException e) {
            Log.w("linkToDeath", e);
        }
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
            Log.w("can't read ", e);
        }
        return null;
    }

    @Keep
    public static void main(String[] args) {
        String originalNativeBridge = readOriginalNativeBridge();
        Log.i(TAG, "readOriginalNativeBridge: " + originalNativeBridge);

        if (originalNativeBridge == null) {
            originalNativeBridge = "0";
        }

        Looper.prepare();
        startWait(SERVICE_FOR_TEST, originalNativeBridge);
        Looper.loop();
    }
}
