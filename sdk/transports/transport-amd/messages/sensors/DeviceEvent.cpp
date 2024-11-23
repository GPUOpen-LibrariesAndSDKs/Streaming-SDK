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

#include "DeviceEvent.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_TIME = "time";
    static constexpr const char* TAG_FLAGS = "flags";
    static constexpr const char* TAG_ID = "id";
    static constexpr const char* TAG_DATA = "data";
    static constexpr const char* TAG_ORIENTATION = "orient";
    static constexpr const char* TAG_POSITION = "pos";

    static constexpr const char* TAG_ORIENTATION_A = "orientA";
    static constexpr const char* TAG_POSITION_A = "posA";
    static constexpr const char* TAG_ORIENTATION_V = "orientV";
    static constexpr const char* TAG_POSITION_V = "posV";

    static constexpr const char* TAG_INDEX = "frmIdx";
    static constexpr const char* TAG_BASE = "baseFrmIdx";
    static constexpr const char* TAG_VALUE = "val";
    static constexpr const char* TAG_EVENTS = "events";

    static constexpr const char* TAG_STREAM_ID = "StreamID";

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DeviceEvent::Pose::Pose()
    {
    }

    DeviceEvent::Pose::Pose(const amf::Quaternion& orientation, const amf::Vector& position, int64_t idx, int64_t baseIdx) :
        amf::Pose(orientation, position), m_Idx(idx), m_BaseIdx(baseIdx)
    {
    }

    DeviceEvent::Pose::Pose(const amf::Quaternion& orientation, const amf::Vector& position, const amf::Vector& orientationVelocity, const amf::Vector& positionVelocity,
                            int64_t idx, int64_t baseIdx) :
        amf::Pose(orientation, position, orientationVelocity, positionVelocity),
        m_Idx(idx),
        m_BaseIdx(baseIdx)
    {
    }

    DeviceEvent::Pose::Pose(const amf::Quaternion& orientation, const amf::Vector& position, const amf::Vector& orientationVelocity, const amf::Vector& positionVelocity,
                            const amf::Vector& orientationAcceleration, const amf::Vector& positionAcceleration, int64_t idx, int64_t baseIdx) :
        amf::Pose(orientation, position, orientationVelocity, positionVelocity, orientationAcceleration, positionAcceleration),
        m_Idx(idx),
        m_BaseIdx(baseIdx)
    {
    }

    DeviceEvent::Pose::Pose(const amf::Pose& pose, int64_t idx, int64_t baseIdx) :
        amf::Pose(pose),
        m_Idx(idx),
        m_BaseIdx(baseIdx)
    {
    }

    bool DeviceEvent::Pose::FromJSON(amf::JSONParser::Element::Ptr elem)
    {
        bool result = false;
        amf::JSONParser::Node::Ptr node(elem);
        if (node != nullptr)
        {
            result = GetInt64Value(node, TAG_INDEX, m_Idx);
            result = GetInt64Value(node, TAG_BASE, m_BaseIdx);

            m_ValidityFlags = PF_NONE;
            size_t size = 4;
            if (GetFloatArray(node, TAG_ORIENTATION, &m_Orientation.x, size) == true)
            {
                m_ValidityFlags |= PF_ORIENTATION;
                result = true;
            }
            size = 3;
            if (GetFloatArray(node, TAG_POSITION, &m_Position.x, size) == true)
            {
                m_ValidityFlags |= PF_POSITION;
                result = result & true;
            }
            size = 3;
            if (GetFloatArray(node, TAG_ORIENTATION_V, &m_OrientationVelocity.x, size) == true)
            {
                m_ValidityFlags |= PF_ORIENTATION_VELOCITY;
            }
            size = 3;
            if (GetFloatArray(node, TAG_POSITION_V, &m_PositionVelocity.x, size) == true)
            {
                m_ValidityFlags |= PF_POSITION_VELOCITY;
            }
            size = 3;
            if (GetFloatArray(node, TAG_ORIENTATION_A, &m_OrientationAcceleration.x, size) == true)
            {
                m_ValidityFlags |= PF_ORIENTATION_ACCELERATION;
            }
            size = 3;
            if (GetFloatArray(node, TAG_POSITION_A, &m_PositionAcceleration.x, size) == true)
            {
                m_ValidityFlags |= PF_POSITION_ACCELERATION;
            }
        }
        return result;
    }

    amf::JSONParser::Element::Ptr DeviceEvent::Pose::ToJSON(amf::JSONParser::Ptr parser) const
    {
        amf::JSONParser::Node::Ptr node;
        parser->CreateNode(&node);

        size_t size = 4;
        if (m_ValidityFlags & PF_ORIENTATION)
        {
            SetFloatArray(parser, node, TAG_ORIENTATION, &m_Orientation.x, size);
        }
        size = 3;
        if (m_ValidityFlags & PF_POSITION)
        {
            SetFloatArray(parser, node, TAG_POSITION, &m_Position.x, size);
        }
        size = 3;
        if (m_ValidityFlags & PF_ORIENTATION_VELOCITY)
        {
            SetFloatArray(parser, node, TAG_ORIENTATION_V, &m_OrientationVelocity.x,
                size);
        }
        size = 3;
        if (m_ValidityFlags & PF_POSITION_VELOCITY)
        {
            SetFloatArray(parser, node, TAG_POSITION_V, &m_PositionVelocity.x,
                size);
        }
        size = 3;
        if (m_ValidityFlags & PF_ORIENTATION_ACCELERATION)
        {
            SetFloatArray(parser, node, TAG_ORIENTATION_A,
                &m_OrientationAcceleration.x, size);
        }
        size = 3;
        if (m_ValidityFlags & PF_POSITION_ACCELERATION)
        {
            SetFloatArray(parser, node, TAG_POSITION_A, &m_PositionAcceleration.x,
                size);
        }

        if (m_Idx != -1LL)
        {
            SetInt64Value(parser, node, TAG_INDEX, m_Idx);
        }
        if (m_BaseIdx != -1LL)
        {
            SetInt64Value(parser, node, TAG_BASE, m_BaseIdx);
        }
        return amf::JSONParser::Element::Ptr(node);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DeviceEvent::DataElement::DataElement(amf::JSONParser::Node* elemNode, amf::JSONParser* parser) :
        m_ElemNode(elemNode),
        m_Parser(parser)
    {
    }

    DeviceEvent::DataElement::DataElement(amf::JSONParser::Element* value, uint64_t flags, amf_pts timestamp, amf::JSONParser* parser) :
        m_Parser(parser)
    {
        m_Parser->CreateNode(&m_ElemNode);
        if (timestamp != -1)
        {
            SetInt64Value(parser, m_ElemNode, TAG_TIME, timestamp);
        }
        if (flags != 0)
        {
            SetUInt64Value(parser, m_ElemNode, TAG_FLAGS, flags);
        }
        m_ElemNode->AddElement(TAG_VALUE, value);
    }

    DeviceEvent::DataElement::~DataElement()
    {
    }

    bool DeviceEvent::DataElement::FromJSON(amf::JSONParser::Element::Ptr& value, uint64_t& flags, amf_pts& timestamp) const
    {
        flags = 0;
        timestamp = -1LL;
        bool result = true;
        result = GetInt64Value(m_ElemNode, TAG_TIME, timestamp);
        GetUInt64Value(m_ElemNode, TAG_FLAGS, flags);
        value = m_ElemNode->GetElementByName(TAG_VALUE);
        return result;
    }

    bool DeviceEvent::DataElement::GetTimestampValue(amf_pts& value) const
    {
        return GetInt64Value(m_ElemNode, TAG_TIME, value);
    }
    bool DeviceEvent::DataElement::GetFlagsValue(uint64_t& value) const
    {
        return GetUInt64Value(m_ElemNode, TAG_FLAGS, value);
    }

    bool DeviceEvent::DataElement::GetVariantValue(amf::AMFVariant& val) const
    {
        amf::JSONParser::Element* element = m_ElemNode->GetElementByName(TAG_VALUE);
        return GetVariantValueFromJSON(element, val);
    }

    amf::JSONParser::Node* DeviceEvent::DataElement::GetValueNode() const
    {
        return (amf::JSONParser::Node*)m_ElemNode->GetElementByName(TAG_VALUE);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DeviceEvent::DeviceEvent(amf::JSONParser::Ptr parser) :
        Message(uint8_t(SENSOR_OP_CODE::DEVICE_EVENT)),
        m_Parser(parser)
    {
    }

    DeviceEvent::DeviceEvent() :
        Message(uint8_t(SENSOR_OP_CODE::DEVICE_EVENT))
    {
        CreateJSONParser(&m_Parser);
    }

    DeviceEvent::DeviceEvent(transport_common::StreamID streamID) :
        Message(uint8_t(SENSOR_OP_CODE::DEVICE_EVENT)),
        m_StreamID(streamID)
    {
        CreateJSONParser(&m_Parser);
    }

    std::string DeviceEvent::ToJSON() const
    {
        amf::JSONParser::Node::Ptr root;
        m_Parser->CreateNode(&root);
        amf::JSONParser::Array::Ptr events;
        m_Parser->CreateArray(&events);
        for (DataCollection::const_iterator it = m_Values.begin(); it != m_Values.end(); ++it)
        {
            amf::JSONParser::Node::Ptr eventNode;
            m_Parser->CreateNode(&eventNode);
            SetStringValue(m_Parser, eventNode, TAG_ID, it->first);

            amf::JSONParser::Array::Ptr elements;
            m_Parser->CreateArray(&elements);
            const Data& data(it->second);
            for (Data::const_iterator dataIt = data.begin(); dataIt != data.end(); ++dataIt)
            {
                elements->AddElement(dataIt->ToJSON());
            }

            eventNode->AddElement(TAG_DATA, elements);
            events->AddElement(eventNode);
        }
        root->AddElement(TAG_EVENTS, events);

        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(m_Parser, root, TAG_STREAM_ID, m_StreamID);
        }

        return root->Stringify();
    }

    bool DeviceEvent::FromJSON(amf::JSONParser::Node* root)
    {
        bool result = false;

        amf::JSONParser::Array::Ptr events(root->GetElementByName(TAG_EVENTS));
        if (events != nullptr)
        {
            size_t count = events->GetElementCount();
            result = true;
            for (size_t i = 0; i < count; i++)
            {
                amf::JSONParser::Node::Ptr elem(events->GetElementAt(i));
                if (elem == nullptr)
                {
                    result = false;
                    break;
                }
                else
                {
                    amf::JSONParser::Value::Ptr idElem(elem->GetElementByName(TAG_ID));
                    amf::JSONParser::Array::Ptr dataArray(elem->GetElementByName(TAG_DATA));
                    if (idElem == nullptr || dataArray == nullptr)
                    {
                        result = false;
                        break;
                    }
                    else
                    {
                        const std::string& id = idElem->GetValue();
                        Data data;
                        size_t valCnt = dataArray->GetElementCount();
                        for (size_t valIdx = 0; valIdx < valCnt; valIdx++)
                        {
                            amf::JSONParser::Node::Ptr elemNode(dataArray->GetElementAt(valIdx));
                            data.push_back(DataElement(elemNode, m_Parser));
                        }
                        m_Values.push_back(DataPair(id, data));
                    }
                }
            }
        }

        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return result;
    }

    void DeviceEvent::AddElementValue(const std::string& id, amf::JSONParser::Element* value, uint64_t flags, amf_pts timestamp)
    {
        DataCollection::iterator found;
        for (found = m_Values.begin(); found != m_Values.end(); found++)
        {
            if (found->first == id)
            {
                break;
            }
        }
        if (found == m_Values.end())
        {
            m_Values.push_back(DataPair(id, Data()));
            found = m_Values.end() - 1;
        }
        found->second.push_back(DataElement(value, flags, timestamp, m_Parser));
    }

    void DeviceEvent::AddValue(const std::string& id, const Pose& value, uint64_t flags, amf_pts timestamp)
    {
        AddElementValue(id, value.ToJSON(m_Parser), flags, timestamp);
    }

    void DeviceEvent::AddValue(const std::string& id, const amf::AMFVariantStruct& value, uint64_t flags, amf_pts timestamp)
    {
        amf::JSONParser::Element::Ptr valueNode;
        CreateVariantValue(m_Parser, &valueNode, (const amf::AMFVariant)value);
        AddElementValue(id, valueNode, flags, timestamp);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TrackableDeviceDisconnected::TrackableDeviceDisconnected() :
        Message(uint8_t(SENSOR_OP_CODE::TRACKABLE_DEVICE_DISCONNECTED))
    {
    }

    TrackableDeviceDisconnected::TrackableDeviceDisconnected(const std::string& id) :
        Message(uint8_t(SENSOR_OP_CODE::TRACKABLE_DEVICE_DISCONNECTED)),
        m_ID(id)
    {
        m_Data += ToJSON();
    }

    bool TrackableDeviceDisconnected::FromJSON(amf::JSONParser::Node* root)
    {
        bool result = true;

        if (GetStringValue(root, TAG_ID, m_ID) == false)
        {
            result = false;
        }
        return result;
    }

    std::string TrackableDeviceDisconnected::ToJSON() const
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);
        SetStringValue(parser, root, TAG_ID, m_ID);
        return root->Stringify();
    }

}
