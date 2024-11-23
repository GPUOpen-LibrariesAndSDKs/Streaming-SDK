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

#include "TrackableDeviceCaps.h"
#include "transports/transport-amd/Channels.h"


namespace ssdk::transport_amd
{
    static constexpr const char* TAG_CLASS = "class";
    static constexpr const char* TAG_CLASS_UNKNOWN = "unknown";
    static constexpr const char* TAG_CLASS_HMD = "hmd";
    static constexpr const char* TAG_CLASS_VR_CONTROLLER = "ctrl";
    static constexpr const char* TAG_CLASS_GAME_CONTROLLER = "gctrl";
    static constexpr const char* TAG_CLASS_MOUSE = "mouse";
    static constexpr const char* TAG_CLASS_TOUCHSCREEN = "touchscreen";
    static constexpr const char* TAG_CLASS_KEYBOARD = "kbd";
    static constexpr const char* TAG_ID = "id";
    static constexpr const char* TAG_DESCRIPTION = "desc";
    static constexpr const char* TAG_INPUTS = "inputs";
    static constexpr const char* TAG_OUTPUTS = "outputs";
    static constexpr const char* TAG_DOF = "DoF";


    TrackableDeviceCaps::TrackableDeviceCaps() :
        Message(uint8_t(SENSOR_OP_CODE::TRACKABLE_DEVICE_CAPS)),
        m_Class(DeviceClass::UNKNOWN),
        m_6DoF(true)
    {
    }

    TrackableDeviceCaps::TrackableDeviceCaps(const std::string& id, DeviceClass cls, const std::string& description, const std::vector<std::string>* const inputs,
                                             const std::vector<std::string>* const outputs, bool is6DoF) :
        Message(uint8_t(SENSOR_OP_CODE::TRACKABLE_DEVICE_CAPS)),
        m_ID(id),
        m_Description(description),
        m_Class(cls),
        m_6DoF(is6DoF)
    {
        if (inputs != nullptr)
        {
            m_Inputs = *inputs;
        }
        if (outputs != nullptr)
        {
            m_Outputs = *outputs;
        }

        m_Data += ToJSON();
    }

    bool TrackableDeviceCaps::FromJSON(amf::JSONParser::Node* root)
    {
        bool result = true;

        if (GetStringValue(root, TAG_ID, m_ID) == false)
        {
            result = false;
        }
        else
        {
            std::string cls;
            if (GetStringValue(root, TAG_CLASS, cls) == false)
            {
                result = false;
            }
            else
            {
                if (cls == TAG_CLASS_HMD)
                {
                    m_Class = DeviceClass::HMD;
                }
                else if (cls == TAG_CLASS_VR_CONTROLLER)
                {
                    m_Class = DeviceClass::VR_CONTROLLER;
                }
                else if (cls == TAG_CLASS_GAME_CONTROLLER)
                {
                    m_Class = DeviceClass::GAME_CONTROLLER;
                }
                else if (cls == TAG_CLASS_MOUSE)
                {
                    m_Class = DeviceClass::MOUSE;
                }
                else if (cls == TAG_CLASS_TOUCHSCREEN)
                {
                    m_Class = DeviceClass::TOUCHSCREEN;
                }
                else if (cls == TAG_CLASS_KEYBOARD)
                {
                    m_Class = DeviceClass::KEYBOARD;
                }
                else
                {
                    m_Class = DeviceClass::UNKNOWN;
                }
                GetStringValue(root, TAG_DESCRIPTION, m_Description);
                ParseControls(TAG_INPUTS, m_Inputs, root);
                ParseControls(TAG_OUTPUTS, m_Outputs, root);
            }
            if (GetBoolValue(root, TAG_DOF, m_6DoF) == false)
            {
                result = false;
            }
        }

        return result;
    }

    std::string TrackableDeviceCaps::ToJSON() const
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetStringValue(parser, root, TAG_ID, m_ID);
        switch (m_Class)
        {
        case DeviceClass::HMD:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_HMD);
            break;
        case DeviceClass::VR_CONTROLLER:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_VR_CONTROLLER);
            break;
        case DeviceClass::GAME_CONTROLLER:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_GAME_CONTROLLER);
            break;
        case DeviceClass::MOUSE:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_MOUSE);
            break;
        case DeviceClass::TOUCHSCREEN:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_TOUCHSCREEN);
            break;
        case DeviceClass::KEYBOARD:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_KEYBOARD);
            break;
        default:
            SetStringValue(parser, root, TAG_CLASS, TAG_CLASS_UNKNOWN);
            break;
        }

        if (m_Description.length() > 0)
        {
            SetStringValue(parser, root, TAG_DESCRIPTION, m_Description);
        }
        CreateControls(TAG_INPUTS, m_Inputs, root, parser);
        CreateControls(TAG_OUTPUTS, m_Outputs, root, parser);
        SetBoolValue(parser, root, TAG_DOF, m_6DoF);

        return root->Stringify();
    }

    bool TrackableDeviceCaps::ParseControls(const char* tag, std::vector<std::string>& values, amf::JSONParser::Node::Ptr root) const
    {
        bool result = true;
        values.clear();
        amf::JSONParser::Array::Ptr controls(root->GetElementByName(tag));
        if (controls != nullptr)
        {
            size_t cnt = controls->GetElementCount();
            for (size_t i = 0; i < cnt; i++)
            {
                amf::JSONParser::Value::Ptr elem(controls->GetElementAt(i));
                if (elem == nullptr)
                {
                    result = false;
                    break;
                }
                else
                {
                    values.push_back(elem->GetValue());
                }
            }
        }

        return result;
    }

    void TrackableDeviceCaps::CreateControls(const char* tag, const std::vector<std::string>& values, amf::JSONParser::Node::Ptr root, amf::JSONParser::Ptr parser) const
    {
        if (values.size() > 0)
        {
            amf::JSONParser::Array::Ptr controls;
            parser->CreateArray(&controls);
            for (std::vector<std::string>::const_iterator it = values.begin();
                it != values.end(); ++it)
            {
                amf::JSONParser::Value::Ptr val;
                parser->CreateValue(&val);
                val->SetValue(*it);
                controls->AddElement(val);
            }
            root->AddElement(tag, controls);
        }
    }

}