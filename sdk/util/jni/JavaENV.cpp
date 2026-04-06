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

#include "JavaENV.h"
#include "ANativeWindowHolder.h"
#include "AInputEventHolder.h"
#include "JavaObjectHolder.h"

#include <android/native_window_jni.h>
#include <android/log.h>

#include <algorithm>

static constexpr const char* TRACE_SCOPE = "JavaENV.cpp";

namespace ssdk::util
{
    JavaVM* JavaENV::m_VM = nullptr;

    JavaENV::JavaENV(JNIEnv* env) noexcept(false) :
        m_Env(env)
    {
        if (env == nullptr)
        {
            static constexpr const char* msg = "JNI environment cannot be nullptr";
            __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, msg);
            throw std::invalid_argument(msg);
        }
        else if (m_VM == nullptr)
        {
            env->GetJavaVM(&m_VM);
        }
    }

    JavaENV::~JavaENV()
    {
        if (m_ThreadAttached == true)
        {
            DetachCurrentThread();
        }
    }

    std::string JavaENV::GetStringUTFChars(jstring javaStr) const
    {
        std::string str;
        if (javaStr != nullptr && m_Env != nullptr)
        {
            const char* c_str = m_Env->GetStringUTFChars(javaStr, nullptr);
            str = c_str;
            m_Env->ReleaseStringUTFChars(javaStr, c_str);
        }
        return str;
    }

    std::vector<int32_t> JavaENV::GetIntArray(jintArray javaArray, jint count) const
    {
        std::vector<int32_t> array(count);
        jint* elements = m_Env->GetIntArrayElements(javaArray, nullptr);
        std::copy_n(elements, count, array.begin());
        if (elements != nullptr)
        {
            m_Env->ReleaseIntArrayElements(javaArray, elements, JNI_ABORT);
        }
        return array;
    }

    std::vector<float> JavaENV::GetFloatArray(jfloatArray javaArray, jint count) const
    {
        std::vector<float> array(count);
        jfloat* elements = m_Env->GetFloatArrayElements(javaArray, nullptr);
        std::copy_n(elements, count, array.begin());
        if (elements != nullptr)
        {
            m_Env->ReleaseFloatArrayElements(javaArray, elements, JNI_ABORT);
        }
        return array;
    }

    ANativeWindowHolder JavaENV::GetNativeWindowFromSurface(jobject surface) const
    {
        ANativeWindowHolder holder(ANativeWindow_fromSurface(m_Env, surface));
        return std::move(holder);
    }

    AInputEventHolder JavaENV::GetKeyEventFromEvent(jobject event) const
    {
        AInputEventHolder holder(AKeyEvent_fromJava(m_Env, event));
        return std::move(holder);
    }

    AInputEventHolder JavaENV::GetMotionEventFromEvent(jobject event) const
    {
        AInputEventHolder holder(AMotionEvent_fromJava(m_Env, event));
        return std::move(holder);
    }

    JavaObjectHolder JavaENV::GetObjectFromJavaObject(jobject object)
    {
        JavaObjectHolder holder(*this, object);
        return std::move(holder);
    }

    bool JavaENV::AttachCurrentThread(JavaENV& env)
    {
        bool result = false;
        if (m_VM != nullptr)
        {
            JNIEnv *newEnv = nullptr;
            if (m_VM->AttachCurrentThread(&newEnv, nullptr) == JNI_OK)
            {
                env.m_Env = newEnv;
                env.m_ThreadAttached = true;
                result = true;
            }
            else
            {
                __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE,
                                    "AttachCurrentThread() failed to attach current thread to JVM");
            }
        }
        return result;
    }

    void JavaENV::DetachCurrentThread()
    {
        if (m_Env != nullptr)
        {
            m_VM->DetachCurrentThread();
            m_ThreadAttached = false;
        }
    }

    void JavaENV::CallStaticVoidMethod(const char* methodName, const char* clazz, const char* signature, ...) noexcept(false)
    {
        if (m_Env == nullptr)
        {
            throw std::invalid_argument("JNI environment cannot be NULL");
        }
        bool failure = true;
        jclass objClass = m_Env->FindClass(clazz);
        jmethodID method = m_Env->GetStaticMethodID(objClass, methodName, signature);
        if (method != nullptr)
        {
            va_list args;
            va_start(args, signature);
            m_Env->CallStaticVoidMethodV(objClass, method, args);
            va_end(args);
            failure = false;
        }
        m_Env->DeleteLocalRef(objClass);
        if (failure == true)
        {
            std::string msg = "Invalid call to Java method ";
            msg += clazz;
            msg += ".";
            msg += methodName;
            __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
            throw std::bad_function_call();
        }
    }

    ssdk::util::JavaObjectHolder JavaENV::CallStaticMethod(const char* methodName, const char* clazz, const char* signature, ...) noexcept(false)
    {
        jobject ret = nullptr;
        if (m_Env == nullptr)
        {
            throw std::invalid_argument("JNI environment cannot be NULL");
        }
        bool failure = true;
        jclass objClass = m_Env->FindClass(clazz);
        jmethodID method = m_Env->GetStaticMethodID(objClass, methodName, signature);
        if (method != nullptr)
        {
            va_list args;
            va_start(args, signature);
            ret = m_Env->CallStaticObjectMethodV(objClass, method, args);
            va_end(args);
            failure = false;
        }
        m_Env->DeleteLocalRef(objClass);
        if (failure == true)
        {
            std::string msg = "Invalid call to Java method ";
            msg += clazz;
            msg += ".";
            msg += methodName;
            __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
            throw std::bad_function_call();
        }
        return JavaObjectHolder(*this, ret);
    }

    bool JavaENV::ExceptionCheck() const noexcept(false)
    {
        if (m_Env == nullptr)
        {
            throw std::invalid_argument("JNI environment cannot be NULL");
        }
        else
        {
            return m_Env->ExceptionCheck();
        }
    }

    void JavaENV::ExceptionDescribe() const noexcept(false)
    {
        if (m_Env == nullptr)
        {
            throw std::invalid_argument("JNI environment cannot be NULL");
        }
        else
        {
            m_Env->ExceptionDescribe();
        }
    }

    void JavaENV::ExceptionClear() const noexcept(false)
    {
        if (m_Env == nullptr)
        {
            throw std::invalid_argument("JNI environment cannot be NULL");
        }
        else
        {
            m_Env->ExceptionClear();
        }
    }

}
