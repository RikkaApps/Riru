package moe.riru.manager.utils;

import android.os.Build;

public class BuildUtils {

    public static boolean isQ() {
        return (Build.VERSION.SDK_INT == 28 && Build.VERSION.PREVIEW_SDK_INT > 0) || Build.VERSION.SDK_INT >= 29;
    }
}
