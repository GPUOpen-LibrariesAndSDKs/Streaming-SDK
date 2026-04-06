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

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

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

#include "MouseCursorAndroid.h"
#include "sdk/util/jni/JavaENV.h"
#include "sdk/util/jni/JavaObjectHolder.h"

#include "public/common/TraceAdapter.h"

#include <android/bitmap.h>

#define AMF_FACILITY L"MouseCursorAndroid"

namespace ssdk::ctls
{
    MouseCursorAndroid::MouseCursorAndroid(ssdk::util::JavaObjectHolder& activity) :
        m_Activity(activity)
    {
    }

    void MouseCursorAndroid::OnCursorChanged(const transport_common::Cursor& cursor)
    {
        amf::AMFSurfacePtr bitmap;
        const_cast<transport_common::Cursor&>(cursor).GetBitmap(&bitmap);
        if (bitmap != nullptr)
        {
            AMFPoint hotspot = cursor.GetHotspot();
            SetCursorFromAMFSurface(bitmap, hotspot.x, hotspot.y, cursor.GetType() == transport_common::Cursor::Type::MONOCHROME);
            AMFTraceInfo(AMF_FACILITY, L"Mouse cursor changed");
        }
        else
        {
            HideCursor();
            AMFTraceInfo(AMF_FACILITY, L"Mouse cursor hidden");
        }
    }

    void MouseCursorAndroid::OnCursorHidden()
    {
        HideCursor();
        AMFTraceInfo(AMF_FACILITY, L"Mouse cursor hidden");
    }

    static inline void CopyLine_BGRA(const uint8_t* s, uint8_t* d, uint32_t w)
    {
        for (uint32_t x = 0; x < w; ++x)
        {
            uint32_t fourX = 4 * x;
            uint8_t B = s[fourX + 0];
            uint8_t G = s[fourX + 1];
            uint8_t R = s[fourX + 2];
            uint8_t A = s[fourX + 3];

            if (A != 0xFF)
            {
                float a = A / 255.f;
                R = static_cast<uint8_t>(R * a + .5f);
                G = static_cast<uint8_t>(G * a + .5f);
                B = static_cast<uint8_t>(B * a + .5f);
            }
            d[fourX + 0] = R;
            d[fourX + 1] = G;
            d[fourX + 2] = B;
            d[fourX + 3] = A;
        }
    }

    bool  MouseCursorAndroid::SetCursorFromAMFSurface(const amf::AMFSurfacePtr& surface, float hotX, float hotY, bool monoIBeam)
    {
        ssdk::util::JavaENV env;
        ssdk::util::JavaENV::AttachCurrentThread(env);

        ssdk::util::JavaObjectHolder windowHolder = std::move(m_Activity.CallObjectMethod(env, "getWindow", "()Landroid/view/Window;"));
        ssdk::util::JavaObjectHolder viewHolder = std::move(windowHolder.CallObjectMethod(env, "getDecorView", "()Landroid/view/View;"));

        if (monoIBeam == true)  //  I-beam / monochrome path
        {
            constexpr int TYPE_TEXT = 1008;                // android.view.PointerIcon.TYPE_TEXT
            ssdk::util::JavaObjectHolder iconHolder = std::move(env.CallStaticMethod("getSystemIcon","android/view/PointerIcon","(Landroid/content/Context;I)Landroid/view/PointerIcon;",
                    static_cast<jobject>(m_Activity), static_cast<jint>(TYPE_TEXT)));

            if (iconHolder != nullptr)
            {
                viewHolder.CallVoidMethod(env, "setPointerIcon", "(Landroid/view/PointerIcon;)V", static_cast<jobject>(iconHolder));
                return env->ExceptionCheck();
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Failed to get system icon");
                return false;
            }
        }
        else    //  Color cursor
        {
            amf::AMFPlane* pl = surface->GetPlaneAt(0);
            const uint32_t width = pl->GetWidth();
            const uint32_t height = pl->GetHeight();
            const uint32_t pitch = pl->GetHPitch();
            const uint8_t* src = static_cast<const uint8_t *>(pl->GetNative());
            if (src == nullptr || width == 0 || height == 0)
            {
                AMFTraceError(AMF_FACILITY, L"Invalid cursor surface: src: %p, width: %u, height: %u", src, width, height);
                return false;
            }

            jclass clsCfg = env->FindClass("android/graphics/Bitmap$Config");
            jfieldID fid = env->GetStaticFieldID(clsCfg, "ARGB_8888",
                                                 "Landroid/graphics/Bitmap$Config;");
            jobject argb8888Cfg = env->GetStaticObjectField(clsCfg, fid);

            ssdk::util::JavaObjectHolder bitmapHolder = std::move(env.CallStaticMethod(
                    "createBitmap",
                    "android/graphics/Bitmap",
                    "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;",
                    static_cast<jint>(width),
                    static_cast<jint>(height),
                    argb8888Cfg));

            if (bitmapHolder == nullptr)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create bitmap");
                return false;
            }

            AndroidBitmapInfo info{};
            void *dstPixels = nullptr;

            if (AndroidBitmap_getInfo(env, bitmapHolder, &info) != ANDROID_BITMAP_RESULT_SUCCESS ||
                AndroidBitmap_lockPixels(env, bitmapHolder, &dstPixels) !=
                ANDROID_BITMAP_RESULT_SUCCESS)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to lock bitmap pixels");
                return false;
            }

            for (uint32_t y = 0; y < height; ++y)
            {
                const uint8_t *s = src + y * pitch;
                uint8_t *d = static_cast<uint8_t *>(dstPixels) + y * info.stride;
                CopyLine_BGRA(s, d, width);
            }
            AndroidBitmap_unlockPixels(env, bitmapHolder);

            ssdk::util::JavaObjectHolder iconHolder = std::move(env.CallStaticMethod("create", "android/view/PointerIcon",
                                                             "(Landroid/graphics/Bitmap;FF)Landroid/view/PointerIcon;",
                                                             static_cast<jobject>(bitmapHolder), hotX, hotY));
            if (iconHolder == nullptr)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create pointer icon");
                return false;
            }

            viewHolder.CallVoidMethod(env, "setPointerIcon", "(Landroid/view/PointerIcon;)V", static_cast<jobject>(iconHolder));
            if (env->ExceptionCheck())
            {
                env->ExceptionDescribe();
                env->ExceptionClear();
                AMFTraceError(AMF_FACILITY, L"A JNI call to setPointerIcon failed");
                return false;
            }
            return true;
        }
    }

    bool MouseCursorAndroid::HideCursor()
    {
        ssdk::util::JavaENV env;
        ssdk::util::JavaENV::AttachCurrentThread(env);
        constexpr int TYPE_NULL = 0;  // Invisible cursor

        // Get invisible cursor icon
        ssdk::util::JavaObjectHolder invisibleIcon = env.CallStaticMethod( "getSystemIcon", "android/view/PointerIcon", "(Landroid/content/Context;I)Landroid/view/PointerIcon;",
                static_cast<jobject>(m_Activity), static_cast<jint>(TYPE_NULL));

        // Get window and view
        ssdk::util::JavaObjectHolder windowHolder = m_Activity.CallObjectMethod(env, "getWindow", "()Landroid/view/Window;");
        ssdk::util::JavaObjectHolder viewHolder = windowHolder.CallObjectMethod(env, "getDecorView", "()Landroid/view/View;");

        // Set invisible cursor
        viewHolder.CallVoidMethod(env, "setPointerIcon", "(Landroid/view/PointerIcon;)V", static_cast<jobject>(invisibleIcon));
        if (env->ExceptionCheck())
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
            AMFTraceError(AMF_FACILITY, L"A JNI call to setPointerIcon failed");
            return false;
        }
        return true;
    }

} // namespace ssdk::cursor

