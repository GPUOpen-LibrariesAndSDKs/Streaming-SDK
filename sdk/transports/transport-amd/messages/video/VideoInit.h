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
#include <string>

namespace ssdk::transport_amd
{
    class VideoInit : public Message
    {
    public:
        VideoInit();
        VideoInit(amf_pts id, const char* codecID, int32_t width, int32_t height, AMFRect viewport, bool bNonLinearScaling, int32_t bitDepth, transport_common::StreamID streamID);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline int32_t GetWidth() const noexcept { return m_iWidth; }
        inline int32_t GetHeight() const noexcept { return m_iHeight; }
        inline AMFRect GetViewport() const noexcept { return m_Viewport; }
        inline const std::string& GetCodecID() const noexcept { return m_CodecID; }
        inline bool GetNonLinearScaling() const noexcept { return m_bNonLinearScaling; }
        inline int32_t GetBitDepth() const noexcept { return m_iBitDepth; }
        inline amf_pts GetID() const noexcept { return m_ID; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_streamID; }

    private:
        int32_t         m_iWidth = 0;
        int32_t         m_iHeight = 0;
        AMFRect         m_Viewport = AMFConstructRect(0, 0, 0, 0);
        std::string     m_CodecID;
        bool            m_bNonLinearScaling = false;
        int32_t         m_iBitDepth = 8;
        amf_pts         m_ID = 0;
        transport_common::StreamID m_streamID = transport_common::DEFAULT_STREAM;
    };

    class VideoInitAck : public Message
    {
    public:
        VideoInitAck();
        VideoInitAck(amf_pts id, bool ack, transport_common::StreamID streamID);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline amf_pts GetID() const noexcept { return m_ID; }
        inline bool GetAck() const noexcept { return m_Ack; }

        inline transport_common::StreamID GetStreamID() const noexcept { return m_streamID; }

    private:
        amf_pts     m_ID = 0;
        bool        m_Ack = false;
        transport_common::StreamID m_streamID = transport_common::DEFAULT_STREAM;
    };

    class VideoInitRequest : public Message
    {
    public:
        VideoInitRequest();
        VideoInitRequest(transport_common::StreamID streamID);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline transport_common::StreamID GetStreamID() const noexcept { return m_streamID; }

    private:
        transport_common::StreamID m_streamID = transport_common::DEFAULT_STREAM;
    };

}
