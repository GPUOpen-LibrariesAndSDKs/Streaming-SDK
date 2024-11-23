cmake_minimum_required(VERSION 3.15)

# Define the project name
project(amf-public)

# Define source files
set(HLSL_PIXEL_SHADER_FILES
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX11_ps.hlsl"
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX12_ps.hlsl"
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX9_ps.hlsl"
)

set(HLSL_VERTEX_SHADER_FILES
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX11_vs.hlsl"
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX12_vs.hlsl"
    "../../amf/amf/public/samples/CPPSamples/common/QuadDX9_vs.hlsl"
)

set(HLSL_COMPUTE_SHADER_FILES
    "../../amf/amf/public/src/components/DisplayCapture/DrawRectsBGRA.hlsl"
)

set(VLSL_SHADER_FILES
    "../../amf/amf/public/samples/CPPSamples/common/QuadVulkan_fs.frag"
    "../../amf/amf/public/samples/CPPSamples/common/QuadVulkan_vs.vert"
)

set(SOURCE_FILES
    "../../amf/amf/public/common/AMFFactory.cpp"
    "../../amf/amf/public/common/AMFSTL.cpp"
    "../../amf/amf/public/common/CurrentTimeImpl.cpp"
    "../../amf/amf/public/common/JsonImpl.cpp"
    "../../amf/amf/public/common/PropertyStorageExImpl.cpp"
    "../../amf/amf/public/common/Thread.cpp"
    "../../amf/amf/public/common/TraceAdapter.cpp"
    "../../amf/amf/public/common/VulkanImportTable.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenter.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenterLinux.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/BitStreamParser.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/BitStreamParserH264.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/BitStreamParserH265.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/BitStreamParserIVF.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/CaptureVideoPipelineBase.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/CmdLineParser.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/CmdLogger.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/MiscHelpers.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/OpenCLLoader.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/ParametersStorage.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/Pipeline.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/PlaybackPipeline.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/PlaybackPipelineBase.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/PreProcessingParams.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SurfaceUtils.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChain.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenter.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceVulkan.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainVulkan.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterVulkan.cpp"
    "../../amf/amf/public/src/components/AudioCapture/AudioCaptureImpl.cpp"
)

# Add platform-specific files:
if(WIN32)
set(SOURCE_FILES ${SOURCE_FILES}
    "../../amf/amf/public/common/Windows/ThreadWindows.cpp"
    "../../amf/amf/public/common/Windows/UtilsWindows.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX11.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX12.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX9.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX11.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX12.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX9.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDXGI.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDXGIDecode.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainOpenGL.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX11.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX12.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX9.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterOpenGL.cpp"
    "../../amf/amf/public/src/components/DisplayCapture/DDAPISource.cpp"
    "../../amf/amf/public/src/components/AudioCapture/WASAPISource.cpp"
    "../../amf/amf/public/src/components/CursorCapture/CursorCaptureWin.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceOpenCL.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceOpenGL.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/Options.cpp"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenterWin.cpp"
    "../../amf/amf/public/common/OpenGLImportTable.cpp"
    "../../amf/amf/public/src/components/DisplayCapture/DisplayCaptureImpl.cpp"
)
else()
    set(SOURCE_FILES ${SOURCE_FILES}
    "../../amf/amf/public/common/Linux/ThreadLinux.cpp"
    "../../amf/amf/public/common/Linux/PulseAudioImportTable.cpp"
    "../../amf/amf/public/src/components/CursorCapture/CursorCaptureLinux.cpp"
    "../../amf/amf/public/src/components/AudioCapture/PulseAudioSimpleAPISource.cpp"
    "../../amf/amf/public/src/components/AudioCapture/PulseAudioSimpleAPISourceFacade.cpp"
    )
endif()

# Define header files
set(HEADER_FILES
    "../../amf/amf/public/common/AMFFactory.h"
    "../../amf/amf/public/common/AMFMath.h"
    "../../amf/amf/public/common/AMFSTL.h"
    "../../amf/amf/public/common/ByteArray.h"
    "../../amf/amf/public/common/CPUCaps.h"
    "../../amf/amf/public/common/CurrentTimeImpl.h"
    "../../amf/amf/public/common/InterfaceImpl.h"
    "../../amf/amf/public/common/Json.h"
    "../../amf/amf/public/common/JsonImpl.h"
    "../../amf/amf/public/common/ObservableImpl.h"
    "../../amf/amf/public/common/PropertyStorageExImpl.h"
    "../../amf/amf/public/common/PropertyStorageImpl.h"
    "../../amf/amf/public/common/Thread.h"
    "../../amf/amf/public/common/TraceAdapter.h"
    "../../amf/amf/public/common/VulkanImportTable.h"
    "../../amf/amf/public/include/core/AudioBuffer.h"
    "../../amf/amf/public/include/core/Buffer.h"
    "../../amf/amf/public/include/core/Compute.h"
    "../../amf/amf/public/include/core/ComputeFactory.h"
    "../../amf/amf/public/include/core/Context.h"
    "../../amf/amf/public/include/core/CurrentTime.h"
    "../../amf/amf/public/include/core/Data.h"
    "../../amf/amf/public/include/core/Debug.h"
    "../../amf/amf/public/include/core/Dump.h"
    "../../amf/amf/public/include/core/Factory.h"
    "../../amf/amf/public/include/core/Interface.h"
    "../../amf/amf/public/include/core/Plane.h"
    "../../amf/amf/public/include/core/Platform.h"
    "../../amf/amf/public/include/core/PropertyStorage.h"
    "../../amf/amf/public/include/core/PropertyStorageEx.h"
    "../../amf/amf/public/include/core/Result.h"
    "../../amf/amf/public/include/core/Surface.h"
    "../../amf/amf/public/include/core/Trace.h"
    "../../amf/amf/public/include/core/Variant.h"
    "../../amf/amf/public/include/core/Version.h"
    "../../amf/amf/public/include/core/VulkanAMF.h"
    "../../amf/amf/public/samples/CPPSamples/common/AMFHalfFloat.h"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenter.h"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenterLinux.h"
    "../../amf/amf/public/samples/CPPSamples/common/CAmfInit.h"
    "../../amf/amf/public/samples/CPPSamples/common/CaptureVideoPipelineBase.h"
    "../../amf/amf/public/samples/CPPSamples/common/CmdLineParser.h"
    "../../amf/amf/public/samples/CPPSamples/common/CmdLogger.h"
    "../../amf/amf/public/samples/CPPSamples/common/MiscHelpers.h"
    "../../amf/amf/public/samples/CPPSamples/common/OpenCLLoader.h"
    "../../amf/amf/public/samples/CPPSamples/common/ParametersStorage.h"
    "../../amf/amf/public/samples/CPPSamples/common/Pipeline.h"
    "../../amf/amf/public/samples/CPPSamples/common/PipelineDefines.h"
    "../../amf/amf/public/samples/CPPSamples/common/PipelineElement.h"
    "../../amf/amf/public/samples/CPPSamples/common/PlaybackPipeline.h"
    "../../amf/amf/public/samples/CPPSamples/common/PlaybackPipelineBase.h"
    "../../amf/amf/public/samples/CPPSamples/common/PollingThread.h"
    "../../amf/amf/public/samples/CPPSamples/common/PreProcessingParams.h"
    "../../amf/amf/public/samples/CPPSamples/common/RawStreamReader.h"
    "../../amf/amf/public/samples/CPPSamples/common/StitchPipeline.h"
    "../../amf/amf/public/samples/CPPSamples/common/StitchPipelineBase.h"
    "../../amf/amf/public/samples/CPPSamples/common/SurfaceGenerator.h"
    "../../amf/amf/public/samples/CPPSamples/common/SurfaceUtils.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChain.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenter.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceVulkan.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainVulkan.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterVulkan.h"
)

if(WIN32)
set(HEADER_FILES ${HEADER_FILES}
    "../../amf/amf/public/common/Windows/UtilsWindows.h"
    "../../amf/amf/public/include/core/D3D12AMF.h"
    "../../amf/amf/public/samples/CPPSamples/common/d3dx12.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX11.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX12.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceDX9.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX11.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX12.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDX9.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDXGI.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainDXGIDecode.h"
    "../../amf/amf/public/samples/CPPSamples/common/SwapChainOpenGL.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX11.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX12.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterDX9.h"
    "../../amf/amf/public/samples/CPPSamples/common/VideoPresenterOpenGL.h"
    "../../amf/amf/public/src/components/DisplayCapture/DDAPISource.h"
    "../../amf/amf/public/src/components/CursorCapture/CursorCaptureWin.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceOpenCL.h"
    "../../amf/amf/public/samples/CPPSamples/common/DeviceOpenGL.h"
    "../../amf/amf/public/samples/CPPSamples/common/Options.h"
    "../../amf/amf/public/samples/CPPSamples/common/AudioPresenterWin.h"
    "../../amf/amf/public/common/OpenGLImportTable.h"
    "../../amf/amf/public/samples/CPPSamples/common/QuadOpenGL.frag.h"
    "../../amf/amf/public/samples/CPPSamples/common/QuadOpenGL.vert.h"
    "../../amf/amf/public/src/components/DisplayCapture/DisplayCaptureImpl.h"
)
else()
    set(HEADER_FILES ${HEADER_FILES}
    "../../amf/amf/public/src/components/CursorCapture/CursorCaptureLinux.h"
    "../../amf/amf/public/common/Linux/PulseAudioImportTable.h"
    "../../amf/amf/public/src/components/AudioCapture/PulseAudioSimpleAPISource.h"
    "../../amf/amf/public/src/components/AudioCapture/PulseAudioSimpleAPISourceFacade.h"
    )
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
            OUTPUT ${SHADER_BINARY}
            COMMAND cd
            COMMAND ${VK_SDK_PATH}/Bin/glslangvalidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}
            COMMAND ${CMAKE_SOURCE_DIR}/amf/Thirdparty/file_to_header/file_to_header.exe ${SHADER_BINARY} ${SHADER_NAME}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Running ${VK_SDK_PATH}/Bin/glslangvalidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}, Project root: ${CMAKE_SOURCE_DIR}..."
        )
    endfunction()
else()
    # Function to compile VLSL shaders
    function(compile_vlsl_shader SHADER_NAME SHADER_SOURCE SHADER_BINARY SHADER_HEADER)
        add_custom_command(
            OUTPUT ${SHADER_BINARY}
            COMMAND cd
            COMMAND glslangValidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}
            COMMAND ${CMAKE_SOURCE_DIR}/amf/Thirdparty/file_to_header/Linux64/file_to_header ${SHADER_BINARY} ${SHADER_NAME}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Running glslangValidator -V ${SHADER_SOURCE} -o ${SHADER_BINARY}, shader header: ${SHADER_HEADER} Project root: ${CMAKE_SOURCE_DIR}..."
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
    list(APPEND SHADER_BINARIES ${SHADER_DIR}/${SHADER_FILE}.spv ${SHADER_DIR}/${SHADER_FILE}.spv.h)
endforeach()


# Create a custom target for compiling shaders
add_custom_target(CompileShaders ALL DEPENDS ${SHADER_BINARIES})

# Add the library
add_library(amf-public STATIC ${SOURCE_FILES} ${HEADER_FILES})
if(WIN32)
add_dependencies(amf-public CompileShaders)
endif()

# Include directories
target_include_directories(amf-public PRIVATE
    ../../amf
    ../../amf/amf
)

# Compile definitions
target_compile_definitions(amf-public PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_LIB>
    $<$<CONFIG:Release>:NDEBUG;_LIB>
)

# Link options
target_link_options(amf-public PRIVATE
    $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
)

