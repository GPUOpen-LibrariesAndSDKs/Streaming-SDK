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

#include <memory>
#include <systemd/sd-bus.h>

//-------------------------------------------------------------------------------------------------
// SDBusImportTable class
//-------------------------------------------------------------------------------------------------
namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    struct SDBusImportTable
    {

        SDBusImportTable();
        ~SDBusImportTable();

        AMF_RESULT LoadFunctionsTable();
        void UnloadFunctionsTable();

        decltype(&sd_bus_open_system)         m_pSDB_Open_System = nullptr;
        decltype(&sd_bus_open_user)           m_pSDB_Open_User = nullptr;

        decltype(&sd_bus_process)             m_pSDB_Process = nullptr;
        decltype(&sd_bus_wait)                m_pSDB_Wait = nullptr;

        decltype(&sd_bus_get_unique_name)     m_pSDB_Get_Unique_Name = nullptr;

        decltype(&sd_bus_message_new_method_call) m_pSDB_Message_New_Method_Call = nullptr;

        decltype(&sd_bus_message_append) m_pSDB_Message_Append = nullptr;

        decltype(&sd_bus_message_read) m_pSDB_Message_Read = nullptr;
        decltype(&sd_bus_message_skip) m_pSDB_Message_Skip = nullptr;
        decltype(&sd_bus_message_peek_type) m_pSDB_Message_Peek_Type = nullptr;

        decltype(&sd_bus_message_open_container) m_pSDB_Message_Open_Container = nullptr;
        decltype(&sd_bus_message_close_container) m_pSDB_Message_Close_Container = nullptr;
        decltype(&sd_bus_message_enter_container) m_pSDB_Message_Enter_Container = nullptr;
        decltype(&sd_bus_message_exit_container) m_pSDB_Message_Exit_Container = nullptr;

        decltype(&sd_bus_match_signal) m_pSDB_Match_Signal = nullptr;

        decltype(&sd_bus_call)                 m_pSDB_Call = nullptr;
        decltype(&sd_bus_get_property_trivial) m_pSDB_Get_Property_Trivial = nullptr;

        decltype(&sd_bus_ref)               m_pSDB_Ref = nullptr;
        decltype(&sd_bus_message_ref)       m_pSDB_Message_Ref = nullptr;
        decltype(&sd_bus_slot_ref)          m_pSDB_Slot_Ref = nullptr;

        decltype(&sd_bus_unref)               m_pSDB_Unref = nullptr;
        decltype(&sd_bus_message_unref)       m_pSDB_Message_Unref = nullptr;
        decltype(&sd_bus_slot_unref)          m_pSDB_Slot_Unref = nullptr;

    private:
        amf_handle                            m_hLibSystemdSO = nullptr;
    };

    typedef std::shared_ptr<SDBusImportTable> SDBusImportTablePtr;

} // namespace ssdk::ctls

#endif // __linux
