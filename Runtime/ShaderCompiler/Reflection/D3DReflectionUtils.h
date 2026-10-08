#pragma once
#if PLATFORM_WINDOWS
#include <d3dcommon.h>

#include "ShaderCore/ShaderReflection.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"

struct FD3DReflectionUtils
{
    /** @brief Append/Consume and counter buffers come back as RWStructuredBuffer with bOutHasCounter set */
    static bool TranslateInputType(D3D_SHADER_INPUT_TYPE Type, D3D_SRV_DIMENSION Dimension, EShaderResourceType& OutType, bool& bOutHasCounter);
    static EShaderResourceDimension TranslateDimension(D3D_SRV_DIMENSION Dimension);
    static EShaderFeatureFlags TranslateRequiresFlags(uint64 Mask);
    static EShaderComponentType TranslateComponentType(D3D_REGISTER_COMPONENT_TYPE ComponentType, D3D_MIN_PRECISION MinPrecision);

    /** @brief Skips system values, sorts by Location and rejects two inputs with the same semantic hash and index */
    template<typename ReflectionType, typename ParameterDescType>
    static bool ReflectVertexInputs(ReflectionType* Reflection, uint32 NumInputParameters, FShaderReflection& OutReflection, String& OutErrors);
};

template<typename ReflectionType, typename ParameterDescType>
bool FD3DReflectionUtils::ReflectVertexInputs(ReflectionType* Reflection, uint32 NumInputParameters, FShaderReflection& OutReflection, String& OutErrors)
{
    for (uint32 Index = 0; Index < NumInputParameters; ++Index)
    {
        ParameterDescType ParameterDesc = {};
        if (FAILED(Reflection->GetInputParameterDesc(Index, &ParameterDesc)) || ParameterDesc.SystemValueType != D3D_NAME_UNDEFINED)
        {
            continue;
        }

        if (ParameterDesc.Register > 0xFF || ParameterDesc.SemanticIndex > 0xFF)
        {
            OutErrors += String::Printf("Vertex input '%s%u' at register %u is outside the supported range\n", ParameterDesc.SemanticName, ParameterDesc.SemanticIndex, ParameterDesc.Register);
            return false;
        }

        uint8 NumComponents = 0;
        for (uint32 Bit = 0; Bit < 4; ++Bit)
        {
            NumComponents += (ParameterDesc.Mask >> Bit) & 1;
        }

        FShaderVertexInput& Input = OutReflection.VertexInputs.Emplace();
        Input.SemanticHash  = HashShaderSemantic(ParameterDesc.SemanticName);
        Input.SemanticIndex = static_cast<uint8>(ParameterDesc.SemanticIndex);
        Input.Location      = static_cast<uint8>(ParameterDesc.Register);
        Input.ComponentType = TranslateComponentType(ParameterDesc.ComponentType, ParameterDesc.MinPrecision);
        Input.NumComponents = NumComponents;
    }

    return FShaderReflectionUtils::SortAndValidateVertexInputs(OutReflection.VertexInputs, OutErrors);
}

#endif
