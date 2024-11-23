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

namespace ssdk::transport_amd
{
    enum class Channel
    {
        SERVICE,
        VIDEO_OUT,
        AUDIO_OUT,
        AUDIO_IN,
        SENSORS_IN,
        CONTROLLER_IN,// deprecated
        MISC_OUT,
        SENSORS_OUT,
        USER_DEFINED,
        VIDEO_IN,

        CHANNELS_COUNT,  //  Must always be the last after data channels

        SYSTEM = 255 // This is used only for sending and receiving transport requests, like requesting missing fragments
    };


    enum class SERVICE_OP_CODE
    {
        DISCOVERY, // send by client in broadcast during discovery
        CONNECTION_REFUSED,
        TRANSMISSION_FAILURE,// deprecated
        START,
        STOP,
        TRACKABLE_DEVICE_CAPS, // Client to Server (MESSAGE in old)
        UPDATE,
        HELLO,
        STAT_LATENCY,
        FORCE_IDR,  // deprecated- kept for backward compatiblity
        PROFILING_DOWNSTREAM,  // Client to Server // deprecated
        PROFILING_UPSTREAM,    // Server to Client // deprecated
        PROFILING_ACK,// deprecated
        PROFILING_NACK,// deprecated
        TERMINATE_SESSION,
        SERVER_STAT,
        CODECS_UPDATE
    };

    enum class SENSOR_OP_CODE
    {
        // items 0 through 3 are not used in the old client
        /*
        POSITION,
        CONTROLLER,
        CONTROLLER_EVENT,
        CONTROLLER_CONNECTED,
        */
        DEVICE_EVENT = 4,
        TRACKABLE_DEVICE_CAPS,
        TRACKABLE_DEVICE_DISCONNECTED,
    };

    enum class VIDEO_OP_CODE
    {
        INIT,
        DATA,
        QOS,
        CURSOR,
        FORCE_UPDATE, // replacement for VIDEO_OP_CODE::FORCE_IDR in the correct channel
        INIT_REQUEST,
        INIT_ACK
    };

    enum class AUDIO_OP_CODE
    {
        INIT,
        DATA,
        INIT_REQUEST, // request to send init data again
        INIT_ACK
    };
}