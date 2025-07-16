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

#if defined(__linux)

#include "MouseCursorLnx.h"

#include <X11/Xcursor/Xcursor.h>

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    MouseCursorLnx::MouseCursorLnx(bool bShowCursor, bool bFullScreen, Display* dpy, amf_handle hWindow) :
        MouseCursor(bShowCursor, bFullScreen, hWindow),
        m_dpy(new XDisplay(dpy))
    {
        const char data[1] = { 0 };
        XColor empty;
        Pixmap cursorPixmap = XCreateBitmapFromData(dpy, (Window)hWindow, data, 1, 1);
        m_emptyCursor = XCreatePixmapCursor(dpy, cursorPixmap, cursorPixmap, &empty, &empty, 0, 0);
        XFreePixmap(dpy, cursorPixmap);

        if (bShowCursor == false)
        {
            XDefineCursor(dpy, (Window)hWindow, m_emptyCursor);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorLnx::Present()
    {
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorLnx::OnCursorChanged(const transport_common::Cursor& cursor)
    {
        amf::AMFLock lock(&m_cs);
        XDisplayPtr dpy(m_dpy);

        amf::AMFSurfacePtr pCursorBitmap;
        if (const_cast<transport_common::Cursor&>(cursor).GetBitmap(&pCursorBitmap) != AMF_OK || nullptr == pCursorBitmap)
        {
            XDefineCursor(dpy, (Window)m_hWindow, m_emptyCursor);
            m_pCursor = nullptr;
            return;
        }

        m_pCursor = std::unique_ptr<transport_common::Cursor>(
            new transport_common::Cursor(pCursorBitmap, cursor.GetHotspot(), cursor.GetServerDisplayResolution(), cursor.GetType())
            );

        amf::AMFPlanePtr plane = pCursorBitmap->GetPlaneAt(0);
        int pixmapwidth = plane->GetWidth();
        int pixmapHeight = plane->GetHeight();
        amf_int32 pitch = plane->GetHPitch();
        amf_uint8* pixels = (amf_uint8*)plane->GetNative();

        XcursorImage* xcursorImage = NULL;
        xcursorImage = XcursorImageCreate(pixmapwidth, pixmapHeight);

        if (!xcursorImage)
        {
            XDefineCursor(dpy, (Window)m_hWindow, m_emptyCursor);
            return;
        }

        if (pixmapwidth == 0 && pixmapHeight == 0)
        {
            XDefineCursor(dpy, (Window)m_hWindow, m_emptyCursor);
            return;
        }

        xcursorImage->xhot = 0;
        xcursorImage->yhot = 0;
        xcursorImage->delay = 0;

        for (int y = 0; y < pixmapHeight; ++y)
        {
            for (int x = 0; x < pixmapwidth; ++x)
            {
                amf_uint8* pixel = pixels + (y * pitch + x * 4);

                amf_uint32 a = *pixel;
                amf_uint32 r = *(pixel + 1);
                amf_uint32 g = *(pixel + 2);
                amf_uint32 b = *(pixel + 3);
                xcursorImage->pixels[y * pixmapwidth + x] = (b << 24) | (g << 16) | (r << 8) | a;
            }
        }

        m_cursor = XcursorImageLoadCursor(dpy, xcursorImage);
        XDefineCursor(dpy, (Window)m_hWindow, m_cursor);

        if (xcursorImage)
        {
            XcursorImageDestroy(xcursorImage);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorLnx::OnCursorHidden()
    {
        amf::AMFLock lock(&m_cs);
        XDisplayPtr dpy(m_dpy);
        XDefineCursor(dpy, (Window)m_hWindow, m_emptyCursor);
        m_pCursor = nullptr;
    }

} // namespace ssdk::ctls

#endif // __linux
