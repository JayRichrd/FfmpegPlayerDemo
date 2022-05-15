package aplay.testffmpeg;

import static aplay.testffmpeg.MainActivity.MP4_FILE_PATH;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.blankj.utilcode.util.SDCardUtils;

/**
 * author : cainjiang
 * date : 2022/5/11
 * description :
 */
public class GLSurfaceVideoView extends GLSurfaceView implements Runnable {
    public static final String TAG = "GLSurfaceVideoView";
    private int w = 0;
    private int h = 0;

    public GLSurfaceVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void run() {
        new SimplePlayer().openVideo(MP4_FILE_PATH, w, h, getHolder().getSurface());
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
