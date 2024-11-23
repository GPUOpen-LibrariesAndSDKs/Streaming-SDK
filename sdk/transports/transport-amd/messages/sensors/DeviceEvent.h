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
#include "transports/transport-common/Transport.h"
#include "amf/public/common/AMFMath.h"
#include <string>
#include <vector>
#include <memory>

namespace ssdk::transport_amd
{
    class DeviceEvent : public Message
    {
    public:
        class Pose : public amf::Pose
        {
        public:
            typedef std::shared_ptr<Pose> Ptr;

        public:
            Pose();
            Pose(const amf::Quaternion& orientation, const amf::Vector& position, int64_t idx, int64_t baseIdx);
            Pose(const amf::Quaternion& orientation, const amf::Vector& position, const amf::Vector& orientationVelocity, const amf::Vector& positionVelocity,
                int64_t idx, int64_t baseIdx);
            Pose(const amf::Quaternion& orientation, const amf::Vector& position, const amf::Vector& orientationVelocity, const amf::Vector& positionVelocity,
                const amf::Vector& orientationAcceleration, const amf::Vector& positionAcceleration, int64_t idx, int64_t baseIdx);
            Pose(const amf::Pose& pose, int64_t idx, int64_t baseIdx);

            bool FromJSON(amf::JSONParser::Element::Ptr elem);
            amf::JSONParser::Element::Ptr ToJSON(amf::JSONParser::Ptr parser) const;

            inline int64_t GetFrameIdx() const noexcept { return m_Idx; }
            inline void SetFrameIdx(int64_t idx) noexcept { m_Idx = idx; }

            inline int64_t GetBaseFrameIdx() const noexcept { return m_BaseIdx; }
            inline void SetBaseFrameIdx(int64_t idx) noexcept { m_BaseIdx = idx; }

        protected:
            int64_t m_Idx = -1;
            int64_t m_BaseIdx = -1;
        };

        class DataElement
        {
        public:
            typedef std::shared_ptr<DataElement> Ptr;

        public:
            DataElement(amf::JSONParser::Node* elemNode, amf::JSONParser* parser);
            DataElement(amf::JSONParser::Element* value, uint64_t flags, amf_pts timestamp, amf::JSONParser* parser);
            virtual ~DataElement();

            bool FromJSON(amf::JSONParser::Element::Ptr& value, uint64_t& flags, amf_pts& timestamp) const;

            inline amf::JSONParser::Node::Ptr ToJSON() const noexcept { return m_ElemNode; }

            bool GetTimestampValue(amf_pts& value) const;
            bool GetFlagsValue(uint64_t& value) const;

            bool GetVariantValue(amf::AMFVariant& val) const;

            amf::JSONParser::Node* GetValueNode() const;

            inline std::string ToString() const { return m_ElemNode->Stringify(); }

        protected:
            amf::JSONParser::Node::Ptr m_ElemNode;
            amf::JSONParser::Ptr m_Parser;
        };

        DeviceEvent();
        DeviceEvent(transport_common::StreamID streamID);
        DeviceEvent(amf::JSONParser::Ptr parser);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;
        inline void Prepare() { m_Data += ToJSON(); }

        void AddElementValue(const std::string& id, amf::JSONParser::Element* value, uint64_t flags, amf_pts timestamp);

        void AddValue(const std::string& id, const Pose& value, uint64_t flags, amf_pts timestamp = -1LL);
        void AddValue(const std::string& id, const amf::AMFVariantStruct& value, uint64_t flags, amf_pts timestamp = -1LL);

        typedef std::vector<DataElement> Data;
        typedef std::pair<std::string, Data> DataPair;
        typedef std::vector<DataPair> DataCollection;

        inline const DataCollection& GetDataCollection() const { return m_Values; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_StreamID; }

    protected:
        std::string ToJSON() const;
        amf::JSONParser::Ptr m_Parser;
        DataCollection m_Values;
        transport_common::StreamID m_StreamID = transport_common::DEFAULT_STREAM;
    };


    class TrackableDeviceDisconnected : public Message
    {
    public:
        TrackableDeviceDisconnected();
        TrackableDeviceDisconnected(const std::string& id);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;
        std::string ToJSON() const;

        inline const std::string& GetID() const noexcept { return m_ID; }

    private:
        std::string m_ID;
    };

}