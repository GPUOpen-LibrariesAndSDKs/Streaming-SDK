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

#include <string>
#include <systemd/sd-bus.h>
#include "SDBusImportTable.h"
#include "SDBusMessagePtr.h"
#include "AbortController.h"
#include "public/common/Thread.h"

//-------------------------------------------------------------------------------------------------
// SDBusSlotPtr class
//
// DBusConnection creates the slots
// due to sdbus API weirdness, we need to juggle assigning the
// class fields and calling sd_bus_match_signal
//-------------------------------------------------------------------------------------------------
namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    class DBusConnection;
    //----------------------------------------------------------------------------------------------
    class SDBusSlotPtr
    {
    public:
        SDBusSlotPtr();
        SDBusSlotPtr(const SDBusSlotPtr& other);
        SDBusSlotPtr& operator=(const SDBusSlotPtr& other);
        ~SDBusSlotPtr();

        void Ref();
        void Unref();

        AMF_RESULT WaitForMessage(int timeoutMS, const AbortController* pController = nullptr);
        SDBusMessagePtr GetMessage();

        void OnSlotTriggered(sd_bus_message *pMsg);

        const std::string& GetPath() const;

        friend DBusConnection;
    private:
        const SDBusImportTable* m_pSd   = nullptr;
        sd_bus*                 m_pBus  = nullptr;
        sd_bus_slot*            m_pSlot = nullptr;
        std::string             m_path;
        amf::AMFCriticalSection m_messageSect;
        SDBusMessagePtr         m_message;
    };

} // namespace ssdk::ctls

#endif // __linux
