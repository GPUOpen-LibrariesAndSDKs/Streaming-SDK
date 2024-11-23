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

#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/PropertyStorage.h"
#include "amf/public/include/core/Buffer.h"
#include "amf/public/include/core/Surface.h"

#include <vector>
#include <memory>
#include <tuple>
#include <string>

namespace ssdk::transport_common
{
    class Cursor
    {
    public:
        enum class Type
        {
            COLOR,
            MONOCHROME
        };
    public:
        Cursor() = default;
        explicit Cursor(amf::AMFSurface* bitmap, const AMFPoint& hotspot, const AMFSize& resolution, Type type);

        AMF_RESULT GetBitmap(amf::AMFSurface** bitmap) noexcept;
        inline const AMFPoint& GetHotspot() const noexcept { return m_Hotspot; }
        inline const AMFSize& GetServerDisplayResolution() const noexcept { return m_ServerDisplayResolution; }
        inline Type GetType() const noexcept { return m_Type; }

    private:
        amf::AMFSurfacePtr  m_Bitmap;
        AMFPoint            m_Hotspot = {};
        AMFSize             m_ServerDisplayResolution = {};
        Type                m_Type = Type::COLOR;
    };

    class VideoFrame
    {
    public:
        enum class ViewType
        {
            LEFT = 0,
            RIGHT = 1,
            MONOSCOPIC = 2,
            MULTIVIEW = 3
        };

        enum class SubframeType
        {
            UNKNOWN = -1,
            IDR = 0,
            I,
            P,
            B,
            SLICE,
            TRANSPARENCY
        };

        struct Subframe
        {
            typedef std::vector<Subframe>  Collection;

            amf::AMFBufferPtr   m_Buf;
            SubframeType        m_Type = SubframeType::UNKNOWN;
        };


    protected:
        explicit VideoFrame(ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);
        explicit VideoFrame(uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);

    public:
        virtual ~VideoFrame() = default;

        inline size_t GetSubframeCount() const noexcept { return m_Subframes.size(); }
        inline SubframeType GetSubframeType(size_t idx) const { return m_Subframes[idx].m_Type; }
        AMF_RESULT GetSubframeBuffer(size_t idx, amf::AMFBuffer** buf) const;

        inline ViewType GetViewType() const noexcept { return m_ViewType; }
        inline uint32_t GetViewIndex() const noexcept { return m_ViewIdx; }     //  View index is only valid for the MULTIVIEW type (for use cases like videowalls)

        inline int64_t GetSequenceNumber() const noexcept { return m_SequenceNumber; }

        inline amf_pts GetPts() const noexcept { return m_Pts; }
        inline amf_pts GetDuration()const noexcept { return m_Duration; }

        inline amf_pts GetOriginPts() const noexcept { return m_OriginPts; }

        inline bool IsDiscontinuity() const noexcept { return m_Discontinuity; }

    protected:
        amf_pts                 m_OriginPts = 0;
        Subframe::Collection    m_Subframes;
        ViewType                m_ViewType = ViewType::MONOSCOPIC;
        uint32_t                m_ViewIdx = 0;
        int64_t                 m_SequenceNumber = 0;
        amf_pts                 m_Pts = -1;
        amf_pts                 m_Duration = -1;
        bool                    m_Discontinuity = false;
    };

    class TransmittableVideoFrame : public VideoFrame
    {
    public:
        explicit TransmittableVideoFrame(ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);
        explicit TransmittableVideoFrame(uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);

        AMF_RESULT AddSubframe(SubframeType type, amf::AMFBuffer* subframe);

        size_t CalculateRequiredBufferSize() const noexcept;
        AMF_RESULT ConstructFrame(void* buf) const;
        size_t GetSubframeOfs(size_t idx) const;
    };

    class ReceivableVideoFrame : public VideoFrame
    {
    public:
        explicit ReceivableVideoFrame(amf::AMFContext* context, ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);
        explicit ReceivableVideoFrame(amf::AMFContext* context, uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity);

        inline void RegisterSubframe(size_t ofs, size_t size, SubframeType type) { m_RegisteredSubframes.push_back(RegisteredSubframe(ofs, size, type)); }
        AMF_RESULT ParseBuffer(const void* buf);

        inline void SetPts(amf_pts pts) noexcept { m_Pts = pts; }
        inline void SetDuration(amf_pts duration) noexcept { m_Duration = duration; }

    protected:
        amf::AMFContextPtr  m_Context;
        typedef std::tuple<size_t, size_t, SubframeType> RegisteredSubframe;
        std::vector<RegisteredSubframe> m_RegisteredSubframes;
    };

    class VideoStreamDescriptor
    {
    public:
        enum class ColorDepth
        {
            SDR_8,
            HDR_10
        };

        enum class ViewType
        {
            MONOSCOPIC,
            STEREOSCOPIC,
            MULTIVIEW
        };

    public:
        VideoStreamDescriptor() = default;
        VideoStreamDescriptor(uint32_t sourceID, const char* description, const char* codec, const AMFSize& resolution, float framerate, int32_t bitrate,
                              ColorDepth colordepth = ColorDepth::SDR_8,
                              ViewType viewType = ViewType::MONOSCOPIC, size_t viewCount = 1);

        inline uint32_t GetSourceID() const noexcept { return m_SourceID; }
        inline void SetSourceID(uint32_t id) noexcept { m_SourceID = id; }

        inline const std::string& GetDescription() const noexcept { return m_Description; }
        inline void SetDescription(const std::string& desc) noexcept { m_Description = desc; }

        inline const std::string& GetCodec() const noexcept { return m_Codec; }
        inline void SetCodec(const std::string& codec) noexcept { m_Codec = codec; }

        inline const AMFSize& GetResolution() const noexcept { return m_Resolution; }
        inline void SetResolution(const AMFSize& res) noexcept { m_Resolution = res; }

        inline float GetFramerate() const noexcept { return m_Framerate; }
        inline void SetFramerate(float rate) noexcept { m_Framerate = rate; }

        inline int32_t GetBitrate() const noexcept { return m_Bitrate; }
        inline void SetBitrate(int32_t rate) noexcept { m_Bitrate = rate; }

        inline ColorDepth GetColorDepth() const noexcept { return m_Depth; }
        inline void SetColorDepth(ColorDepth depth) noexcept { m_Depth = depth; }

    protected:
        std::string     m_Description;
        std::string     m_Codec;
        uint32_t        m_SourceID = 0;
        AMFSize         m_Resolution = {};
        float           m_Framerate = 0;
        int32_t         m_Bitrate = 0;
        ColorDepth      m_Depth = ColorDepth::SDR_8;
        ViewType        m_ViewType = ViewType::MONOSCOPIC;
        size_t          m_ViewCount = 1;
    };
}