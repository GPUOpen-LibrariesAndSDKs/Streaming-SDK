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
#include "Codec.h"
#include "amf/public/include/core/PropertyStorageEx.h"

#include <string>
#include <time.h>

namespace ssdk::transport_amd
{
    //---------------------------------------------------------------------------------------------
    //  Server discovery:
    //  An interface to a server descriptor:
    //---------------------------------------------------------------------------------------------
    class ServerParameters : public amf::AMFInterface
    {
    public:
        typedef amf::AMFInterfacePtr_T<ServerParameters> Ptr;
        AMF_DECLARE_IID(0x1fe24dfe, 0x843f, 0x4983, 0x8f, 0x35, 0x6a, 0x6f, 0xe8, 0xe5, 0x45, 0x4);

        virtual const char* AMF_STD_CALL GetName() const = 0;
        virtual const char* AMF_STD_CALL GetUrl() const = 0;
        virtual int32_t     AMF_STD_CALL GetVersion() const = 0;
        virtual size_t      AMF_STD_CALL GetDatagramSize() const = 0;
        virtual bool        AMF_STD_CALL GetEncryption() const = 0;
        virtual const char* AMF_STD_CALL GetOsName() const = 0;

        virtual bool AMF_STD_CALL GetOptionBool(const char* name, bool& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionUInt32(const char* name, uint32_t& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionInt32(const char* name, int32_t& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionInt64(const char* name, int64_t& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionFloat(const char* name, float& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionSize(const char* name, AMFSize& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionRect(const char* name, AMFRect& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionRate(const char* name, AMFRate& value) const = 0;
        virtual bool AMF_STD_CALL GetOptionString(const char* name, std::string& value) const = 0;
    };

    //---------------------------------------------------------------------------------------------
    //  An optional callback for EnumerateServers: called every time a new server is discovered
    //  This can be useful when there's a UI to update as servers respond to a discovery broadcast message
    //---------------------------------------------------------------------------------------------
    class ServerEnumCallback
    {
    public:
        enum class DiscoveryCtrl
        {
            CONTINUE,
            ABORT
        };

        virtual DiscoveryCtrl    AMF_STD_CALL OnServerDiscovered(ServerParameters* server) = 0;
        virtual DiscoveryCtrl    AMF_STD_CALL OnConnectionRefused() = 0;
    };
    //---------------------------------------------------------------------------------------------
    // Client
    //---------------------------------------------------------------------------------------------
    class Client : public amf::AMFPropertyStorage
    {
    public:
        typedef amf::AMFInterfacePtr_T<Client> Ptr;
        AMF_DECLARE_IID(0x395374c9, 0xd73a, 0x4aee, 0x92, 0x8, 0xbe, 0x4c, 0xc7, 0xff, 0xc2, 0xc4);

        //  Discovery methods:
        virtual transport_common::Result     AMF_STD_CALL EnumerateServers(const char* clientID, unsigned short port, time_t timeoutSec, size_t* const numOfServers, ServerEnumCallback* callback) = 0;
        virtual transport_common::Result     AMF_STD_CALL GetServerParameters(size_t serverIdx, ServerParameters** params) = 0;
        virtual transport_common::Result     AMF_STD_CALL QueryServerInfo(const char* clientID, const char* url, unsigned short port, time_t timeout, ServerEnumCallback* callback) = 0;

        //  Connection methods:
		virtual transport_common::Result     AMF_STD_CALL ConnectToServerAndQueryParameters(const char* url, const char* clientID, Session** session, ServerParameters** params) = 0;
		virtual transport_common::Result     AMF_STD_CALL SetTimeout(time_t timeoutSec) = 0; // connection timeout

        virtual transport_common::Result     AMF_STD_CALL AddVideoCodec(const VideoCodec& codec) = 0;
        virtual transport_common::Result     AMF_STD_CALL AddAudioCodec(const AudioCodec& codec) = 0;

        //  Run the client service in a thread
        virtual transport_common::Result     AMF_STD_CALL Activate() = 0;
        virtual transport_common::Result     AMF_STD_CALL Deactivate() = 0;
    };
}
