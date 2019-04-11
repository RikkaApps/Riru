package moe.riru.manager.utils;

import android.content.Context;

import androidx.annotation.Keep;
import moe.riru.manager.R;

@Keep
public class NativeHelper {

    private static final int MAX_VERSION = 19;
    private static final int V18 = 19;
    private static final int V17_1 = 18;

    static {
        System.loadLibrary("helper");
    }

    public static String versionName(Context context, int versionCode) {
        if (versionCode > MAX_VERSION) {
            return context.getString(R.string.riru_unknown_version, versionCode);
        }

        if (versionCode <= 0) {
            return context.getString(R.string.riru_not_found);
        }

        switch (versionCode) {
            case V18:
                return "v18";
            case V17_1:
                return "v17.1";
            default:
                return "v" + versionCode;
        }
    }

    public static native boolean init();
    public static native boolean isRiruModuleExists(String name);
    public static native int getRiruVersion();
    public static native boolean isZygoteMethodsReplaced();
    public static native int getNativeForkAndSpecializeCallsCount();
    public static native int getNativeForkSystemServerCallsCount();
    public static native String getNativeForkAndSpecializeSignature();
    public static native String getNativeForkSystemServerSignature();
}
