package com.dunn.instrument.iomonitor.sample;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;

import com.dunn.instrument.iomonitor.FileIOMonitor;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

public class MainActivity extends AppCompatActivity {
    public static final String Path = Environment.getExternalStorageDirectory() + "/logger/"; //文件路径

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        FileIOMonitor.start();
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.file_read:
                smallBuffer();
                break;
            case R.id.file_write:
                writeLong();
                break;
            case R.id.no_close:
                leakFile();
                break;
        }
    }

    /**
     * 监控文件打开没有关闭
     */
    private void leakFile() {
        try {
            //File file = new File("sdcard/a_long.txt");
            File file = new File(Path+"AK_logger_cache");
            if (file.exists()) {
                file.delete();
            }
            byte[] data = new byte[512];
            for (int i = 0; i < data.length; i++) {
                data[i] = 'i';
            }
            FileOutputStream fos = new FileOutputStream(file);
            for (int i=0;i<100000;i++){
                fos.write(data);
            }
            fos.flush();
            // 打开文件没有关闭，matrix 里面有参考代码，也不是我的原创，但是大家都没搞过
            // fos.close();
            // guard.close();
            /*if (isFdOwner) {
                IoBridge.closeAndSignalBlockedThreads(fd);
            }*/
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("TAG","文件没关闭操作：e"+e);
        }


        Runtime.getRuntime().gc();
        Runtime.getRuntime().gc();
    }

    /**
     * 监控主线程写入大文件
     */
    private void writeLong() {
        try {
//            File file = new File("sdcard/a_long.txt");
            File file = new File(Path+"AK_logger_cache");
            if (file.exists()) {
                file.delete();
            }
            byte[] data = new byte[5120];
            for (int i = 0; i < data.length; i++) {
                data[i] = 'i';
            }
            FileOutputStream fos = new FileOutputStream(file);
            for (int i=0;i<100000;i++){
                fos.write(data);
            }
            fos.flush();
            fos.close();
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("TAG","文件写操作：e"+e);
        }
    }

    /**
     * 监控读的 buffer 比较小，400
     */
    private void smallBuffer() {
        // I/Choreographer: Skipped 82 frames!
        // The application may be doing too much work on its main thread.
        // 对接的 sdk 业务方，腾讯新闻，腾讯QQ 是不允许在主线程中做 IO 操作
        // 其他业务不允许长时间的 IO 操作，主线程的 IO 操作耗时超过 100ms 会警告，腾讯微视，腾讯体育，腾讯视频
        long startTime = System.currentTimeMillis();
        try {
//            File file = new File("sdcard/a_long.txt");
            File file = new File(Path+"AK_logger_cache");
            byte[] buf = new byte[400];
            FileInputStream fis = new FileInputStream(file);
            int count = 0;
            while ((count = fis.read(buf)) != -1) {
                // Log.e("TAG", "read -> " + count);
            }
            fis.close();
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("TAG","文件读操作：e"+e);
        }

        Log.e("TAG","总的操作时间："+(System.currentTimeMillis() - startTime));
    }
}