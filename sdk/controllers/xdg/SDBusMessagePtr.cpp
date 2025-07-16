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

#include "SDBusMessagePtr.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"SDBusMessagePtr";
    //-------------------------------------------------------------------------------------------------
    SDBusMessagePtr::SDBusMessagePtr()
        : m_pSd(nullptr)
        , m_pMsg(nullptr)
    {}

    SDBusMessagePtr::SDBusMessagePtr(const SDBusImportTable* pSd, sd_bus_message* pMsg)
        : m_pSd(pSd)
        , m_pMsg(pMsg)
    {
        //don't increase ref
    }

    SDBusMessagePtr::SDBusMessagePtr(const SDBusMessagePtr& other)
        : m_pSd(other.m_pSd)
        , m_pMsg(other.m_pMsg)
    {
        Ref();
    }

    SDBusMessagePtr& SDBusMessagePtr::operator=(const SDBusMessagePtr& other) {
        if (this != &other)
        {
            m_pSd = other.m_pSd;
            Unref();
            m_pMsg = other.m_pMsg;
            Ref();
        }
        return *this;
    }

    SDBusMessagePtr::~SDBusMessagePtr() {
        Unref();
        m_pMsg = nullptr;
    }

    sd_bus_message* SDBusMessagePtr::Get() const
    {
        return m_pMsg;
    }

    void SDBusMessagePtr::Ref()
    {
        if (m_pMsg != nullptr)
        {
            m_pSd->m_pSDB_Message_Ref(m_pMsg);
        }
    }

    void SDBusMessagePtr::Unref()
    {
        if (m_pMsg != nullptr)
        {
            m_pSd->m_pSDB_Message_Unref(m_pMsg);
        }
    }

    AMF_RESULT SDBusMessagePtr::OpenContainer(char type, const char* pContents)
    {
        int r = m_pSd->m_pSDB_Message_Open_Container(m_pMsg, type, pContents);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't open container! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::CloseContainer()
    {
        int r = m_pSd->m_pSDB_Message_Close_Container(m_pMsg);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't close container! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::EnterContainer(char type, const char* pContents)
    {
        int r = m_pSd->m_pSDB_Message_Enter_Container(m_pMsg, type, pContents);
        if (r == 0)
        {
            return AMF_EOF;
        }
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't open container! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::ExitContainer()
    {
        int r = m_pSd->m_pSDB_Message_Exit_Container(m_pMsg);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't close container! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::Skip(const char* pTypes)
    {
        int r = m_pSd->m_pSDB_Message_Skip(m_pMsg, pTypes);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't skip data in message! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::PeekType(const char** ppSig)
    {
        int r = m_pSd->m_pSDB_Message_Peek_Type(m_pMsg, NULL, ppSig);
        AMF_RETURN_IF_FALSE(r >= 0, AMF_FAIL, L"Couldn't peek type from message! err=%d", r);
        return AMF_OK;
    }

    AMF_RESULT SDBusMessagePtr::GetKeyAndSignature(std::string& key, std::string& sig)
    {
        const char* pKey;
        AMF_RETURN_IF_FAILED(Read("s", &pKey));

        const char* pSig;
        AMF_RETURN_IF_FAILED(PeekType(&pSig));

        key = std::string(pKey);
        sig = std::string(pSig);
        return AMF_OK;
    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
