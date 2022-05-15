package aplay.testffmpeg
import android.util.Log
import android.view.Surface
import java.io.File

/**
 *  author : cainjiang
 *  date : 2022/5/11
 *  description :
 */
class SimplePlayer {
    companion object {
        const val TAG = "SimplePlayer"
    }

    fun openVideo(url: String, surface: Surface) {
        openVideo(url, 0, 0, surface)
    }

    fun openVideo(url: String, w: Int, h: Int, surface: Surface) {
        nativeOpenVideo(url, w, h, surface)
    }

    fun openAudio(url: String) {
        Log.d(TAG, "openAudio: url = $url")
        val file = File(url)
        if (!file.exists()) {
            Log.e(TAG, "openAudio: file[$url] does not exist!")
            return
        }
        nativeOpenAudio(url)
    }

    /**
     * 播放指定视频
     * @param url 视频路径
     */
    private external fun nativeOpenVideo(url: String, w: Int, h: Int, surface: Surface)

    /**
     * 播放指定的音频
     * @param url 音频路径
     */
    private external fun nativeOpenAudio(url: String)
}