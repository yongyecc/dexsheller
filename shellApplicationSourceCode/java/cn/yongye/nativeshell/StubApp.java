package cn.yongye.nativeshell;

import android.app.Application;
import android.content.Context;


public class StubApp extends Application {


    static {
        System.loadLibrary("yongyejiagu");
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        loadDEX();
    }

    public native void loadDEX();
}
