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

#include "Connect.h"
#include "amf/public/common/AMFSTL.h"
#include "amf/public/common/TraceAdapter.h"

namespace ssdk::transport_amd
{
    static constexpr const wchar_t* TRACE_SCOPE = L"Connect";

#if defined(_WIN32)
    static constexpr const char* PLATFORM = "Windows";
#elif defined (__APPLE__)
    static constexpr const char* PLATFORM = "iOS";
#elif defined (__ANDROID__) || defined(ANDROID)
    static constexpr const char* PLATFORM = "Android";
#elif defined (__linux)
    static constexpr const char* PLATFORM = "Linux";
#else
    static constexpr const char* PLATFORM = "Unknown";
#endif


    static constexpr const char* TAG_PROTOCOL_VERSION = "ProtocolVersion";
    static constexpr const char* TAG_PROTOCOL_MIN_VERSION = "ProtocolMinVersion";
    static constexpr const char* TAG_MAX_DGRAM_SIZE = "MaxDatagramSize";
    static constexpr const char* TAG_MAX_RECEIVED_DGRAM_SIZE = "MaxRecDatagramSize";
    static constexpr const char* TAG_HEADSET_DEVICE_ID = "DeviceID";
    static constexpr const char* TAG_SEQ_NUM = "SeqNum";
    static constexpr const char* TAG_HEADSET_DEVICE_TYPE = "DeviceType";
    static constexpr const char* TAG_REASON_STR = "Reason";
    static constexpr const char* TAG_BUF_PADDING_CHAR = "Padding";
    static constexpr const char* TAG_PLATFORM_INFO = "PlatformInfo";

    static constexpr const char* TAG_OPTIONS = "Options";
    static constexpr const char* TAG_HELLO_OPTIONS = TAG_OPTIONS;
    static constexpr const char* TAG_CODECS = "Codecs";
    static constexpr const char* TAG_VIDEO_CODECS = "VideoCodecs";
    static constexpr const char* TAG_AUDIO_CODECS = "AudioCodecs";


    template<typename Codec_T, typename T>
    static void ParseCodecs(amf::JSONParser::Element* codecsArray, T& dest)
    {
        if (codecsArray != nullptr)
        {
            amf::JSONParser::Array::Ptr codecs(codecsArray);
            if (codecs != nullptr)
            {
                size_t count = codecs->GetElementCount();
                for (size_t i = 0; i < count; i++)
                {
                    amf::JSONParser::Node::Ptr codecNode(codecs->GetElementAt(i));
                    if (codecNode != nullptr)
                    {
                        Codec_T codec;
                        if (codec.FromJSON(codecNode) == amf::JSONParser::Result::OK)
                        {
                            dest.push_back(codec);
                        }
                    }
                }
            }
        }
    }

    template<typename Codec_T>
    static void CodecsToJSON(amf::JSONParser* parser, const std::vector<Codec_T>& src, const char* tag, amf::JSONParser::Node* options)
    {
        amf::JSONParser::Array::Ptr codecsJSON;
        parser->CreateArray(&codecsJSON);
        for (Codec_T it : src)
        {
            amf::JSONParser::Node::Ptr curCodec;
            it.ToJSON(parser, &curCodec);
            codecsJSON->AddElement(curCodec);
        }
        options->AddElement(tag, codecsJSON);
    }

    HelloRequest::HelloRequest(SERVICE_OP_CODE eOptCode) : Message(uint8_t(eOptCode)),
        m_ProtocolVersion(3),
        m_MinProtocolVersion(3)
    {
    }

    HelloRequest::HelloRequest(SERVICE_OP_CODE eOptCode, int32_t version, int32_t minVersion, const char* deviceID, size_t maxDatagramSize,
        const std::vector<VideoCodec>* videoCodecs, const std::vector<AudioCodec>* audioCodecs, amf::AMFPropertyStorage* properties) :
        Message(uint8_t(eOptCode)),
        m_ProtocolVersion(version),
        m_MinProtocolVersion(minVersion),
        m_MaxDatagramSize(maxDatagramSize),
        m_PlatformInfo(PLATFORM)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt32Value(parser, root, TAG_PROTOCOL_VERSION, m_ProtocolVersion);
        SetInt32Value(parser, root, TAG_PROTOCOL_MIN_VERSION, m_MinProtocolVersion);
        SetInt32Value(parser, root, TAG_MAX_DGRAM_SIZE, (int32_t)m_MaxDatagramSize);
        if (deviceID != nullptr)
        {
            m_DeviceID = deviceID;
            if (m_DeviceID.empty() == false)
            {
                SetStringValue(parser, root, TAG_HEADSET_DEVICE_ID, deviceID);
            }
        }

        SetStringValue(parser, root, TAG_PLATFORM_INFO, PLATFORM);

        if (properties != nullptr)
        {
            amf::JSONParser::Node::Ptr options;
            parser->CreateNode(&options);
            size_t count = properties->GetPropertyCount();
            if (count > 0 || (videoCodecs != nullptr && videoCodecs->size() > 0) || (audioCodecs != nullptr && audioCodecs->size() > 0))
            {
                for (size_t i = 0; i < count; i++)
                {
                    const size_t MAX_PROPERTY_NAME_LEN = 128;
                    wchar_t propertyName[MAX_PROPERTY_NAME_LEN];
                    amf::AMFVariant value;
                    properties->GetPropertyAt(i, propertyName, MAX_PROPERTY_NAME_LEN - 1, &value);
                    std::string propertyNameUTF8(amf::amf_from_unicode_to_utf8(propertyName).c_str());
                    if (propertyNameUTF8 == TAG_CODECS)
                    {
                        AMFTraceWarning(TRACE_SCOPE, L"Invalid option name: %s", propertyName);
                    }
                    else if (value.type == amf::AMF_VARIANT_INTERFACE)
                    {
                        SetInterfaceValue(parser, options, propertyNameUTF8.c_str(), value.pInterface);
                    }
                    else
                    {
                        SetVariantValue(parser, options, propertyNameUTF8.c_str(), value);
                    }
                }
                if (videoCodecs != nullptr && videoCodecs->size() > 0)
                {
                    CodecsToJSON<VideoCodec>(parser, *videoCodecs, TAG_VIDEO_CODECS, options);
                }
                if (audioCodecs != nullptr && audioCodecs->size() > 0)
                {
                    CodecsToJSON<AudioCodec>(parser, *audioCodecs, TAG_AUDIO_CODECS, options);
                }
                root->AddElement(TAG_HELLO_OPTIONS, options);
            }
        }

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    bool HelloRequest::FromJSON(amf::JSONParser::Node* root)
    {
        m_ProtocolVersion = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_PROTOCOL_VERSION))->GetValueAsInt32();
        m_MinProtocolVersion = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_PROTOCOL_MIN_VERSION))->GetValueAsInt32();
        m_MaxDatagramSize = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_MAX_DGRAM_SIZE))->GetValueAsInt32();

        GetStringValue(root, TAG_PLATFORM_INFO, m_PlatformInfo);

        amf::JSONParser::Value::Ptr deviceIDValue(root->GetElementByName(
            TAG_HEADSET_DEVICE_ID));  //This tag might not be present, handle
                                      //with care
        if (deviceIDValue != nullptr)
        {
            m_DeviceID = deviceIDValue->GetValue();
        }

        amf::JSONParser::Node::Ptr options(root->GetElementByName(TAG_HELLO_OPTIONS));
        if (options != nullptr)
        {
            m_Options.clear();
            m_VideoCodecs.clear();
            m_AudioCodecs.clear();
            size_t count = options->GetElementCount();
            for (size_t i = 0; i < count; i++)
            {
                std::string valName;
                amf::JSONParser::Node::Ptr valueNode(options->GetElementAt(i, valName));
                if (valName == TAG_CODECS)
                {
                    ParseCodecs<VideoCodec, std::vector<VideoCodec>>(valueNode->GetElementByName(TAG_VIDEO_CODECS), m_VideoCodecs);
                    ParseCodecs<AudioCodec, std::vector<AudioCodec>>(valueNode->GetElementByName(TAG_AUDIO_CODECS), m_AudioCodecs);
                }
                else if (valueNode != nullptr)
                {
                    amf::AMFVariant value;
                    GetVariantValue(options, valName.c_str(), value);
                    m_Options[valName] = value;
                }
            }
        }

        return true;
    }

    bool HelloRequest::GetOption(const char* name, amf::AMFVariant& value) const
    {
        bool result = false;
        std::map<std::string, amf::AMFVariant>::const_iterator it = m_Options.find(name);
        if (it != m_Options.end())
        {
            value = it->second;
            result = true;
        }
        return result;
    }

    bool HelloRequest::GetOption(size_t optIdx, std::string& name, amf::AMFVariant& value) const
    {
        bool result = false;
        size_t idx = 0;
        OptionsMap::const_iterator it = m_Options.begin();
        while (it != m_Options.end() && idx < optIdx)
        {
            ++it;
            ++idx;
        }
        if (it != m_Options.end())
        {
            name = it->first;
            value = it->second;
            result = true;
        }
        return result;
    }



    static constexpr const char* TAG_SERVER_NAME = "ServerName";
    static constexpr const char* TAG_CHANNELS_SUPPORTED = "ChannelsSupported";
    static constexpr const char* TAG_DATAGRAM_SIZE = "DatagramSize";
    static constexpr const char* TAG_MAX_DATAGRAM_SIZE = "MaxDatagramSize";
    static constexpr const char* TAG_PORT = "Port";
    static constexpr const char* TAG_SUPPORTED_TRANSPORTS = "Transports";
    static constexpr const char* TAG_STREAM_ID = "StreamID";

    HelloResponse::HelloResponse() :
        m_ProtocolVersion(0),
        m_MinProtocolVersion(0),
        m_MaxDatagramSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_DatagramSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_Port(0),
        m_StreamID(transport_common::DEFAULT_STREAM),
        m_OsName(PLATFORM)
    {
    }

    HelloResponse::HelloResponse(const std::string& name, uint32_t version, uint32_t minVersion, uint16_t port, uint32_t datagramSize, transport_common::StreamID streamID) :
        Message(uint8_t(SERVICE_OP_CODE::DISCOVERY)),
        m_ProtocolVersion(version),
        m_MinProtocolVersion(minVersion),
        m_ServerName(name),
        m_MaxDatagramSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_DatagramSize(datagramSize),
        m_Port(port),
        m_StreamID(streamID),
        m_OsName(PLATFORM)
    {
        ToJSON();
    }

    HelloResponse::HelloResponse(const std::string& name, uint32_t version, uint32_t minVersion,
                                 uint16_t port, uint32_t datagramSize, const std::vector<std::string>& supportedTransports, transport_common::StreamID streamID) :
        Message(uint8_t(SERVICE_OP_CODE::DISCOVERY)),
        m_ProtocolVersion(version),
        m_MinProtocolVersion(minVersion),
        m_ServerName(name),
        m_SupportedTransports(supportedTransports),
        m_MaxDatagramSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_DatagramSize(datagramSize),
        m_Port(port),
        m_StreamID(streamID),
        m_OsName(PLATFORM)
    {
        ToJSON();
    }

    void HelloResponse::ToJSON()
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);
        m_Root = root;

        amf::JSONParser::Node::Ptr options;
        parser->CreateNode(&options);
        m_Options.SetRoot(options);
        m_Options.SetParser(parser);

        if (m_ServerName.empty())
        {
            m_ServerName = "AMD Streaming Server";
        }
        SetStringValue(parser, root, TAG_SERVER_NAME, m_ServerName.c_str());

        SetInt32Value(parser, root, TAG_PROTOCOL_VERSION, m_ProtocolVersion);
        SetInt32Value(parser, root, TAG_PROTOCOL_MIN_VERSION, m_MinProtocolVersion);

        if (m_DatagramSize == 0)
        {
            m_DatagramSize = m_MaxDatagramSize;
        }
        SetInt32Value(parser, root, TAG_DATAGRAM_SIZE, m_DatagramSize);
        SetInt32Value(parser, root, TAG_MAX_DATAGRAM_SIZE, m_MaxDatagramSize);
        SetInt32Value(parser, root, TAG_PORT, m_Port);

        root->AddElement(TAG_OPTIONS, options);

        if (m_SupportedTransports.size() > 0)
        {
            amf::JSONParser::Array::Ptr transports;
            parser->CreateArray(&transports);
            for (std::vector<std::string>::const_iterator it = m_SupportedTransports.begin(); it != m_SupportedTransports.end(); it++)
            {
                amf::JSONParser::Value::Ptr curTransport;
                parser->CreateValue(&curTransport);
                curTransport->SetValue(*it);
                transports->AddElement(curTransport);
            }
            root->AddElement(TAG_SUPPORTED_TRANSPORTS, transports);
        }

        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    bool HelloResponse::FromJSON(amf::JSONParser::Node* root)
    {
        m_ProtocolVersion = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_PROTOCOL_VERSION))->GetValueAsInt32();
        m_MinProtocolVersion = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_PROTOCOL_MIN_VERSION))->GetValueAsInt32();
        m_ServerName = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_SERVER_NAME))->GetValue();
        m_MaxDatagramSize = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_MAX_DATAGRAM_SIZE))->GetValueAsInt32();
        m_DatagramSize = ((const amf::JSONParser::Value*)root->GetElementByName(TAG_DATAGRAM_SIZE))->GetValueAsInt32();
        m_Port = static_cast<uint16_t>(((const amf::JSONParser::Value*)root->GetElementByName(TAG_PORT))->GetValueAsUInt32());

        m_SupportedTransports.clear();
        const amf::JSONParser::Array* transports = (const amf::JSONParser::Array*)root->GetElementByName(TAG_SUPPORTED_TRANSPORTS);
        if (transports != nullptr)
        {
            size_t count = transports->GetElementCount();
            for (size_t i = 0; i < count; i++)
            {
                m_SupportedTransports.push_back(((const amf::JSONParser::Value*)transports->GetElementAt(i))->GetValue());
            }
        }
        else
        {
            m_SupportedTransports.push_back("UDP");
        }

        amf::JSONParser::Node::Ptr options(root->GetElementByName(TAG_OPTIONS));
        m_Options.SetRoot(options);

        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    void HelloResponse::UpdateData()
    {
        m_Data.resize(1);
        *((unsigned char*)&m_Data[0]) = m_OpCode;
        m_Data += m_Root->Stringify();
    }

    HelloRefused::HelloRefused() :
        Message(uint8_t(SERVICE_OP_CODE::CONNECTION_REFUSED))
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    bool HelloRefused::FromJSON(amf::JSONParser::Node* /*root*/)
    {
        return true;
    }

    ConnectRequest::ConnectRequest() :
        HelloRequest(SERVICE_OP_CODE::HELLO)
    {
    }

    ConnectRequest::ConnectRequest(int32_t version, int32_t minVersion, const char* deviceID, size_t maxDatagramSize,
                                   const std::vector<VideoCodec>& videoCodecs, const std::vector<AudioCodec>& audioCodecs,
                                   amf::AMFPropertyStorage* properties) :
        HelloRequest(SERVICE_OP_CODE::HELLO, version, minVersion, deviceID, maxDatagramSize, &videoCodecs, &audioCodecs, properties)
    {
    }

    DiscoveryRequest::DiscoveryRequest() :
        HelloRequest(SERVICE_OP_CODE::DISCOVERY)
    {
    }

    DiscoveryRequest::DiscoveryRequest(int32_t version, int32_t minVersion, const char* deviceID, size_t maxDatagramSize, amf::AMFPropertyStorage* properties) :
        HelloRequest(SERVICE_OP_CODE::DISCOVERY, version, minVersion, deviceID, maxDatagramSize, nullptr, nullptr, properties)
    {
    }

    inline bool HelloResponse::Options::GetArray(const char* name, amf::JSONParser::Array** val) const
    {
        bool result = false;
        amf::JSONParser::Element* element = m_Root->GetElementByName(name);
        if (element != nullptr)
        {
            amf::JSONParser::Array::Ptr pArray(element);
            if (pArray != nullptr)
            {
                *val = pArray;
                (*val)->Acquire();
                result = true;
            }
        }
        return result;
    }

    inline bool HelloResponse::Options::GetNode(const char* name, amf::JSONParser::Node** val) const
    {
        bool result = false;
        amf::JSONParser::Element* element = m_Root->GetElementByName(name);
        if (element != nullptr)
        {
            amf::JSONParser::Node::Ptr pNode(element);
            if (pNode != nullptr)
            {
                *val = pNode;
                (*val)->Acquire();
                result = true;
            }
        }
        return result;
    }

}
