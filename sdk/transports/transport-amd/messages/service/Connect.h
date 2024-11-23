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

#include "transports/transport-amd/Channels.h"
#include "transports/transport-amd/messages/Message.h"
#include "transports/transport-amd/Channels.h"
#include "transports/transport-amd/FlowCtrlProtocol.h"
#include "transports/transport-amd/Codec.h"
#include "amf/public/include/core/Variant.h"
#include "amf/public/include/core/PropertyStorage.h"
#include "transports/transport-common/Transport.h"
#include <map>
#include <vector>

namespace ssdk::transport_amd
{
    class HelloRequest : public Message
    {
    protected:
        HelloRequest(SERVICE_OP_CODE eOptCode);
        HelloRequest(SERVICE_OP_CODE eOptCode,
            int32_t version,
            int32_t minVersion,
            const char* deviceID,
            size_t maxDatagramSize,
            const std::vector<VideoCodec>* videoCodecs,
            const std::vector<AudioCodec>* audioCodecs,
            amf::AMFPropertyStorage* properties);

    public:
        virtual bool FromJSON(amf::JSONParser::Node* root);

        inline int32_t GetProtocolVersion() const noexcept { return m_ProtocolVersion; }
        inline int32_t GetMinProtocolVersion() const noexcept { return m_MinProtocolVersion; }
        inline size_t GetMaxDatagramSize()  const noexcept { return m_MaxDatagramSize; }
        inline const std::string& GetDeviceID() const noexcept { return m_DeviceID; }
        inline const std::string& GetPlatformInfo() const noexcept { return m_PlatformInfo; }
        inline const std::vector<VideoCodec>& GetVideoCodecs() const noexcept { return m_VideoCodecs; }
        inline const std::vector<AudioCodec>& GetAudioCodecs() const noexcept { return m_AudioCodecs; }

        bool GetOption(const char* name, amf::AMFVariant& value) const;
        bool GetOption(size_t optIdx, std::string& name, amf::AMFVariant& value) const;

    private:
        int32_t m_ProtocolVersion = 3;     //  Version of the protocol supported by client
        int32_t m_MinProtocolVersion = 3;  //  Minimally acceptable version
        size_t m_MaxDatagramSize = FlowCtrlProtocol::MAX_DATAGRAM_SIZE;      //  Maximum datagram size
        std::string m_DeviceID;        //  Client device ID
        std::string m_PlatformInfo;    //  The platform where client app is running on.

        std::vector<VideoCodec> m_VideoCodecs;
        std::vector<AudioCodec> m_AudioCodecs;

        typedef std::map<std::string, amf::AMFVariant> OptionsMap;
        OptionsMap m_Options;
    };

    class ConnectRequest : public HelloRequest
    {
    public:
        ConnectRequest();
        ConnectRequest(int32_t version, int32_t minVersion, const char* deviceID, size_t maxDatagramSize,
            const std::vector<VideoCodec>& videoCodecs, const std::vector<AudioCodec>& audioCodecs,
            amf::AMFPropertyStorage* properties = nullptr);
    };

    class DiscoveryRequest : public HelloRequest
    {
    public:
        DiscoveryRequest();
        DiscoveryRequest(int32_t version, int32_t minVersion, const char* deviceID, size_t maxDatagramSize,
            amf::AMFPropertyStorage* properties = nullptr);
    };


    //  Responses


//---------------------------------------------------------------------------------------------
// Discovery response command
//---------------------------------------------------------------------------------------------
    class HelloResponse : public Message
    {
    public:
        class Options
        {
            friend class HelloResponse;
        public:
            Options() = default;

            inline bool GetBool(const char* name, bool& value) const { return amf::GetBoolValue(m_Root, name, value); }
            inline void SetBool(const char* name, bool value) { amf::SetBoolValue(m_Parser, m_Root, name, value); }

            inline bool GetUInt32(const char* name, uint32_t& value) const { return amf::GetUInt32Value(m_Root, name, value); }
            inline void SetUInt32(const char* name, uint32_t value) { amf::SetUInt32Value(m_Parser, m_Root, name, value); }

            inline bool GetInt32(const char* name, int32_t& value) const { return amf::GetInt32Value(m_Root, name, value); }
            inline void SetInt32(const char* name, int32_t value) { amf::SetInt32Value(m_Parser, m_Root, name, value); }

            inline bool GetInt64(const char* name, int64_t& value) const { return amf::GetInt64Value(m_Root, name, value); }
            inline void SetInt64(const char* name, int64_t value) { amf::SetInt64Value(m_Parser, m_Root, name, value); }

            inline bool GetFloat(const char* name, float& value) const { return amf::GetFloatValue(m_Root, name, value); }
            inline void SetFloat(const char* name, float value) { amf::SetFloatValue(m_Parser, m_Root, name, value); }

            inline bool GetSize(const char* name, AMFSize& value) const { return amf::GetSizeValue(m_Root, name, value); }
            inline void SetSize(const char* name, const AMFSize& value) { amf::SetSizeValue(m_Parser, m_Root, name, value); }

            inline bool GetRect(const char* name, AMFRect& value) const { return amf::GetRectValue(m_Root, name, value); }
            inline void SetRect(const char* name, const AMFRect& value) { amf::SetRectValue(m_Parser, m_Root, name, value); }

            inline bool GetRate(const char* name, AMFRate& value) const { return amf::GetRateValue(m_Root, name, value); }
            inline void SetRate(const char* name, const AMFRate& value) { amf::SetRateValue(m_Parser, m_Root, name, value); }

            inline bool GetString(const char* name, std::string& value) const { return amf::GetStringValue(m_Root, name, value); }
            inline void SetString(const char* name, const std::string& value) { amf::SetStringValue(m_Parser, m_Root, name, value); }

            inline bool GetArray(const char* name, amf::JSONParser::Array** val) const;
            inline void SetArray(const char* name, amf::JSONParser::Array* val) { m_Root->AddElement(name, val); }

            inline bool GetNode(const char* name, amf::JSONParser::Node** val) const;
            inline void SetNode(const char* name, amf::JSONParser::Node* val) { m_Root->AddElement(name, val); }

            inline bool IsPresent() const noexcept { return m_Root != nullptr; }

        private:
            inline void SetRoot(amf::JSONParser::Node* root) { m_Root = root; }
            inline void SetParser(amf::JSONParser* parser) { m_Parser = parser; }

        protected:
            amf::JSONParser::Node::Ptr m_Root;
            amf::JSONParser::Ptr m_Parser;
        };

    public:
        HelloResponse();
        HelloResponse(const std::string& name, uint32_t version, uint32_t minVersion, uint16_t port, uint32_t datagramSize, transport_common::StreamID streamID = transport_common::DEFAULT_STREAM);
        HelloResponse(const std::string& name, uint32_t version, uint32_t minVersion, uint16_t port, uint32_t datagramSize,
                      const std::vector<std::string>& supportedTransports, transport_common::StreamID streamID = transport_common::DEFAULT_STREAM);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        void UpdateData();

        inline const char*  GetName() const noexcept { return m_ServerName.c_str(); }
        inline uint32_t     GetProtocolVersion() const noexcept { return m_ProtocolVersion; }
        inline uint32_t     GetMinProtocolVersion() const noexcept { return m_MinProtocolVersion; }
        inline uint32_t     GetMaxDatagramSize() const noexcept { return m_MaxDatagramSize; }
        inline uint32_t     GetDatagramSize() const noexcept { return m_DatagramSize; }
        inline uint16_t     GetPort() const noexcept { return m_Port; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_StreamID; }
        inline const Options& GetOptions() const { return  m_Options; }
        inline Options&     GetOptions() { return  m_Options; }
        inline const char*  GetOsName() const noexcept { return m_OsName.c_str(); }

        inline const std::vector<std::string>& GetSupportedTransports() const noexcept { return m_SupportedTransports; }

    private:
        void ToJSON();

    private:
        uint32_t m_ProtocolVersion = 0;     //  Version of the protocol supported by server
        uint32_t m_MinProtocolVersion = 0;  //  Minimally acceptable version
        std::string m_ServerName;
        std::vector<std::string> m_SupportedTransports;
        uint32_t m_MaxDatagramSize = FlowCtrlProtocol::MAX_DATAGRAM_SIZE;   // Maximum datagram size
        uint32_t m_DatagramSize = FlowCtrlProtocol::MAX_DATAGRAM_SIZE;
        uint16_t m_Port = 0;
        transport_common::StreamID m_StreamID = transport_common::DEFAULT_STREAM;
        amf::JSONParser::Node::Ptr m_Root;

        std::string m_OsName;

        Options m_Options;
    };

    class HelloRefused : public Message
    {
    public:
        HelloRefused();
        virtual bool FromJSON(amf::JSONParser::Node* root) override;
    };

}
