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

#if defined(_WIN32)

#include "MouseCursorWin.h"
#include "amf/public/common/ByteArray.h"

#define WM_USER_SHOW_CURSOR WM_USER+1

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    MouseCursorWin::MouseCursorWin(bool bShowCursor, bool bFullScreen, amf_handle hWindow) :
        MouseCursor(bShowCursor, bFullScreen, hWindow),
        m_hCursor(0)
    {
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorWin::Present()
    {
        amf::AMFLock lock(&m_cs);

        if (m_pCursor == nullptr)
        {
            return;
        }

        if (m_hCursor == 0)
        {
            amf::AMFPlanePtr pPlane;
            amf::AMFSurfacePtr pCursorBitmap;
            if (m_pCursor->GetBitmap(&pCursorBitmap) == AMF_OK && nullptr != pCursorBitmap)
            {
                pPlane = pCursorBitmap->GetPlaneAt(0);
            }

            HICON hic = NULL;
            if (m_pCursor->GetType() == transport_common::Cursor::Type::MONOCHROME)
            {
                hic = CreateMonoCursor(pPlane);
            }
            else
            {
                hic = CreateColorCursor(pPlane);
            }

            AMFPoint hs = m_pCursor->GetHotspot();
            ICONINFO iconinfo = { 0 };

            GetIconInfo(hic, &iconinfo);

            iconinfo.fIcon = FALSE;
            iconinfo.xHotspot = hs.x;
            iconinfo.yHotspot = hs.y;

            m_hCursor = ::CreateIconIndirect(&iconinfo);

            DestroyIcon(hic);
        }

        if (m_hCursor != 0)
        {
            SetCursor(m_hCursor);
            ShowCursor(TRUE);
        }
    }

    //-------------------------------------------------------------------------------------------------
    bool MouseCursorWin::ProcessMessage(const WindowMessage& msg)
    {
        bool bResult = false;
        switch (msg.message)
        {
        case WM_ACTIVATE:
        {
            bool active = LOWORD(msg.wParam) != WA_INACTIVE; // Check Minimization State
            if (active)
            {
                if (m_bFullScreen)
                {
                    HMONITOR hMonitor = MonitorFromWindow((HWND)m_hWindow, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = {};
                    mi.cbSize = sizeof(mi);
                    GetMonitorInfo(hMonitor, &mi);
                    RECT rect = mi.rcMonitor;
                    ClipCursor(&rect);
                }

            }
            else
            {
                ClipCursor(NULL);
            }
        }
            break;
        case WM_SETCURSOR:
            Present();
            bResult = true;
            break;
        case WM_USER_SHOW_CURSOR:
            if (msg.lParam == 0)
            {
                int count = 0;
                while (count >= 0)
                {
                    count = ShowCursor(FALSE);
                }
            }
            else
            {
                ShowCursor(TRUE);
            }
            bResult = true;
            break;
        default:
            bResult = false;
            break;
        }

        return bResult;
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorWin::OnCursorChanged(const transport_common::Cursor& cursor)
    {
        amf::AMFLock lock(&m_cs);

        bool bShownOld = m_pCursor != nullptr;

        amf::AMFSurfacePtr pCursorBitmap;
        if (const_cast<transport_common::Cursor&>(cursor).GetBitmap(&pCursorBitmap) != AMF_OK || nullptr == pCursorBitmap ||
            pCursorBitmap->GetPlaneAt(0)->GetWidth() == 0 ||
            pCursorBitmap->GetPlaneAt(0)->GetHeight() == 0)
        {
            m_pCursor = nullptr;
        }
        else
        {
            m_pCursor = std::unique_ptr<transport_common::Cursor>(
                new transport_common::Cursor(pCursorBitmap, cursor.GetHotspot(), cursor.GetServerDisplayResolution(), cursor.GetType())
                );
        }

        bool bShownNew = m_pCursor != nullptr;

        if (m_hCursor != 0)
        {
            DestroyIcon(m_hCursor);
            m_hCursor = 0;
        }

        if (bShownOld != bShownNew)
        {
            if (m_pCursor == nullptr)
            {
                ::PostMessage(HWND(m_hWindow), WM_USER_SHOW_CURSOR, 0, FALSE);
            }
            else
            {
                ::PostMessage(HWND(m_hWindow), WM_USER_SHOW_CURSOR, 0, TRUE);
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseCursorWin::OnCursorHidden()
    {
        ::PostMessage(HWND(m_hWindow), WM_USER_SHOW_CURSOR, 0, FALSE);
        amf::AMFLock lock(&m_cs);
        m_pCursor = nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    HICON MouseCursorWin::CreateColorCursor(amf::AMFPlanePtr pPlane)
    {
        if (pPlane == nullptr)
        {
            return NULL;
        }
        int width = pPlane->GetWidth();
        int height = pPlane->GetHeight();
        int pitch = pPlane->GetHPitch();

        AMFByteArray andBits;
        AMFByteArray xorBits;

        andBits.SetSize((width + 7) / 8 * height);
        xorBits.SetSize(width * 4 * height);

        // For color cursor, the xor masks has the rgb value
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                amf_uint8* pixel = (amf_uint8*)pPlane->GetNative() + (y * pitch + x * 4);

                amf_uint8 b = *pixel;
                amf_uint8 g = *(pixel + 1);
                amf_uint8 r = *(pixel + 2);
                amf_uint8 a = *(pixel + 3);

                if (a == 0)
                {
                    andBits[(x + y * width) / 8] |= 1 << (7 - x % 8);
                    xorBits[(x + y * width) * 4] = 0;
                    xorBits[(x + y * width) * 4 + 1] = 0;
                    xorBits[(x + y * width) * 4 + 2] = 0;
                    xorBits[(x + y * width) * 4 + 3] = 0;
                }
                else
                {
                    andBits[(x + y * width) / 8] |= 0 << (7 - x % 8);
                    xorBits[(x + y * width) * 4] = b;
                    xorBits[(x + y * width) * 4 + 1] = g;
                    xorBits[(x + y * width) * 4 + 2] = r;
                    xorBits[(x + y * width) * 4 + 3] = a;
                }
            }
        }

        HICON hic = CreateIcon(NULL, width, height, 1, 32, andBits.GetData(), xorBits.GetData());

        return hic;
    }

    //-------------------------------------------------------------------------------------------------
    HICON MouseCursorWin::CreateMonoCursor(amf::AMFPlanePtr pPlane)
    {
        if (pPlane == nullptr)
        {
            return NULL;
        }
        int width = pPlane->GetWidth();
        int height = pPlane->GetHeight();
        int pitch = pPlane->GetHPitch();

        AMFByteArray andBits;
        AMFByteArray xorBits;

        andBits.SetSize(width / 8 * height);
        xorBits.SetSize(width / 8 * height);

        // For monochrome cursors, the xor masks is 1 bit per pixel
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                amf_uint8* pixel = (amf_uint8*)pPlane->GetNative() + (y * pitch + x * 4);

                //amf_uint8 b = *pixel;
                //amf_uint8 g = *(pixel + 1);
                //amf_uint8 r = *(pixel + 2);
                amf_uint8 a = *(pixel + 3);

                if (a == 0)
                {
                    andBits[(x + y * width) / 8] |= 1 << (7 - x % 8);
                    xorBits[(x + y * width) / 8] |= 0 << (7 - x % 8);
                }
                else
                {
                    andBits[(x + y * width) / 8] |= 1 << (7 - x % 8);
                    xorBits[(x + y * width) / 8] |= 1 << (7 - x % 8);
                }
            }
        }

        HICON hic = CreateIcon(NULL, width, height, 1, 1, andBits.GetData(), xorBits.GetData());

        return hic;
    }

} // namespace ssdk::ctls

#endif // _WIN32
