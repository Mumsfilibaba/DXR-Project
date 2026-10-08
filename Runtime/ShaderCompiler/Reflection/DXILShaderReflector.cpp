#if PLATFORM_WINDOWS
#include "Core/Containers/ComPtr.h"
#include "Core/Math/Math.h"
#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCompiler/Reflection/D3DReflectionUtils.h"
#include "ShaderCompiler/Reflection/DXILShaderReflector.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"

static constexpr uint32 GDxilPartFeatureInfo   = DXC_FOURCC('S', 'F', 'I', '0');
static constexpr uint32 GDxilPartRootSignature = DXC_FOURCC('R', 'T', 'S', '0');

static bool IsLegalRegisterSpace(const D3D12_SHADER_INPUT_BIND_DESC& BindDesc)
{
    if (BindDesc.Space == ShaderBindings::ShaderConstantsSpace && BindDesc.Type == D3D_SIT_CBUFFER)
    {
        return true;
    }

    return BindDesc.Space == ShaderBindings::RayTracingLocalSpace || BindDesc.Space == ShaderBindings::GlobalSpace;
}

static DxcBuffer MakeDxcBuffer(IDxcBlob* Blob)
{
    DxcBuffer Buffer;
    Buffer.Ptr      = Blob->GetBufferPointer();
    Buffer.Size     = Blob->GetBufferSize();
    Buffer.Encoding = 0;
    return Buffer;
}

bool FDXILShaderReflector::Reflect(IDxcUtils* Utils, IDxcBlob* ShaderObject, IDxcBlob* ReflectionBlob, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors)
{
    const DxcBuffer ReflectionBuffer = MakeDxcBuffer(ReflectionBlob);

    if (IsShaderStageRayTracing(CompileInfo.ShaderStage))
    {
        TComPtr<ID3D12LibraryReflection> LibraryReflection;
        if (FAILED(Utils->CreateReflection(&ReflectionBuffer, IID_PPV_ARGS(&LibraryReflection))) || !LibraryReflection)
        {
            OutErrors += "Failed to create the library reflection of the shader\n";
            return false;
        }

        if (!ReflectLibrary(LibraryReflection.Get(), Utils, ShaderObject, CompileInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }
    else
    {
        TComPtr<ID3D12ShaderReflection> ShaderReflection;
        if (FAILED(Utils->CreateReflection(&ReflectionBuffer, IID_PPV_ARGS(&ShaderReflection))) || !ShaderReflection)
        {
            OutErrors += "Failed to create the reflection of the shader\n";
            return false;
        }

        if (!ReflectShader(ShaderReflection.Get(), CompileInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }

    if (HasPart(Utils, ShaderObject, GDxilPartRootSignature))
    {
        OutReflection.Info.Flags |= EShaderReflectionFlags::HasEmbeddedRootSignature;
    }

    return true;
}

bool FDXILShaderReflector::ReflectShader(ID3D12ShaderReflection* Reflection, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors)
{
    D3D12_SHADER_DESC ShaderDesc = {};
    if (FAILED(Reflection->GetDesc(&ShaderDesc)))
    {
        OutErrors += "Failed to retrieve the shader description\n";
        return false;
    }

    for (uint32 Index = 0; Index < ShaderDesc.BoundResources; ++Index)
    {
        D3D12_SHADER_INPUT_BIND_DESC BindDesc = {};
        if (FAILED(Reflection->GetResourceBindingDesc(Index, &BindDesc)))
        {
            continue;
        }

        if (!ReflectBinding(Reflection, BindDesc, false, CompileInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }

    OutReflection.Info.RequiredFeatures = FD3DReflectionUtils::TranslateRequiresFlags(static_cast<uint64>(Reflection->GetRequiresFlags()));

    if (CompileInfo.ShaderStage == EShaderStage::Vertex)
    {
        return FD3DReflectionUtils::ReflectVertexInputs<ID3D12ShaderReflection, D3D12_SIGNATURE_PARAMETER_DESC>(Reflection, ShaderDesc.InputParameters, OutReflection, OutErrors);
    }

    return true;
}

bool FDXILShaderReflector::ReflectLibrary(ID3D12LibraryReflection* Reflection, IDxcUtils* Utils, IDxcBlob* ShaderObject, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors)
{
    D3D12_LIBRARY_DESC LibraryDesc = {};
    if (FAILED(Reflection->GetDesc(&LibraryDesc)))
    {
        OutErrors += "Failed to retrieve the library description\n";
        return false;
    }

    ID3D12FunctionReflection* Function = nullptr;
    D3D12_FUNCTION_DESC FunctionDesc = {};
    for (uint32 Index = 0; Index < LibraryDesc.FunctionCount && !Function; ++Index)
    {
        ID3D12FunctionReflection* Candidate = Reflection->GetFunctionByIndex(static_cast<INT>(Index));

        D3D12_FUNCTION_DESC CandidateDesc = {};
        if (Candidate && SUCCEEDED(Candidate->GetDesc(&CandidateDesc)) && CandidateDesc.Name && DemangleFunctionName(CandidateDesc.Name) == CompileInfo.EntryPoint)
        {
            Function     = Candidate;
            FunctionDesc = CandidateDesc;
        }
    }

    if (!Function)
    {
        OutErrors += String::Printf("The shader library has no function named '%s'\n", *CompileInfo.EntryPoint);
        return false;
    }

    for (uint32 Index = 0; Index < FunctionDesc.BoundResources; ++Index)
    {
        D3D12_SHADER_INPUT_BIND_DESC BindDesc = {};
        if (FAILED(Function->GetResourceBindingDesc(Index, &BindDesc)))
        {
            continue;
        }

        if (!ReflectBinding(Function, BindDesc, true, CompileInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }

    uint64 FeatureFlags = 0;
    if (ReadFeatureInfo(Utils, ShaderObject, FeatureFlags))
    {
        OutReflection.Info.RequiredFeatures = FD3DReflectionUtils::TranslateRequiresFlags(FeatureFlags);
    }

    OutReflection.EntryPoint = CompileInfo.EntryPoint;
    return true;
}

template<typename ReflectionType>
bool FDXILShaderReflector::ReflectBinding(ReflectionType* Reflection, const D3D12_SHADER_INPUT_BIND_DESC& BindDesc, bool bIsLibrary, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, String& OutErrors)
{
    if (!IsLegalRegisterSpace(BindDesc))
    {
        OutErrors += String::Printf("Shader Parameter '%s' has register space '%u' specified, which is invalid\n", BindDesc.Name, BindDesc.Space);
        return false;
    }

    const bool bIsLocalSpace = BindDesc.Space == ShaderBindings::RayTracingLocalSpace;

    // Only ray-tracing libraries have local root signatures
    if (bIsLocalSpace && !bIsLibrary)
    {
        return true;
    }

    EShaderResourceType Type;
    bool bHasCounter = false;
    if (!FD3DReflectionUtils::TranslateInputType(BindDesc.Type, BindDesc.Dimension, Type, bHasCounter) || bHasCounter || BindDesc.Type == D3D_SIT_TBUFFER)
    {
        OutErrors += String::Printf("Unhandled shader resource type '%u' for parameter '%s' at register %u, space %u. This binding would be missing from the root signature\n",
            BindDesc.Type, BindDesc.Name, BindDesc.BindPoint, BindDesc.Space);
        return false;
    }

    if (Type == EShaderResourceType::ConstantBuffer && BindDesc.Space == ShaderBindings::ShaderConstantsSpace)
    {
        if (BindDesc.BindCount > 1)
        {
            OutErrors += String::Printf("Shader Parameter '%s' is an array of %u 32-bit constant buffers, only a single constant buffer is supported in register space %u\n",
                BindDesc.Name, BindDesc.BindCount, ShaderBindings::ShaderConstantsSpace);
            return false;
        }

        uint32 SizeInBytes = 0;
        if (ID3D12ShaderReflectionConstantBuffer* ConstantBuffer = Reflection->GetConstantBufferByName(BindDesc.Name))
        {
            D3D12_SHADER_BUFFER_DESC BufferDesc = {};
            if (SUCCEEDED(ConstantBuffer->GetDesc(&BufferDesc)))
            {
                SizeInBytes = BufferDesc.Size;
            }
        }

        return FShaderReflectionUtils::SetShaderConstantsSize(BindDesc.Name, SizeInBytes, OutReflection, OutErrors);
    }

    const bool bIsTextureUAV = Type == EShaderResourceType::RWTexture;
    if (bIsLocalSpace && bIsTextureUAV)
    {
        OutErrors += String::Printf("Shader Parameter '%s': Texture UAVs are not supported in RT local root signatures (space %u). Only buffer UAVs are allowed\n", BindDesc.Name, BindDesc.Space);
        return false;
    }

    const EShaderBindingSpace Space = bIsLocalSpace ? EShaderBindingSpace::RayTracingLocal : EShaderBindingSpace::Global;
    return FShaderReflectionUtils::AddBinding(BindDesc.Name, Type, FD3DReflectionUtils::TranslateDimension(BindDesc.Dimension), Space, BindDesc.BindPoint, BindDesc.BindCount, CompileInfo.bDebugInfo, OutReflection, OutErrors);
}

bool FDXILShaderReflector::HasPart(IDxcUtils* Utils, IDxcBlob* ShaderObject, uint32 FourCC)
{
    const DxcBuffer ObjectBuffer = MakeDxcBuffer(ShaderObject);

    void*  PartData = nullptr;
    uint32 PartSize = 0;
    return SUCCEEDED(Utils->GetDxilContainerPart(&ObjectBuffer, FourCC, &PartData, &PartSize)) && PartData != nullptr;
}

bool FDXILShaderReflector::ReadFeatureInfo(IDxcUtils* Utils, IDxcBlob* ShaderObject, uint64& OutFlags)
{
    OutFlags = 0;

    const DxcBuffer ObjectBuffer = MakeDxcBuffer(ShaderObject);

    void*  PartData = nullptr;
    uint32 PartSize = 0;
    if (FAILED(Utils->GetDxilContainerPart(&ObjectBuffer, GDxilPartFeatureInfo, &PartData, &PartSize)) || !PartData || PartSize < sizeof(uint64))
    {
        return false;
    }

    Memory::Memcpy(&OutFlags, PartData, sizeof(uint64));
    return true;
}

// DXC mangles the names of the functions in a shader-library ('\x1?MyRayGen@@YAXXZ'),
// while D3D12 expects the unmangled name to be used as export-name.
String FDXILShaderReflector::DemangleFunctionName(const String& MangledName)
{
    constexpr const CHAR* ManglingPrefix = "\x1?";
    constexpr int32       PrefixLength   = 2;

    if (!MangledName.StartsWith(ManglingPrefix))
    {
        return MangledName;
    }

    const int32 NameEnd = MangledName.Find("@", PrefixLength);
    if (NameEnd <= PrefixLength)
    {
        return MangledName;
    }

    return MangledName.SubString(PrefixLength, NameEnd - PrefixLength);
}

#endif
