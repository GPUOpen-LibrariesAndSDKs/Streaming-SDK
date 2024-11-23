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

#include "TransportSession.h"
#include "messages/service/Connect.h"
#include "amf/public/include/core/PropertyStorageEx.h"
#include <time.h>

namespace ssdk::transport_amd
{
    class OnFillOptionsCallback
    {
    public:
        virtual transport_common::Result     AMF_STD_CALL OnFillOptions(bool discovery, Session* session, HelloResponse::Options* options) = 0;
    };
        //---------------------------------------------------------------------------------------------
    class OnClientConnectCallback
    {
    public:

        virtual transport_common::Result AMF_STD_CALL OnDiscovery(Session* session) = 0;
        virtual transport_common::Result AMF_STD_CALL OnClientConnected(Session* session) = 0;
        virtual bool                     AMF_STD_CALL AuthorizeDiscoveryRequest(Session* session, const char* deviceID) = 0;
        virtual bool                     AMF_STD_CALL AuthorizeConnectionRequest(Session* session, const char* deviceID) = 0;
    };
    //---------------------------------------------------------------------------------------------
    class Server : public amf::AMFPropertyStorageEx
    {
    public:
        typedef amf::AMFInterfacePtr_T<Server> Ptr;
        AMF_DECLARE_IID(0x27cd6fb5, 0x911d, 0x4c7c, 0x8d, 0x0, 0x9b, 0x82, 0x60, 0x3c, 0xf0, 0xf1);

        virtual transport_common::Result AMF_STD_CALL SetName(const char* name) = 0;
        virtual transport_common::Result AMF_STD_CALL SetDatagramSize(size_t size) = 0;
        //  Run the server in a thread
        virtual transport_common::Result AMF_STD_CALL Activate(const char* url, OnClientConnectCallback* connectCallback) = 0;
        virtual transport_common::Result AMF_STD_CALL Deactivate() = 0;

        virtual transport_common::Result AMF_STD_CALL SetOptionProvider(OnFillOptionsCallback* provider) = 0;
        virtual void             AMF_STD_CALL SetSessionTimeoutEnabled(bool timeoutEnabled) = 0;
    };

    //----------------------------------------------------------------------------------------------
    // Server properties
    //----------------------------------------------------------------------------------------------
    // Datagram messages monitoring
    extern const wchar_t* DATAGRAM_MSG_INTERVAL;            // amf_int64; default = 10; the interval in seconds for monitoring lost messages due to UDP Datagram size limitations
    extern const wchar_t* DATAGRAM_LOST_MSG_THRESHOLD;      // amf_int64; default = 10; the interval in seconds for monitoring lost messages due to UDP Datagram size limitations
    extern const wchar_t* DATAGRAM_TURNING_POINT_THRESHOLD; // amf_int64; default = 20; the turning point threshold for finding optimal max fragment size of messages sending by UDP

    //----------------------------------------------------------------------------------------------
    // Statistics properties
    //----------------------------------------------------------------------------------------------
    extern const wchar_t* STATISTICS_FULL_LATENCY;              // amf_float; complete latency for the video frame, from captured on server to presented on client (ms)
    extern const wchar_t* STATISTICS_SERVER_LATENCY;            // amf_float; complete latency on the server side (ms)
    extern const wchar_t* STATISTICS_ENCODER_LATENCY;           // amf_float; latency added by encoding the video frame (ms)
    extern const wchar_t* STATISTICS_CLIENT_LATENCY;            // amf_float; complete latency on the client side (ms)
    extern const wchar_t* STATISTICS_NETWORK_LATENCY;           // amf_float; latency added by network transport (ms)
    extern const wchar_t* STATISTICS_DECODER_LATENCY;           // amf_float; latency added by decoding on video frame (ms)
    extern const wchar_t* STATISTICS_ENCODER_QUEUE_DEPTH;       // amf_int64; the number of frames pending in the encoder queue on the server
    extern const wchar_t* STATISTICS_DECODER_QUEUE_DEPTH;       // amf_int64; the number of frames pending in the decoder queue on the client
    extern const wchar_t* STATISTICS_ENCRYPTION_LATENCY;        // amf_float; latency added by encryption (ms)
    extern const wchar_t* STATISTICS_DECRYPTION_LATENCY;        // amf_float; latency added by decryption (ms)
    extern const wchar_t* STATISTICS_CLIENT_ENCRYPTION_LATENCY; // amf_float; latency added by encryption on the client (ms)
    extern const wchar_t* STATISTICS_CLIENT_DECRYPTION_LATENCY; // amf_float; latency added by decryption on the client (ms)
    extern const wchar_t* STATISTICS_UPDATE_TIME;               // amf_pts; time of the last update in 100s of ns

    extern const wchar_t* STATISTICS_BANDWIDTH_VIDEO_OUT;       // amf_float; video out bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_VIDEO_IN;        // amf_float; video in bandwidth (bps)
    extern const wchar_t* STATISTICS_VIDEO_SEND_TIME;           // amf_float; average time in send_to in ms
    extern const wchar_t* STATISTICS_VIDEO_FPS_AT_TX;           // amf_float; real fps measured at transmission point
    extern const wchar_t* STATISTICS_VIDEO_FPS_AT_RX;           // amf_float; real fps measured at receive point
    extern const wchar_t* STATISTICS_BANDWIDTH_AUDIO_OUT;       // amf_float; audio out bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_AUDIO_IN;        // amf_float; audio in bandwidth (bps)
    extern const wchar_t* STATISTICS_AUDIO_SEND_TIME;           // amf_float; average time in send_to in ms
    extern const wchar_t* STATISTICS_BANDWIDTH_CTRL_OUT;        // amf_float; sensors out bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_CTRL_IN;         // amf_float; controllers/kbd/mouse/touchscreen bandwidth (bps)
    extern const wchar_t* STATISTICS_CTRL_SEND_TIME;            // amf_float; average time in send_to in ms
    extern const wchar_t* STATISTICS_BANDWIDTH_USER_OUT;        // amf_float; user-defined messages out bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_USER_IN;         // amf_float; user-defined messages in bandwidth (bps)
    extern const wchar_t* STATISTICS_USER_SEND_TIME;            // amf_float; average time in send_to in ms
    extern const wchar_t* STATISTICS_TOTAL_SEND_TIME;           // amf_float; average time in send_to in ms
    extern const wchar_t* STATISTICS_BANDWIDTH_TOTAL_OUT;       // amf_float; total messages out bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_TOTAL_IN;        // amf_float; total messages in bandwidth (bps)
    extern const wchar_t* STATISTICS_BANDWIDTH_ESTIMATE;        // amf_float; currently used total network bandwidth (bps)
    extern const wchar_t* STATISTICS_FORCE_IDR_REQ_COUNT;       // amf_int64; count of ForceIDR requests since last statistics
    extern const wchar_t* STATISTICS_SLOW_SEND_COUNT;           // amf_int64; count of sends that took longer than 50ms
    extern const wchar_t* STATISTICS_WORST_SEND_TIME;           // amf_float; worst send time in ms

    extern const wchar_t* STATISTICS_AV_DESYNC;                 // amf_float; average audio-video desync (video-audio) in ms

    extern const wchar_t* STATISTICS_LOCAL_UPDATE_TIME;         // amf_pts; time of the last update in 100s of ns

    extern const wchar_t* STATISTICS_TICK_PERIOD;               // ANSClient only. amf_pts. Controls the minimum period of ticks being sent to the client. Default: 0 (once a ms)

    extern const wchar_t* STATISTICS_SERVER_GPU_CLK;            // amf_float; current server gpu core clock (mhz)
    extern const wchar_t* STATISTICS_SERVER_GPU_USAGE;          // amf_float; current server gpu utilisation in percentage (%)
    extern const wchar_t* STATISTICS_SERVER_GPU_TEMP;           // amf_float; current server gpu core temperature (C)
    extern const wchar_t* STATISTICS_SERVER_CPU_CLK;            // amf_float; current server cpu core clock (MHz)
    extern const wchar_t* STATISTICS_SERVER_CPU_TEMP;           // amf_float; current server cpu core temperature (C)
    extern const wchar_t* SERVER_STATISTICS_PERIOD;             // amf_pts; default = 3 seconds. the length of time period that server gpu statistics are sent

    //---------------------------------------------------------------------------------------------
}
