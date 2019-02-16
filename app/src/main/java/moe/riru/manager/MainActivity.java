package moe.riru.manager;

import android.os.Bundle;
import android.util.Log;

import androidx.annotation.Nullable;
import moe.riru.manager.app.BaseActivity;
import moe.riru.manager.utils.NativeHelper;

public class MainActivity extends BaseActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.i("RiruManager", "init: " + NativeHelper.init());
        Log.i("RiruManager", "getRiruVersion: " + NativeHelper.getRiruVersion());
        Log.i("RiruManager", "isZygoteMethodsReplaced: " + NativeHelper.isZygoteMethodsReplaced());
        Log.i("RiruManager", "getNativeForkAndSpecializeCallsCount: " + NativeHelper.getNativeForkAndSpecializeCallsCount());
        Log.i("RiruManager", "getNativeForkSystemServerCallsCount: " + NativeHelper.getNativeForkSystemServerCallsCount());
    }
}
