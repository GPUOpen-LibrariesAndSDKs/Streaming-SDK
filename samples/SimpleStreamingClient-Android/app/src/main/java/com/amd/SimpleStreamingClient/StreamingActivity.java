/*
Notice Regarding Standards.  AMD does not provide a license or sublicense to
any Intellectual Property Rights relating to any standards, including but not
limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
(collectively, the "Media Technologies"). For clarity, you will pay any
royalties due for such third party technologies, which may include the Media
Technologies that are owed as a result of AMD providing the Software to you.

This software uses libraries from the FFmpeg project under the LGPLv2.1.

MIT license

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

package com.amd.SimpleStreamingClient;

import static android.view.KeyEvent.KEYCODE_BACK;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.os.Build;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Objects;

public class StreamingActivity extends Activity
        implements
        GestureDetector.OnGestureListener,
        GestureDetector.OnDoubleTapListener
{
    private Surface         mPresentSurface = null;
    private Surface         mCursorSurface = null;

    private static Object mLock = new Object();

    private static final String TRACE_SCOPE = "StreamingActivity";

    public static final String ACTION_START = "com.amd.SimpleStreamingClient.START";
    public static final String ACTION_TERMINATE = "com.amd.SimpleStreamingClient.TERMINATE";

    private boolean mExitOnBack = false;

    static
    {
        try
        {
            System.loadLibrary("SimpleStreamingClient");
        }
        catch (UnsatisfiedLinkError | SecurityException e)
        {
            Log.e(TRACE_SCOPE, e.toString() + "thrown when attempting to load libSimpleStreamingClient.so. Terminating...");
            System.exit(1);
        }
    }

    private void setImmersiveSticky()
    {
        // Enables sticky immersive mode
        Window window = getWindow();
        WindowInsetsController controller = window.getInsetsController();

        if (controller != null)
        {
            // Use WindowInsetsController directly to set behavior
            controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            controller.hide(WindowInsets.Type.systemBars());

            // Ensure that content appears behind the system bars
            window.setDecorFitsSystemWindows(false);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        Log.d(TRACE_SCOPE, "onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_streaming);

        setImmersiveSticky();
        Log.d(TRACE_SCOPE, "onCreate() finished");
    }

    @Override
    protected void onStart()
    {
        Log.d(TRACE_SCOPE, "onStart()");
        super.onStart();
        Log.d(TRACE_SCOPE, "onStart() finished");
    }

    @Override
    protected void onResume()
    {
        Log.d(TRACE_SCOPE, "onResume()");
        super.onResume();
        synchronized (mLock)
        {
            connectAndStream();
        }
        Log.d(TRACE_SCOPE, "onResume() finished");
    }

    @Override
    protected void onPause()
    {
        Log.d(TRACE_SCOPE, "onPause()");
        synchronized (mLock)
        {
            stopStreaming();
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
        Log.d(TRACE_SCOPE, "onPause() finished");
        super.onPause();
    }

    @Override
    protected void onStop()
    {
        Log.d(TRACE_SCOPE, "onStop()");
        Log.d(TRACE_SCOPE, "onStop() finished");
        super.onStop();
    }

    @Override
    protected void onDestroy()
    {
        Log.d(TRACE_SCOPE, "onDestroy");
        super.onDestroy();
        Log.d(TRACE_SCOPE, "onDestroy finished");
    }

    private void connectAndStream()
    {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Object cbSync = new Object();

        SurfaceView presentSurfaceView = (SurfaceView) findViewById(R.id.surface_view_present);
        SurfaceHolder presentSurfaceHolder = presentSurfaceView.getHolder();
        presentSurfaceHolder.setFormat(PixelFormat.OPAQUE);
        presentSurfaceHolder.addCallback(new SurfaceHolder.Callback()
        {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder)
            {
                synchronized (cbSync)
                {
                    mPresentSurface = holder.getSurface();
                    //  We need both surfaces (presenter and cursor) to exist before streaming is started and there's no guarantee
                    //  of the order of the callbacks - we check the cursor surface here before calling startStreaming()
//                    if (mCursorSurface != null)
                    {
                        stopStreaming();
                        startStreaming();
                    }
                }
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height)
            {

            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder)
            {
                synchronized (cbSync)
                {
                    mPresentSurface = null;
                }
            }
        });

        SurfaceView cursorSurfaceView = (SurfaceView) findViewById(R.id.surface_view_cursor);
        cursorSurfaceView.setZOrderOnTop(true);
        SurfaceHolder cursorSurfaceHolder = cursorSurfaceView.getHolder();
        cursorSurfaceHolder.setFormat(PixelFormat.TRANSPARENT);
        cursorSurfaceHolder.addCallback(new SurfaceHolder.Callback()
        {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder)
            {
                synchronized (cbSync)
                {
                    mCursorSurface = holder.getSurface();
                    //  We need both surfaces (presenter and cursor) to exist before streaming is started and there's no guarantee
                    //  of the order of the callbacks - we check the present surface here before calling startStreaming()
                    if (mPresentSurface != null)
                    {
//                        startStreaming();
                    }
                }
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height)
            {
                setCursorSurface(holder.getSurface());
            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder)
            {
                Log.d(TRACE_SCOPE, "JAVA: surfaceDestroyed");
                synchronized (cbSync)
                {
                    mCursorSurface = null;
                }
                Log.d(TRACE_SCOPE, "JAVA: surfaceDestroyed done");
            }
        });
    }

    private String getDeviceID()
    {
        String deviceID = android.provider.Settings.Secure.getString(getApplicationContext().getContentResolver(), android.provider.Settings.Secure.ANDROID_ID);
        Log.d(TRACE_SCOPE, "Unique Device ID: " + deviceID);
        return deviceID;
    }

    //  GestureDetector.OnDoubleTapListener:
    @Override
    public boolean onSingleTapConfirmed(@NonNull MotionEvent e)
    {
        return false;
    }

    @Override
    public boolean onDoubleTap(@NonNull MotionEvent e)
    {
        return false;
    }

    @Override
    public boolean onDoubleTapEvent(@NonNull MotionEvent e)
    {
        return false;
    }

    //  GestureDetector.OnGestureListener:
    @Override
    public boolean onDown(@NonNull MotionEvent e)
    {
        return false;
    }

    @Override
    public void onShowPress(@NonNull MotionEvent e)
    {

    }

    @Override
    public boolean onSingleTapUp(@NonNull MotionEvent e)
    {
        return false;
    }

    @Override
    public boolean onScroll(@Nullable MotionEvent e1, @NonNull MotionEvent e2, float distanceX, float distanceY)
    {
        return false;
    }

    @Override
    public void onLongPress(@NonNull MotionEvent e)
    {

    }

    @Override
    public boolean onFling(@Nullable MotionEvent e1, @NonNull MotionEvent e2, float velocityX, float velocityY)
    {
        return false;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        if (handleMotionEvent(event) == true)
        {
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (handleMotionEvent(event) == true)
        {
            return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (keyCode != KeyEvent.KEYCODE_BACK && handleKeyEvent(event) == true)
        {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (keyCode != KeyEvent.KEYCODE_BACK && handleKeyEvent(event) == true)
        {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public void onBackPressed()
    {
        super.onBackPressed();
        if (mExitOnBack)
        {
            this.finishAffinity();
        }
    }

    private void startStreaming()
    {
        Settings settings = new Settings();
        Intent intent = getIntent();
        Log.d(TRACE_SCOPE, "Intent received: Action=" + intent.getAction() + ", type: " + intent.getType());
        if (Objects.equals(intent.getAction(), ACTION_START))
        {
            settings.restoreFromIntent(intent);

            SurfaceView presentSurfaceView = (SurfaceView) findViewById(R.id.surface_view_present);
            Rect rect = new Rect();
            presentSurfaceView.getGlobalVisibleRect(rect);

            if (getDisplay() != null &&
                initStreaming(mPresentSurface, mCursorSurface,
                getWindow().getDecorView().getWidth(), getWindow().getDecorView().getHeight(), getDisplay().getRefreshRate(),
                rect.left, rect.top, rect.right, rect.bottom,
                Objects.requireNonNull(getApplicationContext().getExternalFilesDir(null)).getAbsolutePath(),
                Build.MANUFACTURER, Build.MODEL, getDeviceID(),
                settings.getAutoDiscovery() ? null : settings.getURL(), settings.getPort(),
                settings.getEncryptionStatus(), settings.getEncryptionPassword(),
                settings.getSaveLog()))
            {
                Log.d(TRACE_SCOPE, "startStreaming SUCCEEDED");
            }
            else
            {
                Log.e(TRACE_SCOPE, "startStreaming FAILED");
            }
        }
        else if (Objects.equals(intent.getAction(), ACTION_TERMINATE))
        {
            Log.d(TRACE_SCOPE, "Streaming terminated by external request");
            this.finishAffinity();
        }
        else
        {
            Log.e(TRACE_SCOPE, "Unknown intent action " + intent.getAction() + "received");
        }
    }

    private void stopStreaming()
    {
        terminateStreaming();
        setCursorSurface(null);
    }

    /**
     * Perform haptic feedback on the device using Vibrator
     * Called from native code when server sends haptic vibration event
     * @param strength Vibration strength (0.0 to 1.0)
     */
    public void performHapticFeedback(float strength)
    {
        Vibrator vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        if (vibrator != null && vibrator.hasVibrator())
        {
            float clampedStrength = Math.max(0.0f, Math.min(1.0f, strength));
            int amplitude = Math.max(1, (int) (clampedStrength * 255));
            
            // Vibrate for 1 second
            VibrationEffect effect = VibrationEffect.createOneShot(1000, amplitude);
            vibrator.vibrate(effect);
            Log.d(TRACE_SCOPE, "performHapticFeedback() vibrate: strength=" + clampedStrength + ", amplitude=" + amplitude + ", duration=1000ms");
        }
        else
        {
            Log.w(TRACE_SCOPE, "performHapticFeedback() - Could not perform vibration - Vibrator not available");
        }
    }


    /**
     * Native methods that are implemented by the 'StreamingActivity.cpp' native library,
     * which is packaged with this application.
    */
    private native boolean initStreaming(Surface decodeSurface,
                                     Surface cursorSurface,
                                     int deviceWidth,
                                     int deviceHeight,
                                     float deviceRefreshRate,
                                     int deviceViewportLeft,
                                     int deviceViewportTop,
                                     int deviceViewportRight,
                                     int deviceViewportBottom,
                                     String deviceStoragePath,
                                     String deviceVendor,
                                     String deviceModel,
                                     String deviceID,
                                     String connectionUrl,
                                     int discoveryPort,
                                     boolean encryption,
                                     String password,
                                     boolean saveLog);
    private native void terminateStreaming();
    private native void setCursorSurface(Surface surface);

    private native boolean handleMotionEvent(MotionEvent event);
    private native boolean handleKeyEvent(KeyEvent event);
}
