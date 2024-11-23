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

#include "AudioTransmitterAdapter.h"

namespace ssdk::audio
{
    TransmitterAdapter::TransmitterAdapter(ssdk::transport_common::ServerTransport::Ptr transport, ssdk::transport_common::StreamID streamID) :
        AVTransmitterAdapter(transport, streamID)
    {
    }

    ssdk::transport_common::Result TransmitterAdapter::SendAudioInit(const char* codec, ssdk::transport_common::InitID initID, uint32_t channels, uint32_t layout, amf::AMF_AUDIO_FORMAT format, uint32_t samplingRate, amf::AMFBuffer* initBlock)
    {
        SessionToInitIDMap sessions;
        {
            amf::AMFLock lock(&m_Guard);
            sessions = m_SessionToInitIDMap;
            SetInitBlock(codec, initBlock, initID);
            m_Channels = channels;
            m_Format = format;
            m_Layout = layout;
            m_SamplingRate = samplingRate;
        }
        ssdk::transport_common::Result result = ssdk::transport_common::Result::OK;
        for (auto it : sessions)
        {
            ssdk::transport_common::Result res = SendInitToSession(it.first);
            if (res != ssdk::transport_common::Result::OK)
            {
                result = res;
            }
        }

        return result;
    }

    ssdk::transport_common::Result TransmitterAdapter::SendAudioBuffer(const ssdk::transport_common::TransmittableAudioBuffer& buf)
    {
        SessionToInitIDMap sessions;
        ssdk::transport_common::InitID initID;
        {
            amf::AMFLock lock(&m_Guard);
            sessions = m_SessionToInitIDMap;
            initID = m_InitID;
        }
        ssdk::transport_common::Result result = ssdk::transport_common::Result::OK;
        for (auto it : sessions)
        {
            if (it.second == initID)
            {
                ssdk::transport_common::Result res = m_Transport->SendAudioBuffer(it.first, m_StreamID, buf);
                if (res != ssdk::transport_common::Result::OK)
                {
                    result = res;
                }
            }
        }
        return result;
    }

    ssdk::transport_common::Result TransmitterAdapter::SendInitToSession(ssdk::transport_common::SessionHandle session)
    {
        ssdk::transport_common::Result result = ssdk::transport_common::Result::OK;
        const void* initBlockData = nullptr;
        size_t initBlockSize = 0;
        ssdk::transport_common::InitID initID;
        int32_t                 channels = 0;
        int32_t                 layout = 0;
        int32_t                 samplingRate = 0;
        amf::AMF_AUDIO_FORMAT   format = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
        {
            amf::AMFLock lock(&m_Guard);
            if (m_InitBlock != nullptr)
            {
                initBlockData = m_InitBlock->GetNative();
                initBlockSize = m_InitBlock->GetSize();
            }
            initID = m_InitID;
            channels = m_Channels;
            layout = m_Layout;
            samplingRate = m_SamplingRate;
            format = m_Format;
        }
        if (initID != -1)
        {
            result = m_Transport->SendAudioInit(session, m_Codec.c_str(), m_StreamID, initID, channels, layout, format, samplingRate, initBlockData, initBlockSize);
        }
        return result;
    }
}