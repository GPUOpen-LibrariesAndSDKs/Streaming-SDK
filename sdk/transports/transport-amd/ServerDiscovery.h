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

#include "TransportClient.h"
#include "messages/service/Connect.h"

#include "net/DatagramClient.h"
#include "net/Url.h"
#include "DgramClientSessionFlowCtrl.h"
#include "public/common/InterfaceImpl.h"
#include <vector>
#include <map>
#include <string>

namespace ssdk::transport_amd
{
    //---------------------------------------------------------------------------------------------
    class ServerParametersImpl : public amf::AMFInterfaceImpl<ServerParameters>
    {
    public:
        typedef std::vector<ServerParameters::Ptr> Collection;

        static const char* OPTION_MICROPHONE;
        static const char* OPTION_HORIZONTAL_FOV;
        static const char* OPTION_VERTICAL_FOV;
        static const char* OPTION_ENCODER_FRAME_SIZE_EYE;
        static const char* OPTION_KEYBOARD;
        static const char* OPTION_MOUSE;
        static const char* OPTION_TOUCHSCREEN;
        static const char* OPTION_PROFILE_PAYLOAD_SIZE;
        static const char* OPTION_ENCRYPTION;

    public:
        ServerParametersImpl(HelloResponse& serverDescriptor, const std::string& transport, const net::Socket::Address& address);
        ServerParametersImpl(HelloResponse& serverDescriptor, const net::Url& url);

        virtual const char* AMF_STD_CALL GetName() const override;
        virtual const char* AMF_STD_CALL GetUrl() const override;
        virtual int32_t     AMF_STD_CALL GetVersion() const override;
        virtual size_t      AMF_STD_CALL GetDatagramSize() const override;
        virtual bool        AMF_STD_CALL GetEncryption() const override;
        virtual const char* AMF_STD_CALL GetOsName() const override;

        virtual bool AMF_STD_CALL GetOptionBool(const char* name, bool& value) const override;
        virtual bool AMF_STD_CALL GetOptionUInt32(const char* name, uint32_t& value) const override;
        virtual bool AMF_STD_CALL GetOptionInt32(const char* name, int32_t& value) const override;
        virtual bool AMF_STD_CALL GetOptionInt64(const char* name, int64_t& value) const override;
        virtual bool AMF_STD_CALL GetOptionFloat(const char* name, float& value) const override;
        virtual bool AMF_STD_CALL GetOptionSize(const char* name, AMFSize& value) const override;
        virtual bool AMF_STD_CALL GetOptionRect(const char* name, AMFRect& value) const override;
        virtual bool AMF_STD_CALL GetOptionRate(const char* name, AMFRate& value) const override;
        virtual bool AMF_STD_CALL GetOptionString(const char* name, std::string& value) const override;

    private:
        std::string                 m_Name;
        std::string                 m_Url;
        std::string                 m_OsName;
        int32_t                     m_Version = 0;
        size_t                      m_DatagramSize = 0;
        HelloResponse::Options		m_Options;
    };
    //---------------------------------------------------------------------------------------------
    class ServerDiscoverySession :
        public amf::AMFInterfaceBase,
        public DatagramClientSessionFlowCtrl
    {
    public:
        typedef amf::AMFInterfacePtr_T<ServerDiscoverySession> Ptr;
        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(ServerDiscoverySession)
            AMF_INTERFACE_MULTI_ENTRY(DatagramClientSessionFlowCtrl)
            AMF_INTERFACE_MULTI_ENTRY(net::DatagramClientSession)
            AMF_INTERFACE_MULTI_ENTRY(net::Session)
       AMF_END_INTERFACE_MAP

    public:
        ServerDiscoverySession(const std::string& deviceID, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize, ServerParametersImpl::Collection& servers, unsigned short port);

        Result EnumerateServers(ServerEnumCallback* callback, size_t *size, amf::AMFPropertyStoragePtr options);
        Result QueryServerInfo(ServerEnumCallback* callback, amf::AMFPropertyStoragePtr options);

    protected:
        virtual Result AMF_STD_CALL OnInit() override;
        virtual void OnCompleteMessage(FlowCtrlProtocol::MessageID msgID, const void* buf, size_t size, const net::Socket::Address& receivedFrom, uint8_t optional) override;
        virtual Result AMF_STD_CALL OnSessionTimeout() override;
        virtual net::Session::Result   AMF_STD_CALL OnSessionClose() override;
        virtual Result OnTerminate();
    private:
        ServerParametersImpl::Collection&               m_Servers;
        ServerEnumCallback*                             m_Callback = nullptr;
        volatile ServerEnumCallback::DiscoveryCtrl      m_KeepLooking = ServerEnumCallback::DiscoveryCtrl::CONTINUE;
        volatile DatagramClientSessionFlowCtrl::Result  m_LastResult = DatagramClientSessionFlowCtrl::Result::OK;
        std::string                                     m_ClientDeviceID;
        size_t                                          m_ReceiveBufSize = 0;
    };
    //---------------------------------------------------------------------------------------------
    class DiscoveryClient :
        public net::DatagramClient
    {
    public:
        typedef std::shared_ptr<DiscoveryClient>    Ptr;
        typedef std::shared_ptr<const DiscoveryClient>    CPtr;
    public:
        DiscoveryClient(const std::string& deviceID, ServerParametersImpl::Collection& servers, unsigned short port);

    protected:
        virtual net::ClientSession::Ptr AMF_STD_CALL OnCreateSession(Client& client, const net::Socket::Address& peer, net::Socket* connectSocket, void* params);

    private:
        DiscoveryClient(const DiscoveryClient&) = delete;
        DiscoveryClient& operator=(const DiscoveryClient&) = delete;

    private:
        ServerParametersImpl::Collection& m_Servers;
        unsigned short	m_Port;
        std::string		m_DeviceID;

    };
    //---------------------------------------------------------------------------------------------
}
