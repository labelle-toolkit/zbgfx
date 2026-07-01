// SPIKE (labelle-bgfx#8): emscripten/WebGL astc-encoder stub.
//
// Zig's bundled wasm32-emscripten libc++ is built WITHOUT threads, so the ARM
// astc-encoder's ParallelManager (std::mutex / std::thread /
// std::condition_variable) does not compile. A WebGL runtime does not CPU
// transcode ASTC (the GPU consumes ASTC via the WEBGL_compressed_texture_astc
// extension), so on emscripten we drop the encoder sources and provide these
// minimal C-linkage stubs for the six entry points bimg's image.cpp /
// image_encode.cpp reference. Every call fails cleanly; ASTC software
// encode/decode is simply unavailable in the wasm build.
//
// Compiled as C++ with the SAME name-mangling as bimg's C++ TUs (on emscripten
// astcenc.h's ASTCENC_PUBLIC expands empty, i.e. C++ linkage, so no extern "C"
// wrapper here). No <mutex>/<thread> is pulled in.
#include <astcenc.h>

astcenc_error astcenc_config_init(
    astcenc_profile /*profile*/,
    unsigned int /*block_x*/,
    unsigned int /*block_y*/,
    unsigned int /*block_z*/,
    float /*quality*/,
    unsigned int /*flags*/,
    astcenc_config* /*config*/)
{
    return ASTCENC_ERR_NOT_IMPLEMENTED;
}

astcenc_error astcenc_context_alloc(
    const astcenc_config* /*config*/,
    unsigned int /*thread_count*/,
    astcenc_context** context)
{
    if (context) *context = nullptr;
    return ASTCENC_ERR_NOT_IMPLEMENTED;
}

astcenc_error astcenc_compress_image(
    astcenc_context* /*context*/,
    astcenc_image* /*image*/,
    const astcenc_swizzle* /*swizzle*/,
    uint8_t* /*data_out*/,
    size_t /*data_len*/,
    unsigned int /*thread_index*/)
{
    return ASTCENC_ERR_NOT_IMPLEMENTED;
}

astcenc_error astcenc_decompress_image(
    astcenc_context* /*context*/,
    const uint8_t* /*data*/,
    size_t /*data_len*/,
    astcenc_image* /*image_out*/,
    const astcenc_swizzle* /*swizzle*/,
    unsigned int /*thread_index*/)
{
    return ASTCENC_ERR_NOT_IMPLEMENTED;
}

void astcenc_context_free(astcenc_context* /*context*/)
{
}

const char* astcenc_get_error_string(astcenc_error /*status*/)
{
    return "ASTC unsupported in wasm build";
}
