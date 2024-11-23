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

#pragma once

#include "util/pipeline/AVTransmitterAdapter.h"
#include "util/QoS/QoS.h"
#include "transports/transport-common/ServerTransport.h"

#include <memory>

namespace ssdk::video
{
    class TransmitterAdapter :
        public util::AVTransmitterAdapter
    {
    public:
        typedef std::shared_ptr<TransmitterAdapter>    Ptr;

    public:
        TransmitterAdapter(transport_common::ServerTransport::Ptr transport, transport_common::StreamID streamID, ssdk::util::QoS::Ptr QoS);

        transport_common::Result SendVideoInit(const char* codec, transport_common::InitID initID,
            const AMFSize& streamResolution, const AMFRect& viewport, uint32_t bitDepth,
            bool stereoscopic, bool foveated, amf::AMFBuffer* initBlock);
        transport_common::Result SendVideoFrame(const transport_common::TransmittableVideoFrame& frame, ssdk::util::QoS::VideoOutputStats videoOutputStats);

        transport_common::Result SendInitToSession(transport_common::SessionHandle session);

    protected:
        ssdk::util::QoS::Ptr    m_QoS;
        uint32_t                m_BitDepth = 0;
        AMFRect                 m_Viewport = {};
        AMFSize                 m_Resolution = {};
        bool                    m_Stereoscopic = false;
        bool                    m_Foveated = false;
    };
}