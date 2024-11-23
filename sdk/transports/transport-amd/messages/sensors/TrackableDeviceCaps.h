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
#include <string>
#include <vector>
#include <memory>

namespace ssdk::transport_amd
{

    class TrackableDeviceCaps : public Message
    {
    public:
        enum class DeviceClass
        {
            UNKNOWN,
            HMD,
            VR_CONTROLLER,
            MOUSE,
            TOUCHSCREEN,
            KEYBOARD,
            GAME_CONTROLLER,
        };

    public:
        TrackableDeviceCaps();
        TrackableDeviceCaps(const std::string& id, DeviceClass cls, const std::string& description,
            const std::vector<std::string>* const inputs = nullptr, const std::vector<std::string>* const outputs = nullptr, bool is6DoF = true);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;
        std::string ToJSON() const;

        inline const std::string& GetID() const noexcept { return m_ID; }
        inline DeviceClass GetClass() const noexcept { return m_Class; }
        inline const std::string& GetDescription() const noexcept { return m_Description; }

        inline const std::vector<std::string>& GetInputs() const noexcept { return m_Inputs; }
        inline const std::vector<std::string>& GetOutputs() const noexcept { return m_Outputs; }
        inline bool Is6DoF() const noexcept { return m_6DoF; }

    private:
        bool ParseControls(const char* tag, std::vector<std::string>& values, amf::JSONParser::Node::Ptr root) const;
        void CreateControls(const char* tag, const std::vector<std::string>& values, amf::JSONParser::Node::Ptr root, amf::JSONParser::Ptr parser) const;

    protected:
        std::string m_ID;
        std::string m_Description;
        DeviceClass m_Class = DeviceClass::UNKNOWN;
        std::vector<std::string> m_Inputs, m_Outputs;
        bool m_6DoF = false;
    };

}
