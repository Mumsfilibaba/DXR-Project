#pragma once
#include "ShaderCompiler/ShaderPreprocessor.h"

struct SHADERCOMPILER_API FFXCShaderTranslator
{
    /** Changes whenever the translated source for the same input changes, so compiled DXBC can be invalidated */
    static constexpr uint32 Version = 2;

    /**
     * @brief Rewrites the constructs that FXC cannot parse, so the same shader source works with both compilers
     *
     * - ConstantBuffer<T> Name : register(bN) becomes cbuffer Name_CB : register(bN) { T Name; }, which has the same layout.
     *   Shader Model 5.0 has no register spaces, so the shader constants in space1 get no register at all. FXC places them
     *   in the first free slot, and D3D11RHI finds the slot by reflecting the cbuffer named Constants_CB.
     * - [[attribute]] becomes [attribute].
     * - Hexadecimal float literals become decimal literals with the same value.
     *
     * @return Returns false and adds to InOutSource.Errors when something cannot be expressed in Shader Model 5.0
     */
    static bool Translate(FShaderPreprocessorOutput& InOutSource);
};
