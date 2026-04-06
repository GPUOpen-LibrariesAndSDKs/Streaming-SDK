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
#pragma once


#include <jni.h>
#include <string>
#include <vector>

namespace ssdk::util
{
    class JavaObjectHolder;
    class ANativeWindowHolder;
    class AInputEventHolder;
    class JavaENV
    {
    public:
        JavaENV() = default;
        JavaENV(JNIEnv* env) noexcept(false);
        JavaENV(const JavaENV& other) = delete;
        JavaENV& operator=(const JavaENV& other) = delete;
        ~JavaENV();

        inline const JNIEnv* operator->() const noexcept { return m_Env; }
        inline JNIEnv* operator->() noexcept { return m_Env; }
        inline const JNIEnv& operator*() const noexcept { return *m_Env; }
        inline JNIEnv& operator*() noexcept { return *m_Env; }

        inline bool operator==(const JavaENV& other) const noexcept { return m_Env == other.m_Env; }
        inline bool operator!=(const JavaENV& other) const noexcept { return !operator==(other); }

        inline bool operator==(JNIEnv* env) const noexcept { return m_Env == env; }
        inline bool operator!=(JNIEnv* env) const noexcept { return !operator==(env); }

        inline operator JNIEnv*() noexcept { return m_Env; }
        inline operator const JNIEnv*() const noexcept { return m_Env; }

        std::string GetStringUTFChars(jstring javaStr) const;
        std::vector<int32_t> GetIntArray(jintArray javaArray, jint count) const;
        std::vector<float> GetFloatArray(jfloatArray javaArray, jint count) const;

        ANativeWindowHolder GetNativeWindowFromSurface(jobject surface) const;
        AInputEventHolder GetKeyEventFromEvent(jobject event) const;
        AInputEventHolder GetMotionEventFromEvent(jobject event) const;
        JavaObjectHolder GetObjectFromJavaObject(jobject object);

        static bool AttachCurrentThread(JavaENV& env);
        void DetachCurrentThread();

        void CallStaticVoidMethod(const char* method, const char* clazz, const char* signature, ...) noexcept(false);
        JavaObjectHolder CallStaticMethod(const char* method, const char* clazz, const char* signature, ...) noexcept(false);

        bool ExceptionCheck() const noexcept(false);
        void ExceptionDescribe() const noexcept(false);
        void ExceptionClear() const noexcept(false);

    private:
        static JavaVM* m_VM;
        JNIEnv* m_Env = nullptr;
        bool    m_ThreadAttached = false;
    };

}
