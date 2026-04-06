cmake_minimum_required(VERSION 3.15)

# Define the project name
project(amf-public)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-value -Wno-deprecated-declarations")
endif ()

set(AMF_LOCATION "../../amf/amf")

# Define source files
set(HLSL_PIXEL_SHADER_FILES
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX11_ps.hlsl"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX12_ps.hlsl"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX9_ps.hlsl"
)

set(HLSL_VERTEX_SHADER_FILES
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX11_vs.hlsl"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX12_vs.hlsl"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadDX9_vs.hlsl"
)

set(HLSL_COMPUTE_SHADER_FILES
    "${AMF_LOCATION}/public/src/components/DisplayCapture/DrawRectsBGRA.hlsl"
)

set(VLSL_SHADER_FILES
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadVulkan_fs.frag"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadVulkan_vs.vert"
)

set(ALL_PLATFORMS_SOURCE_FILES
    "${AMF_LOCATION}/public/common/AMFFactory.cpp"
    "${AMF_LOCATION}/public/common/AMFSTL.cpp"
    "${AMF_LOCATION}/public/common/CurrentTimeImpl.cpp"
    "${AMF_LOCATION}/public/common/JsonImpl.cpp"
    "${AMF_LOCATION}/public/common/PropertyStorageExImpl.cpp"
    "${AMF_LOCATION}/public/common/Thread.cpp"
    "${AMF_LOCATION}/public/common/TraceAdapter.cpp"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/CmdLogger.cpp"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/MiscHelpers.cpp"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenter.cpp"
)

# Add platform-specific files:
if (WIN32 OR LINUX)
    set(WIN_LINUX_SOURCE_FILES
        "${AMF_LOCATION}/public/common/VulkanImportTable.cpp"
        "${AMF_LOCATION}/public/common/OpenGLImportTable.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/CmdLineParser.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenter.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/BitStreamParser.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/BitStreamParserH264.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/BitStreamParserH265.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/BitStreamParserIVF.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/CaptureVideoPipelineBase.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/OpenCLLoader.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/ParametersStorage.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/Pipeline.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/PlaybackPipeline.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/PlaybackPipelineBase.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/PreProcessingParams.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SurfaceUtils.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChain.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenter.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceVulkan.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainVulkan.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainOpenGL.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterVulkan.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterOpenGL.cpp"
        "${AMF_LOCATION}/public/src/components/AudioCapture/AudioCaptureImpl.cpp"
    )
endif()
if(WIN32)
    set(SOURCE_FILES ${WIN_LINUX_SOURCE_FILES} ${ALL_PLATFORMS_SOURCE_FILES}
        "${AMF_LOCATION}/public/common/Windows/ThreadWindows.cpp"
        "${AMF_LOCATION}/public/common/Windows/UtilsWindows.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX11.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX12.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX9.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX11.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX12.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX9.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDXGI.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDXGIDecode.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX11.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX12.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX9.cpp"
        "${AMF_LOCATION}/public/src/components/DisplayCapture/DDAPISource.cpp"
        "${AMF_LOCATION}/public/src/components/AudioCapture/WASAPISource.cpp"
        "${AMF_LOCATION}/public/src/components/CursorCapture/CursorCaptureWin.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceOpenCL.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceOpenGL.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/Options.cpp"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenterWin.cpp"
        "${AMF_LOCATION}/public/src/components/DisplayCapture/DisplayCaptureImpl.cpp"
    )
elseif(LINUX OR ANDROID)
    if (NOT ANDROID)
        set(SOURCE_FILES ${WIN_LINUX_SOURCE_FILES} ${ALL_PLATFORMS_SOURCE_FILES}
            "${AMF_LOCATION}/public/common/Linux/ThreadLinux.cpp"
            "${AMF_LOCATION}/public/common/Linux/PulseAudioImportTable.cpp"
            "${AMF_LOCATION}/public/src/components/CursorCapture/CursorCaptureLinux.cpp"
            "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenterLinux.cpp"
            "${AMF_LOCATION}/public/src/components/AudioCapture/PulseAudioSimpleAPISource.cpp"
            "${AMF_LOCATION}/public/src/components/AudioCapture/PulseAudioSimpleAPISourceFacade.cpp"
        )
    else()
        set(SOURCE_FILES ${ALL_PLATFORMS_SOURCE_FILES}
            "${AMF_LOCATION}/public/common/Linux/ThreadLinux.cpp"
        )
    endif()

endif()

# Define header files
set(ALL_PLATFORMS_HEADER_FILES
    "${AMF_LOCATION}/public/common/AMFFactory.h"
    "${AMF_LOCATION}/public/common/AMFMath.h"
    "${AMF_LOCATION}/public/common/AMFSTL.h"
    "${AMF_LOCATION}/public/common/ByteArray.h"
    "${AMF_LOCATION}/public/common/CPUCaps.h"
    "${AMF_LOCATION}/public/common/CurrentTimeImpl.h"
    "${AMF_LOCATION}/public/common/InterfaceImpl.h"
    "${AMF_LOCATION}/public/common/Json.h"
    "${AMF_LOCATION}/public/common/JsonImpl.h"
    "${AMF_LOCATION}/public/common/ObservableImpl.h"
    "${AMF_LOCATION}/public/common/PropertyStorageExImpl.h"
    "${AMF_LOCATION}/public/common/PropertyStorageImpl.h"
    "${AMF_LOCATION}/public/common/Thread.h"
    "${AMF_LOCATION}/public/common/TraceAdapter.h"
    "${AMF_LOCATION}/public/common/VulkanImportTable.h"
    "${AMF_LOCATION}/public/common/OpenGLImportTable.h"
    "${AMF_LOCATION}/public/include/core/AudioBuffer.h"
    "${AMF_LOCATION}/public/include/core/Buffer.h"
    "${AMF_LOCATION}/public/include/core/Compute.h"
    "${AMF_LOCATION}/public/include/core/ComputeFactory.h"
    "${AMF_LOCATION}/public/include/core/Context.h"
    "${AMF_LOCATION}/public/include/core/CurrentTime.h"
    "${AMF_LOCATION}/public/include/core/Data.h"
    "${AMF_LOCATION}/public/include/core/Debug.h"
    "${AMF_LOCATION}/public/include/core/Dump.h"
    "${AMF_LOCATION}/public/include/core/Factory.h"
    "${AMF_LOCATION}/public/include/core/Interface.h"
    "${AMF_LOCATION}/public/include/core/Plane.h"
    "${AMF_LOCATION}/public/include/core/Platform.h"
    "${AMF_LOCATION}/public/include/core/PropertyStorage.h"
    "${AMF_LOCATION}/public/include/core/PropertyStorageEx.h"
    "${AMF_LOCATION}/public/include/core/Result.h"
    "${AMF_LOCATION}/public/include/core/Surface.h"
    "${AMF_LOCATION}/public/include/core/Trace.h"
    "${AMF_LOCATION}/public/include/core/Variant.h"
    "${AMF_LOCATION}/public/include/core/Version.h"
    "${AMF_LOCATION}/public/include/core/VulkanAMF.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/AMFHalfFloat.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenter.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/CAmfInit.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/CaptureVideoPipelineBase.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/CmdLineParser.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/CmdLogger.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/MiscHelpers.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/OpenCLLoader.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/ParametersStorage.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/Pipeline.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PipelineDefines.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PipelineElement.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PlaybackPipeline.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PlaybackPipelineBase.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PollingThread.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/PreProcessingParams.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/RawStreamReader.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/StitchPipeline.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/StitchPipelineBase.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SurfaceGenerator.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SurfaceUtils.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChain.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenter.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceVulkan.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainVulkan.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainOpenGL.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterVulkan.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterOpenGL.h"
)

if(WIN32)
set(HEADER_FILES ${ALL_PLATFORMS_HEADER_FILES}
    "${AMF_LOCATION}/public/common/Windows/UtilsWindows.h"
    "${AMF_LOCATION}/public/include/core/D3D12AMF.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/d3dx12.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX11.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX12.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceDX9.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX11.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX12.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDX9.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDXGI.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/SwapChainDXGIDecode.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX11.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX12.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/VideoPresenterDX9.h"
    "${AMF_LOCATION}/public/src/components/DisplayCapture/DDAPISource.h"
    "${AMF_LOCATION}/public/src/components/CursorCapture/CursorCaptureWin.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceOpenCL.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/DeviceOpenGL.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/Options.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenterWin.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadOpenGL.frag.h"
    "${AMF_LOCATION}/public/samples/CPPSamples/common/QuadOpenGL.vert.h"
    "${AMF_LOCATION}/public/src/components/DisplayCapture/DisplayCaptureImpl.h"
)
elseif(LINUX OR ANDROID)
    if (NOT ANDROID)
        set(HEADER_FILES ${ALL_PLATFORMS_HEADER_FILES}
        "${AMF_LOCATION}/public/src/components/CursorCapture/CursorCaptureLinux.h"
        "${AMF_LOCATION}/public/samples/CPPSamples/common/AudioPresenterLinux.h"
        "${AMF_LOCATION}/public/common/Linux/PulseAudioImportTable.h"
        "${AMF_LOCATION}/public/src/components/AudioCapture/PulseAudioSimpleAPISource.h"
        "${AMF_LOCATION}/public/src/components/AudioCapture/PulseAudioSimpleAPISourceFacade.h"
        )
    else()
        set(HEADER_FILES ${ALL_PLATFORMS_HEADER_FILES}
        )
    endif()
endif()

if(WIN32)
    # Function to compile HLSL shaders
    function(compile_hlsl_shader SHADER_NAME PROFILE SHADER_SOURCE SHADER_BINARY SHADER_HEADER)
        add_custom_command(
            OUTPUT ${SHADER_BINARY}
            COMMAND cd
            COMMAND ${WINDOWS_SDK_BIN}/fxc /nologo /T ${PROFILE} /E main /Vn ${SHADER_NAME} /Fo ${SHADER_BINARY} /Fh ${SHADER_HEADER} ${SHADER_SOURCE}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Running ${WINDOWS_SDK_BIN}/fxc /nologo /T ${PROFILE} /E main /Vn ${SHADER_NAME} /Fo ${SHADER_BINARY} /Fh ${SHADER_HEADER} ${SHADER_SOURCE}..."
        )
    endfunction()


    # Compile HLSL pixel shaders
    foreach(SHADER ${HLSL_PIXEL_SHADER_FILES})
        get_filename_component(SHADER_NAME_ABS ${SHADER} ABSOLUTE)
        get_filename_component(SHADER_NAME ${SHADER_NAME_ABS} NAME_WE)
        get_filename_component(SHADER_DIR ${SHADER_NAME_ABS} DIRECTORY)
        compile_hlsl_shader(${SHADER_NAME} "ps_4_0" ${SHADER_NAME_ABS} ${SHADER_DIR}/${SHADER_NAME}.cso ${SHADER_DIR}/${SHADER_NAME}.h)
        list(APPEND SHADER_BINARIES ${SHADER_NAME}.cso)
    endforeach()

    # Compile HLSL vertex shaders
    foreach(SHADER ${HLSL_VERTEX_SHADER_FILES})
        get_filename_component(SHADER_NAME_ABS ${SHADER} ABSOLUTE)
        get_filename_component(SHADER_NAME ${SHADER_NAME_ABS} NAME_WE)
        get_filename_component(SHADER_DIR ${SHADER_NAME_ABS} DIRECTORY)
        compile_hlsl_shader(${SHADER_NAME} "vs_4_0" ${SHADER_NAME_ABS} ${SHADER_DIR}/${SHADER_NAME}.cso ${SHADER_DIR}/${SHADER_NAME}.h)
        list(APPEND SHADER_BINARIES ${SHADER_NAME}.cso)
    endforeach()

    # Compile HLSL compute shaders
    foreach(SHADER ${HLSL_COMPUTE_SHADER_FILES})
        get_filename_component(SHADER_NAME_ABS ${SHADER} ABSOLUTE)
        get_filename_component(SHADER_NAME ${SHADER_NAME_ABS} NAME_WE)
        get_filename_component(SHADER_DIR ${SHADER_NAME_ABS} DIRECTORY)
        compile_hlsl_shader(${SHADER_NAME} "cs_5_0" ${SHADER_NAME_ABS} ${SHADER_DIR}/${SHADER_NAME}.cso ${SHADER_DIR}/${SHADER_NAME}_64.h)
        list(APPEND SHADER_BINARIES ${SHADER_NAME}.cso)
    endforeach()

    # Function to compile VLSL shaders
    function(compile_vlsl_shader SHADER_NAME SHADER_SOURCE SHADER_BINARY SHADER_HEADER)
        add_custom_command(
            OUTPUT ${SHADER_HEADER}
            COMMAND cd
            COMMAND ${VK_SDK_PATH}/Bin/glslangvalidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}
            COMMAND ${CMAKE_SOURCE_DIR}/amf/Thirdparty/file_to_header/file_to_header.exe ${SHADER_BINARY} ${SHADER_NAME}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Running ${VK_SDK_PATH}/Bin/glslangvalidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}, Project root: ${CMAKE_SOURCE_DIR}..."
        )
    endfunction()
elseif(UNIX)
    # Function to compile VLSL shaders
    function(compile_vlsl_shader SHADER_NAME SHADER_SOURCE SHADER_BINARY SHADER_HEADER)
        add_custom_command(
            OUTPUT ${SHADER_HEADER}
            COMMAND cd
            COMMAND ${VK_SDK_PATH}/bin/glslangValidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}
            COMMAND ${CMAKE_SOURCE_DIR}/amf/Thirdparty/file_to_header/Linux64/file_to_header ${SHADER_BINARY} ${SHADER_NAME}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Running ${VK_SDK_PATH}/bin/glslangValidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}, shader header: ${SHADER_HEADER} Project root: ${CMAKE_SOURCE_DIR}..."
        )
    endfunction()
endif()

# Compile VLSL shaders
foreach(SHADER ${VLSL_SHADER_FILES})
    get_filename_component(SHADER_NAME_ABS ${SHADER} ABSOLUTE)
    get_filename_component(SHADER_FILE ${SHADER_NAME_ABS} NAME)
    get_filename_component(SHADER_NAME ${SHADER_NAME_ABS} NAME_WE)
    get_filename_component(SHADER_DIR ${SHADER_NAME_ABS} DIRECTORY)
    compile_vlsl_shader(${SHADER_NAME} ${SHADER_NAME_ABS} ${SHADER_DIR}/${SHADER_FILE}.spv ${SHADER_DIR}/${SHADER_FILE}.spv.h)
    list(APPEND SHADER_BINARIES ${SHADER_DIR}/${SHADER_FILE}.spv.h)
endforeach()


# Create a custom target for compiling shaders
add_custom_target(CompileShaders ALL DEPENDS ${SHADER_BINARIES})

# Add the library
add_library(amf-public STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Add dependency
if (WIN32 OR LINUX)
add_dependencies(amf-public CompileShaders)
endif()

# Include directories
target_include_directories(amf-public PRIVATE
    ../../amf
    ${AMF_LOCATION}
)

# Compile definitions
target_compile_definitions(amf-public PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_LIB>
    $<$<CONFIG:Release>:NDEBUG;_LIB>
)

if(WIN32)
# Link options
target_link_options(amf-public PRIVATE
    $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
)
endif()

