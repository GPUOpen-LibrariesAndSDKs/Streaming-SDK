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

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "transports/transport-amd/messages/Message.h"
#include "transports/transport-common/Transport.h"

namespace ssdk::transport_amd
{
    class VideoData : public Message
    {
    public:
        VideoData();
        VideoData(amf_pts pts, amf_pts originPts, amf_pts ptsServerLatency, amf_pts ptsEncoderLatency, uint32_t compressedFrameSize,
                  transport_common::VideoFrame::ViewType eViewType, transport_common::VideoFrame::SubframeType eSubframeType, amf_pts ptsLastSendDuration, amf_uint64 uiFrameNum,
                  bool discontinuity, transport_common::StreamID streamID = transport_common::DEFAULT_STREAM);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline amf_pts GetOriginPts() const noexcept { return m_originPts; }
        inline amf_pts GetServerLatency() const noexcept { return m_ptsServerLatency; }
        inline amf_pts GetEncoderLatency() const noexcept { return m_ptsEncoderLatency; }
        inline amf_pts GetPts() const noexcept { return m_pts; }
        inline size_t GetCompressedFrameSize() const noexcept { return m_CompressedFrameSize; }
        inline transport_common::VideoFrame::ViewType GetViewType() const noexcept { return m_eViewType; }
        inline transport_common::VideoFrame::SubframeType GetSubframeType() const noexcept { return m_eSubframeType; }
        inline amf_pts GetLastSendDuration() const noexcept { return m_ptsLastSendDuration; }
        inline uint64_t GetFrameNum() const noexcept { return m_uiFrameNum; }
        inline bool GetDiscontinuity() const noexcept { return m_bDiscontinuity; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_streamID; }

    private:
        amf_pts                                             m_originPts = 0;
        amf_pts                                             m_ptsServerLatency = 0;
        amf_pts                                             m_pts = 0;
        uint32_t                                            m_CompressedFrameSize = 0;
        transport_common::VideoFrame::ViewType              m_eViewType = transport_common::VideoFrame::ViewType::MONOSCOPIC;
        transport_common::VideoFrame::SubframeType          m_eSubframeType = transport_common::VideoFrame::SubframeType::UNKNOWN;
        amf_pts                                             m_ptsEncoderLatency = 0;
        amf_pts                                             m_ptsLastSendDuration = 0;
        uint64_t                                            m_uiFrameNum = 0;
        bool                                                m_bDiscontinuity = true;
        transport_common::StreamID                          m_streamID = transport_common::DEFAULT_STREAM;
    };

    class VideoForceUpdate : public Message
    {
    public:
        VideoForceUpdate();
        VideoForceUpdate(transport_common::StreamID videoStreamID);
        virtual bool FromJSON(amf::JSONParser::Node*) override;

        inline transport_common::StreamID GetStreamID() const noexcept { return m_videoStreamID; }

    private:
        transport_common::StreamID m_videoStreamID = transport_common::DEFAULT_STREAM;
    };

}
