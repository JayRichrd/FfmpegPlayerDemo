package aplay.testffmpeg

import android.app.Application
import android.content.Context
import android.util.Log
import androidx.multidex.MultiDex

class FfmpegAVApplication: Application() {
    companion object{
        const val TAG = "FfmpegAVApplication"
    }
    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "onCreate: ")
        System.loadLibrary("native-lib")
    }

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        MultiDex.install(base)
    }
}