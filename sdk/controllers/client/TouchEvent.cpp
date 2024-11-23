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

#include "TouchEvent.h"

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    TouchEvent::TouchEvent()
    {
    }
    TouchEvent::TouchEvent(const int32_t action, const int64_t time) :
        m_action(action), m_time(time)
    {
    }
    TouchEvent::~TouchEvent()
    {
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL TouchEvent::ToJson(amf::JSONParser* parser, amf::JSONParser::Node* node) const
    {
        amf::SetInt32Value(parser, node, "action", m_action);
        amf::SetInt64Value(parser, node, "time", m_time);

        amf::JSONParser::Array::Ptr pointerArray;
        parser->CreateArray(&pointerArray);

        for (size_t i = 0; i < m_pointers.size(); i++)
        {
            amf::JSONParser::Node::Ptr pointer;
            parser->CreateNode(&pointer);
            amf::SetInt32Value(parser, pointer, "id", m_pointers[i].m_id);
            amf::SetFloatValue(parser, pointer, "x", m_pointers[i].m_x);
            amf::SetFloatValue(parser, pointer, "y", m_pointers[i].m_y);
            pointerArray->AddElement(pointer);
        }
        node->AddElement("pointers", pointerArray);

        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL TouchEvent::FromJson(const amf::JSONParser::Node* node)
    {
        amf::GetInt32Value(node, "action", m_action);
        amf::GetInt64Value(node, "time", m_time);

        m_pointers.clear();
        amf::JSONParser::Array::Ptr pointerArray(node->GetElementByName("pointers"));
        if (nullptr != pointerArray)
        {
            for (size_t i = 0; i < pointerArray->GetElementCount(); i++)
            {
                int32_t id = 0;
                float x = 0;
                float y = 0;

                const amf::JSONParser::Node* pointer = (const amf::JSONParser::Node*)pointerArray->GetElementAt(i);
                amf::GetInt32Value(pointer, "id", id);
                amf::GetFloatValue(pointer, "x", x);
                amf::GetFloatValue(pointer, "y", y);

                m_pointers.push_back({ id, x, y });
            }
        }
        return AMF_OK;
    }
}// namespace ssdk::ctls
