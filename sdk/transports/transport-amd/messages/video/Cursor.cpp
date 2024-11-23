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

#include "Cursor.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_WIDTH = "Width";
    static constexpr const char* TAG_HEIGHT = "Height";
    static constexpr const char* TAG_PITCH = "Pitch";
    static constexpr const char* TAG_HOTSPOT_X = "HotspotX";
    static constexpr const char* TAG_HOTSPOT_Y = "HotspotY";
    static constexpr const char* TAG_CAPTURE_RESOLUTION_X = "CaptureResolutionX";
    static constexpr const char* TAG_CAPTURE_RESOLUTION_Y = "CaptureResolutionY";
    static constexpr const char* TAG_VISIBLE = "Visible";
    static constexpr const char* TAG_MONOCHROME = "Monochrome";

    CursorData::CursorData() :
        Message(uint8_t(VIDEO_OP_CODE::CURSOR))
    {
    }

    CursorData::CursorData(int32_t width, int32_t height, int32_t pitch, int32_t hotspotX, int32_t hotspotY,
                           int32_t captureResolutionX, int32_t captureResolutionY, bool visible, bool monochrome) :
        Message(uint8_t(VIDEO_OP_CODE::CURSOR)),
        m_Width(width),
        m_Height(height),
        m_Pitch(pitch),
        m_HotspotX(hotspotX),
        m_HotspotY(hotspotY),
        m_CaptureResolutionX(captureResolutionX),
        m_CaptureResolutionY(captureResolutionY),
        m_Visible(visible),
        m_Monochrome(monochrome)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt32Value(parser, root, TAG_WIDTH, m_Width);
        SetInt32Value(parser, root, TAG_HEIGHT, m_Height);
        SetInt32Value(parser, root, TAG_PITCH, m_Pitch);
        SetInt32Value(parser, root, TAG_HOTSPOT_X, m_HotspotX);
        SetInt32Value(parser, root, TAG_HOTSPOT_Y, m_HotspotY);
        SetInt32Value(parser, root, TAG_CAPTURE_RESOLUTION_X, m_CaptureResolutionX);
        SetInt32Value(parser, root, TAG_CAPTURE_RESOLUTION_Y, m_CaptureResolutionY);
        SetBoolValue(parser, root, TAG_VISIBLE, m_Visible);
        SetBoolValue(parser, root, TAG_MONOCHROME, m_Monochrome);

        m_Data += root->Stringify();
    }

    bool CursorData::FromJSON(amf::JSONParser::Node* root)
    {
        m_Width = ((amf::JSONParser::Value*)root->GetElementByName(TAG_WIDTH))->GetValueAsInt32();
        m_Height = ((amf::JSONParser::Value*)root->GetElementByName(TAG_HEIGHT))->GetValueAsInt32();
        m_Pitch = ((amf::JSONParser::Value*)root->GetElementByName(TAG_PITCH))->GetValueAsInt32();
        m_HotspotX = ((amf::JSONParser::Value*)root->GetElementByName(TAG_HOTSPOT_X))->GetValueAsInt32();
        m_HotspotY = ((amf::JSONParser::Value*)root->GetElementByName(TAG_HOTSPOT_Y))->GetValueAsInt32();

        amf::JSONParser::Element* captureResolutionElement = nullptr;

        captureResolutionElement = root->GetElementByName(TAG_CAPTURE_RESOLUTION_X);
        m_CaptureResolutionX = captureResolutionElement == nullptr ? 0 : ((amf::JSONParser::Value*)captureResolutionElement)->GetValueAsInt32();

        captureResolutionElement = root->GetElementByName(TAG_CAPTURE_RESOLUTION_Y);
        m_CaptureResolutionY = captureResolutionElement == nullptr ? 0 : ((amf::JSONParser::Value*)captureResolutionElement)->GetValueAsInt32();

        m_Visible = ((amf::JSONParser::Value*)root->GetElementByName(TAG_VISIBLE))->GetValueAsBool();
        m_Monochrome = ((amf::JSONParser::Value*)root->GetElementByName(TAG_MONOCHROME))->GetValueAsBool();
        return true;
    }

}