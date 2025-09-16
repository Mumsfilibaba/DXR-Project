#ifndef CORE_DEFINES_HLSLI
#define CORE_DEFINES_HLSLI
#include "Constants.hlsli"

#if SHADER_LANG == SHADER_LANG_SPIRV
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

#if SHADER_LANG == SHADER_LANG_SPIRV
    #define TEXTURE_FORMAT_UNKNOWN [[vk::image_format("unknown")]] 
#else
    #define TEXTURE_FORMAT_UNKNOWN
#endif

#endif