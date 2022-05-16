package aplay.testffmpeg;

import android.Manifest;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;

import com.blankj.utilcode.util.SDCardUtils;

import org.jetbrains.annotations.NotNull;

public class MainActivity extends AppCompatActivity implements PermissionUtil.RequestPermissionCallback {
    private static final String TAG = "MainActivity";
    public static final String MP4_FILE_PATH = SDCardUtils.getSDCardPathByEnvironment() + "/1080.mp4";
    public static final String FLV_FILE_PATH = SDCardUtils.getSDCardPathByEnvironment() + "/dongfengpo.flv";

    private final String[] permissions = {Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        PermissionUtil.INSTANCE.requestPermissions(this, permissions, this);
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI(MP4_FILE_PATH));
        AppCompatButton videoBtv = findViewById(R.id.btv_video_activity);
        videoBtv.setOnClickListener(v -> MainActivity.this.startActivity(new Intent(MainActivity.this, VideoActivity.class)));
        AppCompatButton audioBtv = findViewById(R.id.btv_audio_activity);
        audioBtv.setOnClickListener(v -> MainActivity.this.startActivity(new Intent(MainActivity.this, AudioActivity.class)));
        AppCompatButton yuvVideoBtv = findViewById(R.id.btv_yuv_video_activity);
        yuvVideoBtv.setOnClickListener(v -> MainActivity.this.startActivity(new Intent(MainActivity.this, YuvVideoActivity.class)));
    }

    @Override
    public void onPermissionDenied(@NotNull String[] permissions) {
        Log.w(TAG, "onPermissionDenied:");
    }

    @Override
    public void onPermissionGranted(@NotNull String[] permissions) {
        Log.i(TAG, "onPermissionGranted: ");
    }

    @Override
    public void onNeverAskAgain(@NotNull String[] permissions) {
        Log.w(TAG, "onNeverAskAgain: ");
    }

    @Override
    public void onError(@NotNull String[] permissions, @NotNull Throwable throwable) {
        Log.e(TAG, "onError: ");
    }

    @Override
    public void onNeedShowRationale(@NotNull String[] permissions) {
        Log.i(TAG, "onNeedShowRationale: ");
    }

    public native String stringFromJNI(String url);
}

