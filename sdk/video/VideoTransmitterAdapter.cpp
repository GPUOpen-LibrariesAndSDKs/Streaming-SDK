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

#include "VideoTransmitterAdapter.h"

namespace ssdk::video
{
    TransmitterAdapter::TransmitterAdapter(transport_common::ServerTransport::Ptr transport, transport_common::StreamID streamID, ssdk::util::QoS::Ptr QoS) :
        AVTransmitterAdapter(transport, streamID),
        m_QoS(QoS)
    {
    }

    transport_common::Result TransmitterAdapter::SendVideoInit(const char* codec, transport_common::InitID initID, const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth, bool stereoscopic, bool foveated, amf::AMFBuffer* initBlock)
    {
        SessionToInitIDMap sessions;
        {
            amf::AMFLock lock(&m_Guard);
            sessions = m_SessionToInitIDMap;
            SetInitBlock(codec, initBlock, initID);
            m_Viewport = viewport;
            m_Resolution = streamResolution;
            m_BitDepth = bitDepth;
            m_Stereoscopic = stereoscopic;
            m_Foveated = foveated;
        }
        transport_common::Result result = transport_common::Result::OK;
        for (auto it : sessions)
        {
            transport_common::Result res = SendInitToSession(it.first);
            if (res != transport_common::Result::OK)
            {
                result = res;
            }
        }
        return result;
    }

    transport_common::Result TransmitterAdapter::SendInitToSession(transport_common::SessionHandle session)
    {
        transport_common::Result result = transport_common::Result::OK;
        const void* initBlockData = nullptr;
        size_t initBlockSize = 0;
        transport_common::InitID initID;
        uint32_t                bitDepth = 0;
        AMFRect                 viewport = {};
        AMFSize                 resolution = {};
        bool                    stereoscopic = false;
        bool                    foveated = false;
        {
            amf::AMFLock lock(&m_Guard);
            if (m_InitBlock != nullptr)
            {
                initBlockData = m_InitBlock->GetNative();
                initBlockSize = m_InitBlock->GetSize();
            }
            initID = m_InitID;
            bitDepth = m_BitDepth;
            viewport = m_Viewport;
            resolution = m_Resolution;
            stereoscopic = m_Stereoscopic;
            foveated = m_Foveated;
        }
        if (initID != -1)
        {
            result = m_Transport->SendVideoInit(session, m_Codec.c_str(), m_StreamID, initID, resolution, viewport, bitDepth, stereoscopic, foveated, initBlockData, initBlockSize);
        }
        return result;
    }

    transport_common::Result TransmitterAdapter::SendVideoFrame(const transport_common::TransmittableVideoFrame& frame, ssdk::util::QoS::VideoOutputStats videoOutputStats)
    {
        SessionToInitIDMap sessions;
        transport_common::InitID initID;
        {
            amf::AMFLock lock(&m_Guard);
            sessions = m_SessionToInitIDMap;
            initID = m_InitID;
        }

        transport_common::Result result = transport_common::Result::OK;
        for (auto it : sessions)
        {
            if (it.second == initID)
            {
                transport_common::Result res = m_Transport->SendVideoFrame(it.first, m_StreamID, frame);
                if (res != transport_common::Result::OK)
                {
                    result = res;
                }
            }
        }
        
        if (m_QoS != nullptr)
        {
            m_QoS->AdjustStreamQuality(videoOutputStats);
        }
        return result;
    }
}
