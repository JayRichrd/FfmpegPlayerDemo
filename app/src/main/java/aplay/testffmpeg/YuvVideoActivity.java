package aplay.testffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import com.blankj.utilcode.util.SDCardUtils;

public class YuvVideoActivity extends AppCompatActivity {
    public static final String YUV_FILE_PATH = SDCardUtils.getSDCardPathByEnvironment() + "/out.yuv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_yuv_video);
    }
}