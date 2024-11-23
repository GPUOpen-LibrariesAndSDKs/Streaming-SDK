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

#include "Video.h"

#include "amf/public/common/TraceAdapter.h"

#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_common::VideoFrame";


namespace ssdk::transport_common
{
    VideoFrame::VideoFrame(ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        m_ViewType(viewType),
        m_OriginPts(originPts),
        m_SequenceNumber(sequenceNumber),
        m_Discontinuity(discontinuity)
    {
    }

    VideoFrame::VideoFrame(uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        m_ViewType(ViewType::MULTIVIEW),
        m_ViewIdx(viewIdx),
        m_OriginPts(originPts),
        m_SequenceNumber(sequenceNumber),
        m_Discontinuity(discontinuity)
    {
    }

    AMF_RESULT VideoFrame::GetSubframeBuffer(size_t idx, amf::AMFBuffer** buf) const
    {
        AMF_RESULT result = AMF_OK;

        if (buf == nullptr)
        {
            result = AMF_INVALID_ARG;
        }
        else if (idx >= m_Subframes.size())
        {
            result = AMF_OUT_OF_RANGE;
        }
        else
        {
            (*buf) = m_Subframes[idx].m_Buf;
            (*buf)->Acquire();
        }

        return result;
    }

    /////////////////////////////////////////////////////////////////////////////////
    TransmittableVideoFrame::TransmittableVideoFrame(ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        VideoFrame(viewType, originPts, sequenceNumber, discontinuity)
    {
    }

    TransmittableVideoFrame::TransmittableVideoFrame(uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        VideoFrame(viewIdx, originPts, sequenceNumber, discontinuity)
    {
    }

    AMF_RESULT TransmittableVideoFrame::AddSubframe(SubframeType type, amf::AMFBuffer* subframe)
    {
        AMF_RESULT result = AMF_OK;
        if (subframe == nullptr || type == SubframeType::UNKNOWN)
        {
            result = AMF_INVALID_ARG;
        }
        else
        {
            if (m_Pts == -1)
            {
                m_Pts = subframe->GetPts();
                m_Duration = subframe->GetDuration();
            }
            if (m_Pts != subframe->GetPts() || m_Duration != subframe->GetDuration())
            {
                result = AMF_INVALID_ARG;
                AMFTraceError(AMF_FACILITY, L"Subframe timestamp or duration mismatch: frame(pts=%lld, duration=%lld), subframe(pts=%lld, duration=%lld)",
                              m_Pts, subframe->GetPts(), m_Duration, subframe->GetDuration());
            }
            else
            {
                m_Subframes.push_back({ subframe, type });
            }
        }
        return result;
    }

    size_t TransmittableVideoFrame::CalculateRequiredBufferSize() const noexcept
    {
        size_t totalSize = 0;
        for (auto& it : m_Subframes)
        {
            totalSize += it.m_Buf->GetSize();
        }
        return totalSize;
    }

    AMF_RESULT TransmittableVideoFrame::ConstructFrame(void* buf) const
    {
        AMF_RESULT result = AMF_OK;
        if (buf == nullptr)
        {
            result = AMF_INVALID_ARG;
        }
        else
        {
            uint8_t* dest = static_cast<uint8_t*>(buf);
            for (auto& it : m_Subframes)
            {
                size_t curSize = it.m_Buf->GetSize();
                memcpy(dest, it.m_Buf->GetNative(), curSize);
                dest += curSize;
            }
        }
        return result;
    }

    size_t TransmittableVideoFrame::GetSubframeOfs(size_t idx) const
    {
        size_t ofs = 0;
        for (size_t i = 1; i < idx; i++)
        {
            ofs += m_Subframes[i - 1].m_Buf->GetSize();
        }
        return ofs;
    }


    /////////////////////////////////////////////////////////////////////////////////
    static constexpr const std::size_t SF_OFS = 0, SF_SIZE = 1, SF_TYPE = 2;

    ReceivableVideoFrame::ReceivableVideoFrame(amf::AMFContext* context, ViewType viewType, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        VideoFrame(viewType, originPts, sequenceNumber, discontinuity),
        m_Context(context)
    {
    }

    ReceivableVideoFrame::ReceivableVideoFrame(amf::AMFContext* context, uint32_t viewIdx, amf_pts originPts, int64_t sequenceNumber, bool discontinuity) :
        VideoFrame(viewIdx, originPts, sequenceNumber, discontinuity),
        m_Context(context)
    {
    }

    AMF_RESULT ReceivableVideoFrame::ParseBuffer(const void* buf)
    {
        AMF_RESULT result = AMF_OK;
        if (m_Context == nullptr || m_RegisteredSubframes.size() == 0)
        {
            result = AMF_NOT_INITIALIZED;
        }
        else
        {
            m_Subframes.clear();
            for (auto& it : m_RegisteredSubframes)
            {
                amf::AMFBufferPtr subframeBuf;
                if ((result = m_Context->AllocBuffer(amf::AMF_MEMORY_HOST, std::get<SF_SIZE>(it), &subframeBuf)) != AMF_OK)
                {
                    break;
                }
                else
                {
                    memcpy(subframeBuf->GetNative(), static_cast<const uint8_t*>(buf) + std::get<SF_OFS>(it), std::get<SF_SIZE>(it));
                    m_Subframes.push_back({ subframeBuf, std::get<SF_TYPE>(it) });
                    subframeBuf->SetPts(m_Pts);
                    subframeBuf->SetDuration(m_Duration);
                }
            }
        }
        return result;
    }

    /////////////////////////////////////////////////////////////////////////////////
    Cursor::Cursor(amf::AMFSurface* bitmap, const AMFPoint& hotspot, const AMFSize& resolution, Type type) :
        m_Bitmap(bitmap),
        m_Hotspot(hotspot),
        m_ServerDisplayResolution(resolution),
        m_Type(type)
    {
    }

    AMF_RESULT Cursor::GetBitmap(amf::AMFSurface** bitmap) noexcept
    {
        AMF_RESULT result = AMF_OK;
        if (bitmap == nullptr)
        {
            result = AMF_INVALID_ARG;
        }
        else if (m_Bitmap == nullptr)
        {
            result = AMF_NOT_INITIALIZED;
        }
        else
        {
            (*bitmap) = m_Bitmap;
            (*bitmap)->Acquire();
        }
        return result;
    }


    /////////////////////////////////////////////////////////////////////////////////
    VideoStreamDescriptor::VideoStreamDescriptor(uint32_t sourceID, const char* description, const char* codec, const AMFSize& resolution, float framerate, int32_t bitrate,
                                                 ColorDepth colordepth, ViewType viewType, size_t viewCount) :
        m_SourceID(sourceID),
        m_Codec(codec),
        m_Resolution(resolution),
        m_Framerate(framerate),
        m_Bitrate(bitrate),
        m_Depth(colordepth),
        m_ViewType(viewType),
        m_ViewCount(viewCount)
    {
        if (description != nullptr)
        {
            m_Description = description;
        }
        else
        {
            std::stringstream desc;
            desc << codec << ", " << resolution.width << 'x' << resolution.height << '@' << framerate;
            m_Description = desc.str();
        }
    }
}
