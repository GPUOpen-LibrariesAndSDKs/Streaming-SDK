# Advanced Interactive Streaming SDK

Advanced Interactive Streaming SDK is a C++ library that provides building blocks and samples for low latency streaming applications, such as Cloud Game Streaming, Virtual Desktop Interface (VDI),
Virtual and Augmented Reality, etc. using AMD Radeon graphics. It allows you to build a complete streaming solution including video and audio capture, video and audio encoder/decoder/pre-post-processing pipelines, a robust and secure network stack, or use some of its components, while implementing the rest yourself.

## Changelog:
- v1.1.0 - added support for Linux:
	- Server supported on AMD graphics with the Pro driver (X.org required)
	- Client supported on AMD graphics with the Pro and RADV drivers, NVidia graphics (proprietary driver recommended), Intel graphics (X.org recommended, Wayland supported with some limitations)
	- Tested with Ubuntu 22.04 LTS and 24.04 LTS
- v1.0.2 - bug fixes:
	- Encoder frame rate wasn't being set correctly when display capture mode was set to *present* or when QoS was turned off
- v1.0.1 - bug fixes:
	- System mouse cursor doesn't get hidden correctly in some games
	- Deadlock on server exit when streaming at high (8K) resolution

## Dependencies:
Advanced Interactive Streaming SDK depends on a number of other components from AMD and third parties:
- [Advanced Media Framework (AMF SDK)](https://github.com/GPUOpen-LibrariesAndSDKs/AMF) for video encoding/decoding/processing
- [MbedTLS](https://github.com/Mbed-TLS/mbedtls) for encryption
- [Ffmpeg](https://github.com/FFmpeg/FFmpeg) for audio encoding/decoding/resampling

## License:
Advanced Interactive Streaming SDK is distributed in open source under the [MIT license](LICENSE.md)

## Documentation:
Comprehensive documentation is provided by the [Streaming SDK Wiki](https://github.com/GPUOpen-LibrariesAndSDKs/Streaming-SDK/wiki)

