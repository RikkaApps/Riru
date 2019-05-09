package moe.riru.manager;

import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.Nullable;
import moe.riru.manager.app.BaseActivity;
import moe.riru.manager.utils.NativeHelper;
import moe.shizuku.support.text.HtmlCompat;

public class MainActivity extends BaseActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        boolean init, isZygoteMethodsReplaced;
        int version, nativeForkAndSpecializeCallsCount, nativeForkSystemServerCallsCount, nativeSpecializeAppProcessCallsCount;
        String nativeForkAndSpecializeSignature, nativeSpecializeAppProcessSignature, nativeForkSystemServerSignature;

        StringBuilder sb = new StringBuilder();

        init = NativeHelper.init();
        if (init) {
            version = NativeHelper.getRiruVersion();
            if (version < 13) {
                sb.append("Riru version less than 13, diagnostic is only available for Riru v13 or above.");
            } else {
                isZygoteMethodsReplaced = NativeHelper.isZygoteMethodsReplaced();
                nativeForkAndSpecializeCallsCount = NativeHelper.getNativeForkAndSpecializeCallsCount();
                nativeForkSystemServerCallsCount = NativeHelper.getNativeForkSystemServerCallsCount();
                nativeSpecializeAppProcessCallsCount = NativeHelper.getNativeSpecializeAppProcessCallsCount();
                nativeForkAndSpecializeSignature = NativeHelper.getNativeForkAndSpecializeSignature();
                nativeSpecializeAppProcessSignature = NativeHelper.getNativeSpecializeAppProcessSignature();
                nativeForkSystemServerSignature = NativeHelper.getNativeForkSystemServerSignature();

                sb.append("Riru ").append(NativeHelper.versionName(this, version)).append(" found.").append("<br>");

                if (isZygoteMethodsReplaced) {
                    sb.append("Native methods of Zygote class replaced.").append("<br><br>")
                            .append("nativeForkAndSpecialize calls count: ").append(nativeForkAndSpecializeCallsCount).append("<br>")
                            .append("nativeForkSystemServer calls count: ").append(nativeForkSystemServerCallsCount).append("<br>")
                            .append("nativeSpecializeAppProcess calls count: ").append(nativeSpecializeAppProcessCallsCount).append("<br>");

                    if (nativeForkAndSpecializeCallsCount <= 0 && nativeSpecializeAppProcessCallsCount <= 0) {
                        sb.append("<br>nativeForkAndSpecialize/nativeSpecializeAppProcess calls count is 0, Riru is not working correctly.<br>This may because Riru's hook is overwritten by other things, please check yourself.");
                    } else if (nativeForkSystemServerCallsCount != 0) {
                        sb.append("<br>Everything looks fine :D");
                    }
                } else {
                    sb.append("However, native methods of Zygote class not replaced, please contact developer with the following information.").append("<br><br>")
                            .append("nativeForkAndSpecializeSignature:<br><font face=\"monospace\">").append(nativeForkAndSpecializeSignature).append("</font><br><br>")
                            .append("nativeSpecializeAppProcessSignature (from Q beta 3):<br><font face=\"monospace\">").append(nativeSpecializeAppProcessSignature).append("</font><br><br>")
                            .append("nativeForkSystemServerSignature:<br><font face=\"monospace\">").append(nativeForkSystemServerSignature).append("</font>");
                }
            }
        } else {
            sb.append("Riru not found in memory.");
        }

        new AlertDialog.Builder(this)
                .setMessage(HtmlCompat.fromHtml(sb.toString()))
                .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    finish();
                })
                .setNeutralButton("Copy", (dialog, which) -> {
                    getSystemService(ClipboardManager.class).setPrimaryClip(ClipData.newPlainText("text", HtmlCompat.fromHtml(sb.toString()).toString()));
                    finish();
                })
                .setCancelable(false)
                .show();

        Log.i("RiruManager", "init: " + NativeHelper.init());
        Log.i("RiruManager", "getRiruVersion: " + NativeHelper.getRiruVersion());
        Log.i("RiruManager", "isZygoteMethodsReplaced: " + NativeHelper.isZygoteMethodsReplaced());
        Log.i("RiruManager", "getNativeForkAndSpecializeCallsCount: " + NativeHelper.getNativeForkAndSpecializeCallsCount());
        Log.i("RiruManager", "getNativeForkSystemServerCallsCount: " + NativeHelper.getNativeForkSystemServerCallsCount());
        Log.i("RiruManager", "getNativeForkAndSpecializeSignature: " + NativeHelper.getNativeForkAndSpecializeSignature());
        Log.i("RiruManager", "getNativeSpecializeAppProcessSignature: " + NativeHelper.getNativeSpecializeAppProcessSignature());
        Log.i("RiruManager", "getNativeForkSystemServerSignature: " + NativeHelper.getNativeForkSystemServerSignature());
    }
}
