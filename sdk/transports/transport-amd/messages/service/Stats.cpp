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

#include "Stats.h"
#include "transports/transport-amd/Channels.h"


namespace ssdk::transport_amd
{
    static constexpr const char* TAG_STAT_FULL = "Full";
    static constexpr const char* TAG_STAT_CLIENT = "Client";
    static constexpr const char* TAG_STAT_SERVER = "Server";
    static constexpr const char* TAG_STAT_ENCODER = "Encoder";
    static constexpr const char* TAG_STAT_DECODER = "Decoder";
    static constexpr const char* TAG_STAT_DECODER_QUEUE = "DecQueue";
    static constexpr const char* TAG_STAT_NETWORK = "Network";
    static constexpr const char* TAG_STAT_ENCRYPT = "Encr";
    static constexpr const char* TAG_STAT_DECRYPT = "Decr";
    static constexpr const char* TAG_STAT_AVDESYNC = "AVDesync";
    static constexpr const char* TAG_STAT_FPS_RX = "FpsR";

    static constexpr const char* TAG_STAT_GPU_CLOCK = "GpuClock";
    static constexpr const char* TAG_STAT_GPU_USAGE = "GpuUsage";
    static constexpr const char* TAG_STAT_GPU_TEMP = "GpuTemp";
    static constexpr const char* TAG_STAT_CPU_CLOCK = "CpuCLock";
    static constexpr const char* TAG_STAT_CPU_TEMP = "CpuTemp";

    static constexpr const char* TAG_STREAM_ID = "StreamID";

    Statistics::Statistics() :
        Message(uint8_t(SERVICE_OP_CODE::STAT_LATENCY))
    {
    }

    Statistics::Statistics(transport_common::StreamID streamID, float Full, float client, float server, float encoder, float network, float decoder, float encryption, float decryption, int32_t decoderQueue, float avDesync, float fpsRx) :
        Message(uint8_t(SERVICE_OP_CODE::STAT_LATENCY)),
        m_Full(Full),
        m_Client(client),
        m_Server(server),
        m_Encoder(encoder),
        m_Network(network),
        m_Decoder(decoder),
        m_EncrLatency(encryption),
        m_DecrLatency(decryption),
        m_DecoderQueue(decoderQueue),
        m_AVDesync(avDesync),
        m_FpsRx(fpsRx),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (m_Full != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_FULL, m_Full);
        }
        if (m_Client != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_CLIENT, m_Client);
        }
        if (m_Server != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_SERVER, m_Server);
        }
        if (m_Encoder != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_ENCODER, m_Encoder);
        }
        if (m_Decoder != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_DECODER, m_Decoder);
        }
        if (m_Network != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_NETWORK, m_Network);
        }
        if (m_DecoderQueue != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_DECODER_QUEUE, m_DecoderQueue);
        }
        if (m_EncrLatency != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_ENCRYPT, m_EncrLatency);
        }
        if (m_DecrLatency != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_DECRYPT, m_DecrLatency);
        }
        if (m_AVDesync != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_AVDESYNC, m_AVDesync);
        }
        if (m_FpsRx != 0)
        {
            SetFloatValue(parser, root, TAG_STAT_FPS_RX, m_FpsRx);
        }
        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    bool Statistics::FromJSON(amf::JSONParser::Node* root)
    {
        GetFloatValue(root, TAG_STAT_FULL, m_Full);
        GetFloatValue(root, TAG_STAT_CLIENT, m_Client);
        GetFloatValue(root, TAG_STAT_SERVER, m_Server);
        GetFloatValue(root, TAG_STAT_ENCODER, m_Encoder);
        GetFloatValue(root, TAG_STAT_DECODER, m_Decoder);
        GetFloatValue(root, TAG_STAT_NETWORK, m_Network);
        GetInt32Value(root, TAG_STAT_DECODER_QUEUE, m_DecoderQueue);
        GetFloatValue(root, TAG_STAT_ENCRYPT, m_EncrLatency);
        GetFloatValue(root, TAG_STAT_DECRYPT, m_DecrLatency);
        GetFloatValue(root, TAG_STAT_AVDESYNC, m_AVDesync);
        GetFloatValue(root, TAG_STAT_FPS_RX, m_FpsRx);
        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    ServerStat::ServerStat() :
        Message(uint8_t(SERVICE_OP_CODE::SERVER_STAT))
    {
    }

    ServerStat::ServerStat(transport_common::StreamID streamID, int32_t gpuClk, int32_t gpuUsage, int32_t gpuTemp, int32_t cpuClk, int32_t cpuTemp) :
        Message(uint8_t(SERVICE_OP_CODE::SERVER_STAT)),
        m_gpuClk(gpuClk),
        m_gpuUsage(gpuUsage),
        m_gpuTemp(gpuTemp),
        m_cpuClk(cpuClk),
        m_cpuTemp(cpuTemp),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (m_gpuClk != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_GPU_CLOCK, m_gpuClk);
        }
        if (m_gpuUsage != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_GPU_USAGE, m_gpuUsage);
        }
        if (m_gpuTemp != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_GPU_TEMP, m_gpuTemp);
        }
        if (m_cpuClk != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_CPU_CLOCK, m_cpuClk);
        }
        if (m_cpuTemp != 0)
        {
            SetInt32Value(parser, root, TAG_STAT_CPU_TEMP, m_cpuTemp);
        }
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    bool ServerStat::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt32Value(root, TAG_STAT_GPU_CLOCK, m_gpuClk);
        GetInt32Value(root, TAG_STAT_GPU_USAGE, m_gpuUsage);
        GetInt32Value(root, TAG_STAT_GPU_TEMP, m_gpuTemp);
        GetInt32Value(root, TAG_STAT_CPU_CLOCK, m_cpuClk);
        GetInt32Value(root, TAG_STAT_CPU_TEMP, m_cpuTemp);
        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }
}
