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
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#include "AVTransmitterAdapter.h"

namespace ssdk::util
{
    AVTransmitterAdapter::AVTransmitterAdapter(transport_common::ServerTransport::Ptr transport, transport_common::StreamID streamID) :
        m_Transport(transport),
        m_StreamID(streamID)
    {
    }

    bool AVTransmitterAdapter::RegisterSession(transport_common::SessionHandle session)
    {
        bool result = false;
        amf::AMFLock lock(&m_Guard);
        SessionToInitIDMap::iterator found = m_SessionToInitIDMap.find(session);
        if (found == m_SessionToInitIDMap.end())
        {
            m_SessionToInitIDMap.insert({ session, transport_common::INVALID_INIT_ID });
            result = true;
        }
        return result;
    }

    void AVTransmitterAdapter::UnregisterSession(transport_common::SessionHandle session)
    {
        amf::AMFLock lock(&m_Guard);
        m_SessionToInitIDMap.erase(session);
    }

    bool AVTransmitterAdapter::UpdateSession(transport_common::SessionHandle session, transport_common::InitID initID)
    {
        bool result = false;
        amf::AMFLock lock(&m_Guard);
        SessionToInitIDMap::iterator found = m_SessionToInitIDMap.find(session);
        if (found != m_SessionToInitIDMap.end())
        {
            found->second = initID;
            result = true;
        }
        return result;
    }

    void AVTransmitterAdapter::SetInitBlock(const std::string& codec, amf::AMFBuffer* initBlock, transport_common::InitID initID)
    {
        amf::AMFLock lock(&m_Guard);
        m_InitBlock = initBlock;
        m_InitID = initID;
        m_Codec = codec;
    }
}