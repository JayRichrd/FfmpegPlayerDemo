package aplay.testffmpeg;

import static aplay.testffmpeg.YuvVideoActivity.YUV_FILE_PATH;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;

/**
 * author : cainjiang
 * date : 2022/5/11
 * description :
 */
public class YuvGLSurfaceVideoView extends GLSurfaceView implements Runnable {
    public static final String TAG = "YuvGLSurfaceVideoView";
    private int w = 0;
    private int h = 0;

    public YuvGLSurfaceVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void run() {
        new SimplePlayer().openYuvVideo(YUV_FILE_PATH, w, h, getHolder().getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated:");
        tryStart();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        this.w = w;
        this.h = h;
        Log.d(TAG, "surfaceChanged: w = " + w + ", h = " + h + ", format = " + format);
        tryStart();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed:");
    }

    private void tryStart() {
        if (w == 0 || h == 0 || getHolder().getSurface() == null) {
            Log.e(TAG, "tryStart: parameter is invalid！ w = " + w + ", h = " + h);
            return;
        }
        new Thread(this).start();
    }
}
