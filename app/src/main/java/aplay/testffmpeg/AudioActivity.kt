package aplay.testffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import com.blankj.utilcode.util.SDCardUtils

class AudioActivity : AppCompatActivity() {
    companion object {
        const val TAG = "AudioActivity"
        val TEST_PCM_PATH = SDCardUtils.getSDCardPathByEnvironment() + "/test.pcm";
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_audio)
        openAudio()
    }

    private fun openAudio() {
        Log.d(TAG, "openAudio:")
        SimplePlayer().openAudio(TEST_PCM_PATH)
    }
}