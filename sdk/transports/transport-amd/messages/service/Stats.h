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
    class Statistics : public Message
    {
    public:
        Statistics();
        Statistics(transport_common::StreamID streamID, float full, float client, float server, float encoder, float network, float decoder, float encryption, float decryption, int32_t decoderQueue, float avDesync, float fpsRx);
        Statistics(transport_common::StreamID streamID, int32_t decoderQueue, float fpsRx, float full);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline float GetFull() const noexcept { return m_Full; }
        inline float GetClient() const noexcept { return m_Client; }
        inline float GetServer() const noexcept { return m_Server; }
        inline float GetEncoder() const noexcept { return m_Encoder; }
        inline float GetDecoder() const noexcept { return m_Decoder; }
        inline float GetNetwork() const noexcept { return m_Network; }
        inline int32_t GetDecoderQueueDepth() const noexcept { return m_DecoderQueue; }
        inline float GetEncryptionLatency() const noexcept { return m_EncrLatency; }
        inline float GetDecryptionLatency() const noexcept { return m_DecrLatency; }
        inline float GetAVDesync() const noexcept { return m_AVDesync; }
        inline float GetFpsRx() const noexcept { return m_FpsRx; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_StreamID; }

    private:
        float m_Full = 0;
        float m_Client = 0;
        float m_Server = 0;
        float m_Encoder = 0;
        float m_Network = 0;
        float m_Decoder = 0;
        int32_t m_DecoderQueue = 0;
        float m_EncrLatency = 0;
        float m_DecrLatency = 0;
        float m_AVDesync = 0;
        float m_FpsRx = 0;
        transport_common::StreamID m_StreamID = transport_common::DEFAULT_STREAM;
    };

    class ServerStat : public Message
    {
    public:
        ServerStat();
        ServerStat(transport_common::StreamID streamID, int32_t gpuClk, int32_t gpuUsage, int32_t gpuTemp, int32_t cpuClk, int32_t cpuTemp);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline int32_t GetGpuClk() const noexcept { return m_gpuClk; }
        inline int32_t GetGpuUsage() const noexcept { return m_gpuUsage; }
        inline int32_t GetGpuTemp() const noexcept { return m_gpuTemp; }
        inline int32_t GetCpuClk() const noexcept { return m_cpuClk; }
        inline int32_t GetCpuTemp() const noexcept { return m_cpuTemp; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_StreamID; }

    private:
        int32_t m_gpuClk = 0;
        int32_t m_gpuUsage = 0;
        int32_t m_gpuTemp = 0;
        int32_t m_cpuClk = 0;
        int32_t m_cpuTemp = 0;
        transport_common::StreamID m_StreamID = transport_common::DEFAULT_STREAM;
    };
}
