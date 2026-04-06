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

#include "JavaObjectHolder.h"
#include "JavaENV.h"

#include "amf/public/common/TraceAdapter.h"

#include <android/log.h>

namespace ssdk::util
{
    static constexpr const char* TRACE_SCOPE = "JavaObjectHolder.cpp";

    JavaObjectHolder::Exception::Exception(const std::string& message) :
        m_Message(message)
    {
    }

    JavaObjectHolder::JavaObjectHolder(JNIEnv* env) noexcept :
        m_Env(env)
    {
    }

    JavaObjectHolder::JavaObjectHolder(JavaENV& env) noexcept :
        m_Env(env)
    {
    }

    JavaObjectHolder::JavaObjectHolder(JNIEnv *env, jobject obj) noexcept :
        m_Env(env)
    {
        if (obj != nullptr)
        {
            m_Object = env->NewGlobalRef(obj);
        }
    }

    JavaObjectHolder::JavaObjectHolder(JavaENV& env, jobject obj) noexcept :
        m_Env(env)
    {
        if (obj != nullptr)
        {
            m_Object = env->NewGlobalRef(obj);
        }
    }

    JavaObjectHolder::~JavaObjectHolder()
    {
        if (m_Object != nullptr && m_Env != nullptr)
        {
            m_Env->DeleteGlobalRef(m_Object);
        }
    }

    JavaObjectHolder::JavaObjectHolder(const JavaObjectHolder& other) noexcept :
        m_Env(other.m_Env)
    {
        operator=(other);
    }

    JavaObjectHolder::JavaObjectHolder(JavaObjectHolder&& other) noexcept :
        m_Env(other.m_Env),
        m_Object(other.m_Object)
    {
        other.m_Object = nullptr;
        other.m_Env = nullptr;
    }

    JavaObjectHolder& JavaObjectHolder::operator=(const JavaObjectHolder& other) noexcept
    {
        if (this == &other)
        {
            return *this;  // Self-assignment check
        }

        // Clean up old object
        if (m_Object != nullptr && m_Env != nullptr)
        {
            m_Env->DeleteGlobalRef(m_Object);
            m_Object = nullptr;
        }

        // Update env (even if different from old env)
        m_Env = other.m_Env;

        // Copy new object
        if (other.m_Object != nullptr && m_Env != nullptr)
        {
            m_Object = m_Env->NewGlobalRef(other.m_Object);
        }

        return *this;
    }

    JavaObjectHolder& JavaObjectHolder::operator=(JavaObjectHolder&& other) noexcept
    {
        if (m_Object != nullptr && m_Env != nullptr)
        {
            m_Env->DeleteGlobalRef(m_Object);
        }
        m_Env = other.m_Env;
        other.m_Env = nullptr;
        m_Object = other.m_Object;
        other.m_Object = nullptr;
        return *this;
    }

    JavaObjectHolder& JavaObjectHolder::operator=(jobject obj) noexcept
    {
        if (m_Env != nullptr && m_Object != obj)
        {
            if (m_Object != nullptr)
            {
                m_Env->DeleteGlobalRef(m_Object);
            }
            m_Object = obj != nullptr ? m_Env->NewGlobalRef(obj) : nullptr;
        }
        return *this;
    }

    bool JavaObjectHolder::operator==(const JavaObjectHolder& other) const noexcept
    {
        return m_Env == other.m_Env && m_Object == other.m_Object;
    }

    bool JavaObjectHolder::operator!=(const JavaObjectHolder& other) const noexcept
    {
        return !operator==(other);
    }

    bool JavaObjectHolder::operator==(jobject obj) const noexcept
    {
        return (m_Env != nullptr && m_Object == obj) || ( m_Object == nullptr && obj == nullptr);
    }

    bool JavaObjectHolder::operator!=(jobject obj) const noexcept
    {
        return !operator==(obj);
    }

    std::string JavaObjectHolder::GetClassName() const noexcept(false)
    {
        jclass classClass = m_Env->FindClass("java/lang/Class");
        jmethodID getNameMethod = m_Env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
        jclass objClass = m_Env->GetObjectClass(m_Object);
        jstring className = (jstring)m_Env->CallObjectMethod(objClass, getNameMethod);
        return JavaENV(m_Env).GetStringUTFChars(className);
    }

    void JavaObjectHolder::CallVoidMethod(const char* name, const char* signature, ...) noexcept(false)
    {
        if (m_Env == nullptr)
        {
            throw Exception("JNI environment cannot be NULL");
        }
        if (m_Object == nullptr)
        {
            throw Exception("Object cannot be NULL");
        }
        else
        {
            JavaENV threadEnv;
            if (ssdk::util::JavaENV::AttachCurrentThread(threadEnv) == true)
            {
                bool failure = true;
                jclass objClass = threadEnv->GetObjectClass(m_Object);
                jmethodID method = threadEnv->GetMethodID(objClass, name, signature);
                if (method != nullptr)
                {
                    va_list args;
                    va_start(args, signature);
                    threadEnv->CallVoidMethodV(m_Object, method, args);
                    va_end(args);
                    failure = false;
                }
                threadEnv->DeleteLocalRef(objClass);
                if (failure == true)
                {
                    std::string msg = "Invalid call to Java method ";
                    msg += GetClassName() + ".";
                    msg += name;
                    __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
                    throw std::bad_function_call();
                }
            }
            else
            {
                __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "Failed to attach current thread to JVM");
                throw std::bad_function_call();
            }
        }
    }

    JavaObjectHolder JavaObjectHolder::CallObjectMethod(const char* name, const char* signature, ...) noexcept(false)
    {
        jobject ret = nullptr;
        if (m_Env == nullptr)
        {
            throw Exception("JNI environment cannot be NULL");
        }
        if (m_Object == nullptr)
        {
            throw Exception("Object cannot be NULL");
        }
        else
        {
            JavaENV threadEnv;
            if (ssdk::util::JavaENV::AttachCurrentThread(threadEnv) == true)
            {
                bool failure = true;
                jclass objClass = threadEnv->GetObjectClass(m_Object);
                jmethodID method = threadEnv->GetMethodID(objClass, name, signature);
                if (method != nullptr)
                {
                    va_list args;
                    va_start(args, signature);
                    ret = threadEnv->CallObjectMethodV(m_Object, method, args);
                    va_end(args);
                    failure = false;
                }
                threadEnv->DeleteLocalRef(objClass);
                if (failure == true)
                {
                    std::string msg = "Invalid call to Java method ";
                    msg += GetClassName() + ".";
                    msg += name;
                    __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
                    throw std::bad_function_call();
                }
            }
            else
            {
                __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "Failed to attach current thread to JVM");
                throw std::bad_function_call();
            }
        }
        JavaObjectHolder holder(m_Env, ret);
        if (ret != nullptr)
        {
            m_Env->DeleteLocalRef(ret);
        }
        return holder;
    }

    void JavaObjectHolder::CallVoidMethod(JavaENV& env, const char *name, const char *signature, ...) noexcept(false)
    {
        if (env == nullptr)
        {
            throw Exception("JNI environment cannot be NULL");
        }
        if (m_Object == nullptr)
        {
            throw Exception("Object cannot be NULL");
        }
        else
        {
            bool failure = true;
            jclass objClass = env->GetObjectClass(m_Object);
            jmethodID method = env->GetMethodID(objClass, name, signature);
            if (method != nullptr)
            {
                va_list args;
                va_start(args, signature);
                env->CallVoidMethodV(m_Object, method, args);
                va_end(args);
                failure = false;
            }
            env->DeleteLocalRef(objClass);
            if (failure == true)
            {
                std::string msg = "Invalid call to Java method ";
                msg += GetClassName() + ".";
                msg += name;
                __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
                throw std::bad_function_call();
            }
        }
    }

    JavaObjectHolder JavaObjectHolder::CallObjectMethod(JavaENV& env, const char *name, const char *signature, ...) noexcept(false)
    {
        jobject ret = nullptr;
        if (env == nullptr)
        {
            throw Exception("JNI environment cannot be NULL");
        }
        if (m_Object == nullptr)
        {
            throw Exception("Object cannot be NULL");
        }
        else
        {
            bool failure = true;
            jclass objClass = env->GetObjectClass(m_Object);
            jmethodID method = env->GetMethodID(objClass, name, signature);
            if (method != nullptr)
            {
                va_list args;
                va_start(args, signature);
                ret = env->CallObjectMethodV(m_Object, method, args);
                va_end(args);
                failure = false;
            }
            env->DeleteLocalRef(objClass);
            if (failure == true)
            {
                std::string msg = "Invalid call to Java method ";
                msg += GetClassName() + ".";
                msg += name;
                __android_log_print(ANDROID_LOG_ERROR, TRACE_SCOPE, "%s", msg.c_str());
                throw std::bad_function_call();
            }
        }
        JavaObjectHolder holder(env, ret);
        if (ret != nullptr)
        {
            env->DeleteLocalRef(ret);
        }
        return holder;
    }
}
