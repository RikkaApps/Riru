package moe.riru.manager.utils;

import android.os.Build;

public class BuildUtils {

    public static boolean isQ() {
        return "Q".equals(Build.VERSION.RELEASE) || Build.VERSION.SDK_INT >= 29;
    }
}
