#if PLATFORM_WINDOWS
#include "Core/Containers/ComPtr.h"
#include "Core/Templates/CString.h"
#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCompiler/Reflection/D3DReflectionUtils.h"
#include "ShaderCompiler/Reflection/DXBCShaderReflector.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"

bool FDXBCShaderReflector::Reflect(PFN_FXC_D3D_REFLECT D3DReflectFunc, const void* Code, SIZE_T CodeSize, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors)
{
    TComPtr<ID3D11ShaderReflection> Reflection;
    if (!D3DReflectFunc || FAILED(D3DReflectFunc(Code, CodeSize, IID_PPV_ARGS(&Reflection))))
    {
        OutErrors += "Failed to reflect the DXBC shader\n";
        return false;
    }

    D3D11_SHADER_DESC ShaderDesc = {};
    if (FAILED(Reflection->GetDesc(&ShaderDesc)))
    {
        OutErrors += "Failed to retrieve the shader description\n";
        return false;
    }

    for (uint32 Index = 0; Index < ShaderDesc.BoundResources; ++Index)
    {
        D3D11_SHADER_INPUT_BIND_DESC BindDesc = {};
        if (FAILED(Reflection->GetResourceBindingDesc(Index, &BindDesc)))
        {
            continue;
        }

        // Counter buffers are plain RWStructuredBuffer UAVs to D3D11RHI
        EShaderResourceType Type;
        bool bHasCounter = false;
        if (!FD3DReflectionUtils::TranslateInputType(BindDesc.Type, BindDesc.Dimension, Type, bHasCounter))
        {
            OutErrors += String::Printf("Unhandled shader resource type '%u' for parameter '%s' at register %u\n", BindDesc.Type, BindDesc.Name, BindDesc.BindPoint);
            return false;
        }

        const EShaderResourceDimension Dimension = FD3DReflectionUtils::TranslateDimension(BindDesc.Dimension);

        if (Type == EShaderResourceType::ConstantBuffer && CString::Strcmp(BindDesc.Name, ShaderBindings::ShaderConstantsBufferName) == 0)
        {
            uint32 SizeInBytes = 0;
            if (ID3D11ShaderReflectionConstantBuffer* ConstantBuffer = Reflection->GetConstantBufferByName(BindDesc.Name))
            {
                D3D11_SHADER_BUFFER_DESC BufferDesc = {};
                if (SUCCEEDED(ConstantBuffer->GetDesc(&BufferDesc)))
                {
                    SizeInBytes = BufferDesc.Size;
                }
            }

            if (!FShaderReflectionUtils::SetShaderConstantsSize(BindDesc.Name, SizeInBytes, OutReflection, OutErrors))
            {
                return false;
            }

            if (!FShaderReflectionUtils::AddBinding(BindDesc.Name, Type, Dimension, EShaderBindingSpace::ShaderConstants, BindDesc.BindPoint, BindDesc.BindCount, CompileInfo.bDebugInfo, OutReflection, OutErrors))
            {
                return false;
            }

            continue;
        }

        if (!FShaderReflectionUtils::AddBinding(BindDesc.Name, Type, Dimension, EShaderBindingSpace::Global, BindDesc.BindPoint, BindDesc.BindCount, CompileInfo.bDebugInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }

    OutReflection.EntryPoint.Clear();

    if (CompileInfo.ShaderStage == EShaderStage::Vertex)
    {
        return FD3DReflectionUtils::ReflectVertexInputs<ID3D11ShaderReflection, D3D11_SIGNATURE_PARAMETER_DESC>(Reflection.Get(), ShaderDesc.InputParameters, OutReflection, OutErrors);
    }

    return true;
}

#endif
