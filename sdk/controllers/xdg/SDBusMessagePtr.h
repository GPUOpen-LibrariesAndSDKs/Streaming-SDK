// 
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
// 
// MIT license 
// 
//
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#if defined(__linux)

#include <systemd/sd-bus.h>
#include "SDBusImportTable.h"
#include "public/common/TraceAdapter.h"
#include "public/include/core/Result.h"
#include <string>
#include <functional>

//-------------------------------------------------------------------------------------------------
// SDBusMessagePtr class
//-------------------------------------------------------------------------------------------------
namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    class SDBusMessagePtr
    {
    public:
        SDBusMessagePtr();
        SDBusMessagePtr(const SDBusImportTable* pSd, sd_bus_message* pMsg);
        SDBusMessagePtr(const SDBusMessagePtr& other);
        SDBusMessagePtr& operator=(const SDBusMessagePtr& other);
        ~SDBusMessagePtr();

        sd_bus_message* Get() const;
        void Ref();
        void Unref();

        template <typename... Args>
        AMF_RESULT Append(const char* pTypes, Args... args);

        AMF_RESULT OpenContainer(char type, const char* pContents);
        AMF_RESULT CloseContainer();
        AMF_RESULT EnterContainer(char type, const char* pContents);
        AMF_RESULT ExitContainer();

        template <typename... Args>
        AMF_RESULT Read(const char* pTypes, Args... args);

        AMF_RESULT Skip(const char* pTypes);
        AMF_RESULT PeekType(const char** ppSig);
        AMF_RESULT GetKeyAndSignature(std::string& key, std::string& sig);
    private:
        const SDBusImportTable* m_pSd  = nullptr;
        sd_bus_message*         m_pMsg = nullptr;
    };

    #define AMF_FACILITY L"SDBusMessagePtr"

    // we need to use varidic templates because we only have "read" which takes
    // c-style varidic arguments. there is a "readv" but it is not supported
    // in all versions of systemd.
    template <typename... Args>
    AMF_RESULT SDBusMessagePtr::Read(const char* pTypes, Args... args)
    {
        int r = m_pSd->m_pSDB_Message_Read(m_pMsg, pTypes, args...);
        if (r == 0)
        {
            return AMF_EOF;
        }
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't read from message! err=%d", r);
        return AMF_OK;
    }

    // same comment above "read" applies to "append"
    template <typename... Args>
    AMF_RESULT SDBusMessagePtr::Append(const char* pTypes, Args... args)
    {
        int r = m_pSd->m_pSDB_Message_Append(m_pMsg, pTypes, args...);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't append to message! err=%d", r);
        return AMF_OK;
    }

    #undef AMF_FACILITY

} // namespace ssdk::ctls

#endif // __linux
