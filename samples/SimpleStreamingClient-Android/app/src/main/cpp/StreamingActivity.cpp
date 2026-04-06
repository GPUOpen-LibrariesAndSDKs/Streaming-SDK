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
#include "SimpleStreamingClient.h"
#include "util/jni/JavaENV.h"
#include "util/jni/AInputEventHolder.h"
#include "controllers/client/android/GameControllerAndroid.h"

#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"

#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <jni.h>
#include <string>
#include <locale>
#include <codecvt>

static constexpr const char* TRACE_SCOPE = "StreamingActivity.cpp";

extern "C" JNIEXPORT jboolean JNICALL
Java_com_amd_SimpleStreamingClient_StreamingActivity_initStreaming(JNIEnv* env, jobject thiz,
        jobject decodeSurface, jobject cursorSurface,
        jint deviceWidth, jint deviceHeight, jfloat deviceRefreshRate,
        jint deviceViewportLeft, jint deviceViewportTop, jint deviceViewportRight, jint deviceViewportBottom,
        jstring deviceStoragePath, jstring deviceVendor, jstring deviceModel, jstring deviceID,
        jstring connectionUrl, jint discoveryPort, jboolean encryption, jstring password,
        jboolean saveLog)
{
    __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE, "StreamingActivity.initStreaming");

    bool result = true;
    ssdk::util::JavaENV jEnv(env);

    ssdk::util::ANativeWindowHolder pDecodeNativeWindow = jEnv.GetNativeWindowFromSurface(decodeSurface);
    ssdk::util::ANativeWindowHolder pCursorNativeWindow;// = jEnv.GetNativeWindowFromSurface(cursorSurface);

    if (pDecodeNativeWindow.IsEmpty() == true)
    {
        __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE,"StreamingActivity.initStreaming: Unable to get native window from decoder surface");
        result = false;
    }
/*
    else if (pCursorNativeWindow.IsEmpty() == true)
    {
        __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE,"StreamingActivity.initStreaming: Unable to get native window from cursor surface");
        result = false;
    }
    */
    else
    {
        __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE,"StreamingActivity.initStreaming: Initialize and start the Video Receiver");
        if (SimpleStreamingClient::GetInstance().Init(std::move(jEnv.GetObjectFromJavaObject(thiz)),
                                                  std::move(pDecodeNativeWindow), std::move(pCursorNativeWindow),
                                                  deviceWidth, deviceHeight, static_cast<int>(std::round(deviceRefreshRate)),
                                                  deviceViewportLeft, deviceViewportTop, deviceViewportRight, deviceViewportBottom,
                                                  jEnv.GetStringUTFChars(deviceVendor), jEnv.GetStringUTFChars(deviceModel), jEnv.GetStringUTFChars(deviceID),
                                                  jEnv.GetStringUTFChars(deviceStoragePath),
                                                  jEnv.GetStringUTFChars(connectionUrl), discoveryPort,
                                                  encryption, jEnv.GetStringUTFChars(password), saveLog) != SimpleStreamingClient::Result::OK)
        {
            __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE,"StreamingActivity.initStreaming: VideoReceiver initialization failed");
            result = false;
        }
        else
        {
            __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE,
                                "StreamingActivity.initStreaming: Initializing Client - done");
        }
    }
    __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE, "StreamingActivity.initStreaming done");

    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_amd_SimpleStreamingClient_StreamingActivity_terminateStreaming(JNIEnv* env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE, "terminateStreaming");
    
    SimpleStreamingClient::Destroy();
    __android_log_print(ANDROID_LOG_DEBUG, TRACE_SCOPE, "terminateStreaming done");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_amd_SimpleStreamingClient_StreamingActivity_setCursorSurface(JNIEnv* env, jobject thiz, jobject surface)
{

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_amd_SimpleStreamingClient_StreamingActivity_handleMotionEvent(JNIEnv *env, jobject /*thiz*/, jobject event)
{
    ssdk::util::JavaENV jEnv(env);
    ssdk::util::AInputEventHolder inputEvent = jEnv.GetMotionEventFromEvent(event);
    return SimpleStreamingClient::GetInstance().DispatchInputEvent(*inputEvent);
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_amd_SimpleStreamingClient_StreamingActivity_handleKeyEvent(JNIEnv *env, jobject /*thiz*/, jobject event)
{
    ssdk::util::JavaENV jEnv(env);
    ssdk::util::AInputEventHolder inputEvent = jEnv.GetKeyEventFromEvent(event);
    return SimpleStreamingClient::GetInstance().DispatchInputEvent(*inputEvent);
}