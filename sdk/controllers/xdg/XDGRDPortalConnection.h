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

#if defined(__linux)

#include "public/include/core/Result.h"
#include "public/include/core/Platform.h"
#include "public/common/Thread.h"
#include "DBusConnection.h"

//-------------------------------------------------------------------------------------------------
// This is a wrapper around org.freedesktop.portal.RemoteDesktop
//-------------------------------------------------------------------------------------------------
namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    class XDGRDPortalConnection
    {
    public:
        // using an enum here causes a problem when we want to do stuff like (bitmask & XDG_SOURCE_TYPE_MONITOR)
        typedef int SOURCE_TYPE;
        static constexpr int SOURCE_TYPE_NONE = 0;
        static constexpr int SOURCE_TYPE_KEYBOARD = (1 << 0);
        static constexpr int SOURCE_TYPE_POINTER = (1 << 1);
        static constexpr int SOURCE_TYPE_TOUCHSCREEN = (1 << 2);

        XDGRDPortalConnection();
        ~XDGRDPortalConnection();

        AMF_RESULT Initialize(DBusConnection* pDBus, const std::string& handleToken);
        AMF_RESULT Terminate();

        // Return a bitmask of available devide types
        AMF_RESULT AvailableDeviceTypes(SOURCE_TYPE& types);

        // Create Remote Desktop portal session
        AMF_RESULT CreateSession(const std::string& sessionHandleToken);

        // Select device type
        AMF_RESULT SelectDevices(SOURCE_TYPE sources, const std::string& restoreToken);

        // Start session
        AMF_RESULT Start(const std::string& parentWindow, const AbortController& controller);

        // The dx and dy values indicate the change in position since the last pointer event
        AMF_RESULT NotifyPointerMotion(double dx, double dy);
        
        // Warp mouse cursor position using NotifyPointerMotionAbsolute,
        // x and y are normalized coordinates [0, 1] (e.g., 0.5, 0.5 for the center of the screen)
        AMF_RESULT NotifyPointerMotionAbsolute(double x, double y);
    private:
        AMF_RESULT GetPropertyTrivial(const char* pProperty, char type, void* pData);
        AMF_RESULT CreateResponseSlot(SDBusSlotPtr& slot);

        DBusConnection*         m_pDBus = nullptr;
        std::string             m_handleToken;
        std::string             m_sessionHandle;
        amf::AMFCriticalSection m_responseSect;
    };

} // namespace ssdk::ctls

#endif // __linux
