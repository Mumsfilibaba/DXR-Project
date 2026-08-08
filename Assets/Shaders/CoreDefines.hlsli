#ifndef CORE_DEFINES_HLSLI
#define CORE_DEFINES_HLSLI

#include "Constants.hlsli"

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS (0)
#endif

// Mirrors EVertexAttributeFlags.
#define VERTEX_ATTRIBUTE_POSITION      (1 << 0)
#define VERTEX_ATTRIBUTE_TANGENT_BASIS (1 << 1)
#define VERTEX_ATTRIBUTE_TEXCOORD0     (1 << 2)
#define VERTEX_ATTRIBUTE_COLOR         (1 << 3)

#ifndef VERTEX_ATTRIBUTES
    #define VERTEX_ATTRIBUTES (VERTEX_ATTRIBUTE_POSITION)
#endif

#define HAS_VERTEX_TANGENT_BASIS ((VERTEX_ATTRIBUTES & VERTEX_ATTRIBUTE_TANGENT_BASIS) != 0)
#define HAS_VERTEX_TEXCOORD0     ((VERTEX_ATTRIBUTES & VERTEX_ATTRIBUTE_TEXCOORD0) != 0)
#define HAS_VERTEX_COLOR         ((VERTEX_ATTRIBUTES & VERTEX_ATTRIBUTE_COLOR) != 0)

#if SHADER_BACKEND == SHADER_BACKEND_VULKAN
    #define SHADER_CONSTANT_BLOCK_BEGIN \
        [[vk::push_constant]]        \
        struct FShaderBlockConstants \
        {

    #define SHADER_CONSTANT_BLOCK_END \
        } Constants;
#else
    #define SHADER_CONSTANT_BLOCK_BEGIN \
        struct FShaderBlockConstants \
        {

    #define SHADER_CONSTANT_BLOCK_END \
        }; \
        ConstantBuffer<FShaderBlockConstants> Constants : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
#endif

#if SHADER_BACKEND == SHADER_BACKEND_VULKAN
    #define TEXTURE_FORMAT_UNKNOWN [[vk::image_format("unknown")]] 
#else
    #define TEXTURE_FORMAT_UNKNOWN
#endif

#endif