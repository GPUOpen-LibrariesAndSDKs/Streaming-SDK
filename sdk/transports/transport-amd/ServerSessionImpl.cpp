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

Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

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

#include "ServerSessionImpl.h"
#include "Misc.h"
#include "DgramClientSessionFlowCtrl.h"
#include "public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerSessionImpl";

namespace ssdk::transport_amd
{
    ssdk::transport_common::SessionHandle ServerSessionImpl::m_UniqID = 0;

    ServerSessionImpl::ServerSessionImpl(ServerImpl* server) :
        m_Server(server)
    {
        ++m_UniqID;
        m_SessionHandle = m_UniqID;
    }

    ServerSessionImpl::~ServerSessionImpl()
    {
        AMFTraceDebug(AMF_FACILITY, L"Session destroyed");
    }

    void AMF_STD_CALL ServerSessionImpl::RegisterReceiverCallback(ReceiverCallback* callback)
    {
        m_Callback = callback;
    }

    void ServerSessionImpl::TimeoutNotify()
    {
        DisconnectNotify(ReceiverCallback::TerminationReason::TIMEOUT);
    }

    void ServerSessionImpl::TerminateNotify()
    {
        DisconnectNotify(ReceiverCallback::TerminationReason::DISCONNECT);
    }

    void ServerSessionImpl::DisconnectNotify(ReceiverCallback::TerminationReason reason)
    {
        if (IsTerminated() == false)
        {
            if (m_Callback != nullptr)
            {
                m_Callback->OnTerminate(this, reason);
            }
        }
    }

    void ServerSessionImpl::PopulateOptionsFromHello(const HelloRequest& request, amf::AMFPropertyStorage* propStorage)
    {
/*  TODO: refactor codec negotiation
        ClientCodecsHolder::Ptr codecHolder(propStorage);
        ClientCodecsHolder::Ptr requestCodecs = request.GetCodecs();
        if (codecHolder != nullptr && requestCodecs != nullptr)
        {
            codecHolder->CopyFrom(requestCodecs);
        }
*/
        if (propStorage != nullptr)
        {
            std::string optName;
            amf::AMFVariant optValue;
            for (size_t optIdx = 0; request.GetOption(optIdx, optName, optValue) == true; ++optIdx)
            {
                propStorage->SetProperty(amf::amf_from_utf8_to_unicode(optName.c_str()).c_str(), optValue);
            }
        }
    }

    const char* AMF_STD_CALL ServerSessionImpl::GetPeerUrl() const noexcept
    {
        return m_PeerAddress.c_str();
    }

    const char* AMF_STD_CALL ServerSessionImpl::GetPeerPlatform() const noexcept
    {
        return m_PeerPlatform.c_str();
    }


}