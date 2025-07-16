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

#if defined(__linux)

#include "SDBusSlotPtr.h"
#include "public/common/TraceAdapter.h"
#include <systemd/sd-bus.h>

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"SDBusSlotPtr";
    //-------------------------------------------------------------------------------------------------
    SDBusSlotPtr::SDBusSlotPtr()
    {}

    SDBusSlotPtr::SDBusSlotPtr(const SDBusSlotPtr& other)
        : m_pSd(other.m_pSd)
        , m_pBus(other.m_pBus)
        , m_pSlot(other.m_pSlot)
        , m_path(other.m_path)
        , m_message(other.m_message)
    {
        Ref();
    }

    SDBusSlotPtr& SDBusSlotPtr::operator=(const SDBusSlotPtr& other) {
        if (this != &other) {
            m_pSd = other.m_pSd;
            m_pBus = other.m_pBus;
            Unref();
            m_pSlot = other.m_pSlot;
            Ref();
            m_path = other.m_path;
            m_message = other.m_message;
        }
        return *this;
    }

    SDBusSlotPtr::~SDBusSlotPtr() {
        Unref();
    }

    void SDBusSlotPtr::Ref()
    {
        if (m_pSlot != nullptr)
        {
            m_pSd->m_pSDB_Slot_Ref(m_pSlot);
        }
    }

    void SDBusSlotPtr::Unref()
    {
        if (m_pSlot != nullptr)
        {
            m_pSd->m_pSDB_Slot_Unref(m_pSlot);
        }
    }

    AMF_RESULT SDBusSlotPtr::WaitForMessage(int timeoutMS, const AbortController* pController) {
        // wait for the response signal from dbus
        amf_pts deadline = amf_high_precision_clock() + timeoutMS * AMF_MILLISECOND;
        AMF_RETURN_IF_FALSE(timeoutMS > 0 || pController != nullptr, AMF_FAIL, L"WaitForMessage called without timeout or controller");
        while (true)
        {
            if (amf_high_precision_clock() > deadline && timeoutMS > 0)
            {
                return AMF_FAIL;
            }
            if (pController != nullptr && pController->IsCancelled() == true)
            {
                return AMF_FAIL;
            }

            {
                amf::AMFLock lock(&m_messageSect);
                if (m_message.Get() != nullptr)
                {
                    break;
                }
            }

            int r = m_pSd->m_pSDB_Process(m_pBus, nullptr);
            if (r > 0)
            {
                continue;
            }
            r = m_pSd->m_pSDB_Wait(m_pBus, 100);
        }
        return AMF_OK;
    }

    SDBusMessagePtr SDBusSlotPtr::GetMessage() {
        amf::AMFLock lock(&m_messageSect);
        return m_message;
    }

    void SDBusSlotPtr::OnSlotTriggered(sd_bus_message* pMsg)
    {
        if (pMsg == nullptr)
        {
            return;
        }
        amf::AMFLock lock(&m_messageSect);
        m_message = SDBusMessagePtr(m_pSd, pMsg);
        // increase m's refcount because it is borrowed in this call
        m_message.Ref();
    }

    const std::string& SDBusSlotPtr::GetPath() const
    {
        return m_path;
    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
