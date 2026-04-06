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

#include "JavaENV.h"

#include <jni.h>
#include <memory>
#include <functional>

namespace ssdk::util
{
    class JavaObjectHolder
    {
    public:
        typedef std::shared_ptr<JavaObjectHolder>   Ptr;

    class Exception : public std::exception
        {
        public:
            Exception(const std::string& message);

            virtual const char* what() const noexcept { return m_Message.c_str(); }
        private:
            std::string m_Message;
        };

    public:
        JavaObjectHolder() = default;
        explicit JavaObjectHolder(JNIEnv* env) noexcept;
        explicit JavaObjectHolder(JavaENV& env) noexcept;
        explicit JavaObjectHolder(JNIEnv* env, jobject obj) noexcept;
        explicit JavaObjectHolder(JavaENV& env, jobject obj) noexcept;
        JavaObjectHolder(const JavaObjectHolder& other) noexcept;
        JavaObjectHolder(JavaObjectHolder&& other) noexcept;
        ~JavaObjectHolder();

        JavaObjectHolder& operator=(const JavaObjectHolder& other) noexcept;
        JavaObjectHolder& operator=(JavaObjectHolder&& other) noexcept;
        JavaObjectHolder& operator=(jobject obj) noexcept;

        bool operator==(const JavaObjectHolder& other) const noexcept;
        bool operator!=(const JavaObjectHolder& other) const noexcept;
        bool operator==(jobject obj) const noexcept;
        bool operator!=(jobject obj) const noexcept;

        inline operator jobject() const noexcept { return m_Object; }

        std::string GetClassName() const noexcept(false);

        void CallVoidMethod(const char* name, const char* signature, ...) noexcept(false);
        void CallVoidMethod(JavaENV& env, const char* name, const char* signature, ...) noexcept(false);
        JavaObjectHolder CallObjectMethod(const char* name, const char* signature, ...) noexcept(false);
        JavaObjectHolder CallObjectMethod(JavaENV& env, const char* name, const char* signature, ...) noexcept(false);

    private:

    private:
        JNIEnv*     m_Env = nullptr;
        jobject     m_Object = nullptr;
    };
}
