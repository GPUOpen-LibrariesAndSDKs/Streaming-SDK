//
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
//
// MIT license
//
//
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include "amf/public/common/Json.h"
#include "amf/public/common/InterfaceImpl.h"

#define TOUCH_EVENT_ACTION_MASK 0xFF
#define TOUCH_EVENT_ACTION_DOWN 0x00
#define TOUCH_EVENT_ACTION_POINTER_DOWN 0x05
#define TOUCH_EVENT_ACTION_MOVE 0x02
#define TOUCH_EVENT_ACTION_UP 0x01
#define TOUCH_EVENT_ACTION_POINTER_UP 0x06
#define TOUCH_EVENT_ACTION_POINTER_INDEX_SHIFT 0x08
#define TOUCH_EVENT_ACTION_POINTER_INDEX_MASK 0x0000ff00

namespace ssdk::ctls
{
class TouchEvent : public amf::AMFInterfaceImpl<amf::AMFInterfaceJSONSerializable>
{
public:
    // {C03FC882-81E7-4B17-8967-2A06B2D1CE06}
    AMF_DECLARE_IID(0xc03fc882, 0x81e7, 0x4b17, 0x89, 0x67, 0x2a, 0x6, 0xb2, 0xd1, 0xce, 0x6)
    typedef amf::AMFInterfacePtr_T<TouchEvent>  Ptr;
    
    struct Pointer
    {
        int32_t m_id;
        float m_x;
        float m_y;
    };

    AMF_BEGIN_INTERFACE_MAP
        AMF_INTERFACE_ENTRY(TouchEvent)
        AMF_INTERFACE_CHAIN_ENTRY(amf::AMFInterfaceImpl<amf::AMFInterfaceJSONSerializable>)
    AMF_END_INTERFACE_MAP

    TouchEvent();
    TouchEvent(const int32_t action, const int64_t time);
    virtual ~TouchEvent();

    // AMFInterfaceJSONSerializable interface implementations
    virtual AMF_RESULT AMF_STD_CALL ToJson(amf::JSONParser* parser, amf::JSONParser::Node* node) const override;
    virtual AMF_RESULT AMF_STD_CALL FromJson(const amf::JSONParser::Node* node) override;

    inline void AddPointer(int32_t id, float x, float y) { m_pointers.push_back({ id, x, y }); }
    inline std::vector<Pointer>::const_iterator PointersBegin() const { return m_pointers.cbegin(); };
    inline std::vector<Pointer>::const_iterator PointersEnd() const { return m_pointers.cend(); };
    inline int32_t GetAction() const { return m_action; }
    inline int64_t GetTime() const { return m_time; }

private:
    int32_t m_action = 0;
    int64_t m_time = 0;
    std::vector<Pointer> m_pointers;
};
} // namespace ssdk::ctls
