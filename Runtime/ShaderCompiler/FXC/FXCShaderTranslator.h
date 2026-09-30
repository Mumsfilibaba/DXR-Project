#pragma once
#include "ShaderCompiler/ShaderPreprocessor.h"

/** The constant-buffer slot that D3D11RHI binds the shader constants to. Must match D3D11_SHADER_CONSTANTS_REGISTER. */
#define FXC_SHADER_CONSTANTS_REGISTER "b13"

struct SHADERCOMPILER_API FFXCShaderTranslator
{
    /**
     * @brief Rewrites the constructs that FXC cannot parse, so the same shader source works with both compilers
     *
     * - ConstantBuffer<T> Name : register(bN) becomes cbuffer Name_CB : register(bN) { T Name; }, which has the same layout.
     *   The shader constants in space1 move to FXC_SHADER_CONSTANTS_REGISTER, since Shader Model 5.0 has no register spaces.
     * - [[attribute]] becomes [attribute].
     * - Hexadecimal float literals become decimal literals with the same value.
     *
     * @return Returns false and adds to InOutSource.Errors when something cannot be expressed in Shader Model 5.0
     */
    static bool Translate(FShaderPreprocessorOutput& InOutSource);
};
