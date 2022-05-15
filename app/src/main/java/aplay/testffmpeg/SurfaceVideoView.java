package aplay.testffmpeg;

import static aplay.testffmpeg.MainActivity.MP4_FILE_PATH;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * author : cainjiang
 * date : 2022/5/11
 * description :
 */
public class SurfaceVideoView extends SurfaceView implements Runnable,SurfaceHolder.Callback {
    public static final String TAG = "SurfaceVideoView";

    public SurfaceVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        getHolder().addCallback(this);
    }

    public void run() {
        new SimplePlayer().openVideo(MP4_FILE_PATH, getHolder().getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated:");
        new Thread(this).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.d(TAG, "surfaceChanged:");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed:");
    }
}
