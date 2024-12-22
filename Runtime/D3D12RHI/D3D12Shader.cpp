#include "D3D12Shader.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12RootSignature.h"
#include "Core/Misc/CRC.h"

static bool IsShaderResourceView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_TEXTURE || Type == D3D_SIT_BYTEADDRESS || Type == D3D_SIT_STRUCTURED || Type == D3D_SIT_RTACCELERATIONSTRUCTURE;
}

static bool IsUnorderedAccessView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_UAV_RWTYPED || Type == D3D_SIT_UAV_RWBYTEADDRESS || Type == D3D_SIT_UAV_RWSTRUCTURED;
}

static bool IsRayTracingLocalSpace(uint32 RegisterSpace)
{
    return RegisterSpace == D3D12_SHADER_REGISTER_SPACE_RT_LOCAL;
}

static bool IsLegalRegisterSpace(const D3D12_SHADER_INPUT_BIND_DESC& ShaderBindDesc)
{
    if (ShaderBindDesc.Space == D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS && ShaderBindDesc.Type == D3D_SIT_CBUFFER)
    {
        return true;
    }
    if (IsRayTracingLocalSpace(ShaderBindDesc.Space))
    {
        return true;
    }
    if (ShaderBindDesc.Space == 0)
    {
        return true;
    }

    return false;
}


FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility InShaderVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCode()
    , ShaderVisibility(InShaderVisibility)
{
    // Copy ByteCode
    ByteCode.BytecodeLength  = InCode.SizeInBytes();
    ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);
    FMemory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);

    // The beginning of the DXIL has the following layout
    //   * Bytes 0-3 are always set to the string "DXCB"
    //   * Bytes 4-19 are a checksum
    const uint8* CodeData = InCode.Data() + 4;
    ByteCodeHash = *reinterpret_cast<const FD3D12ShaderHash*>(CodeData);
}

FD3D12Shader::~FD3D12Shader()
{
    FMemory::Free(ByteCode.pShaderBytecode);
    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
}

template<typename TD3D12ReflectionInterface>
bool FD3D12Shader::GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, FD3D12Shader* Shader, uint32 NumBoundResources)
{
    FShaderResourceCount ResourceCount;
    FShaderResourceCount RTLocalResourceCount;

    D3D12_SHADER_INPUT_BIND_DESC ShaderBindDesc;
    for (uint32 Index = 0; Index < NumBoundResources; Index++)
    {
        FMemory::Memzero(&ShaderBindDesc);
        if (FAILED(Reflection->GetResourceBindingDesc(Index, &ShaderBindDesc)))
        {
            continue;
        }

        if (!IsLegalRegisterSpace(ShaderBindDesc))
        {
            D3D12_ERROR("Shader Parameter '%s' has register space '%u' specified, which is invalid.", ShaderBindDesc.Name, ShaderBindDesc.Space);
            return false;
        }

        if (ShaderBindDesc.Type == D3D_SIT_CBUFFER)
        {
            uint32 SizeInBytes = 0;

            ID3D12ShaderReflectionConstantBuffer* BufferVar = Reflection->GetConstantBufferByName(ShaderBindDesc.Name);
            if (BufferVar)
            {
                D3D12_SHADER_BUFFER_DESC BufferDesc;
                if (SUCCEEDED(BufferVar->GetDesc(&BufferDesc)))
                {
                    SizeInBytes = BufferDesc.Size;
                }
            }

            if (ShaderBindDesc.Space == D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
            {
                // NOTE: For now only one binding per shader can be used for constants
                const uint8 Num32BitConstants = static_cast<uint8>(SizeInBytes / 4);
                if (ShaderBindDesc.BindCount > 1 || Num32BitConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || ResourceCount.Num32BitConstants != 0)
                {
                    return false;
                }

                ResourceCount.Num32BitConstants = Num32BitConstants;
            }
            else
            {
                if (ShaderBindDesc.Space == 0)
                {
                    ResourceCount.Ranges.NumCBVs = FMath::Max<uint8>(ResourceCount.Ranges.NumCBVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
                }
                else
                {
                    RTLocalResourceCount.Ranges.NumCBVs = FMath::Max<uint8>(RTLocalResourceCount.Ranges.NumCBVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
                }
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumSamplers = FMath::Max<uint8>(ResourceCount.Ranges.NumSamplers, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                RTLocalResourceCount.Ranges.NumSamplers = FMath::Max<uint8>(RTLocalResourceCount.Ranges.NumSamplers, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumSRVs = FMath::Max<uint8>(ResourceCount.Ranges.NumSRVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                RTLocalResourceCount.Ranges.NumSRVs = FMath::Max<uint8>(RTLocalResourceCount.Ranges.NumSRVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumUAVs = FMath::Max<uint8>(ResourceCount.Ranges.NumUAVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                RTLocalResourceCount.Ranges.NumUAVs = FMath::Max<uint8>(RTLocalResourceCount.Ranges.NumUAVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
    }

    Shader->ResourceCount        = ResourceCount;
    Shader->RTLocalResourceCount = RTLocalResourceCount;
    return true;
}

bool FD3D12Shader::GetShaderReflection(FD3D12Shader* Shader)
{
    CHECK(Shader != nullptr);

    TComPtr<ID3D12ShaderReflection> Reflection;
    if (!GD3D12ShaderCompiler->GetReflection(Shader, &Reflection))
    {
        return false;
    }

    D3D12_SHADER_DESC ShaderDesc;
    if (FAILED(Reflection->GetDesc(&ShaderDesc)))
    {
        return false;
    }

    if (!GetShaderResourceBindings(Reflection.Get(), Shader, ShaderDesc.BoundResources))
    {
        D3D12_ERROR("[D3D12BaseShader]: Error when analysing shader parameters");
        return false;
    }

    if (GD3D12ShaderCompiler->HasRootSignature(Shader))
    {
        Shader->bContainsRootSignature = true;
    }

    return true;
}

bool FD3D12RayTracingShader::GetRayTracingShaderReflection(FD3D12RayTracingShader* Shader)
{
    CHECK(Shader != nullptr);

    TComPtr<ID3D12LibraryReflection> Reflection;
    if (!GD3D12ShaderCompiler->GetLibraryReflection(Shader, &Reflection))
    {
        return false;
    }

    D3D12_LIBRARY_DESC LibDesc;
    FMemory::Memzero(&LibDesc);

    HRESULT Result = Reflection->GetDesc(&LibDesc);
    if (FAILED(Result))
    {
        return false;
    }

    CHECK(LibDesc.FunctionCount > 0);

    // Make sure that the first shader is the one we wanted
    ID3D12FunctionReflection* Function = Reflection->GetFunctionByIndex(0);

    D3D12_FUNCTION_DESC FuncDesc;
    FMemory::Memzero(&FuncDesc);

    Function->GetDesc(&FuncDesc);
    if (FAILED(Result))
    {
        return false;
    }

    if (!GetShaderResourceBindings(Function, Shader, FuncDesc.BoundResources))
    {
        D3D12_ERROR("[FD3D12RayTracingShader]: Error when analysing shader parameters");
        return false;
    }

    // HACK: Since the Nvidia driver can't handle these names, we have to change the names :(
    const FString Identifier = FuncDesc.Name;

    auto NameStart = Identifier.FindLastCharWithPredicate([](CHAR Char) -> bool 
    { 
        return (Char == '\x1') || (Char == '?');
    });

    if (NameStart != FString::InvalidIndex)
    {
        NameStart++;
    }

    const int32 NameEnd = Identifier.Find("@");
    Shader->Identifier = Identifier.SubString(NameStart, NameEnd - NameStart);
    return true;
}


bool FD3D12ComputeShader::Initialize()
{
    TComPtr<ID3D12ShaderReflection> Reflection;
    if (!GD3D12ShaderCompiler->GetReflection(this, &Reflection))
    {
        return false;
    }

    D3D12_SHADER_DESC ShaderDesc;
    if (FAILED(Reflection->GetDesc(&ShaderDesc)))
    {
        return false;
    }

    if (!GetShaderResourceBindings(Reflection.Get(), this, ShaderDesc.BoundResources))
    {
        D3D12_ERROR("[D3D12BaseComputeShader]: Error when analysing shader parameters");
        return false;
    }

    if (GD3D12ShaderCompiler->HasRootSignature(this))
    {
        bContainsRootSignature = true;
    }

    return true;
}


void FShaderResourceCount::Combine(const FShaderResourceCount& Other)
{
    Ranges.NumCBVs     = FMath::Max(Ranges.NumCBVs, Other.Ranges.NumCBVs);
    Ranges.NumSRVs     = FMath::Max(Ranges.NumSRVs, Other.Ranges.NumSRVs);
    Ranges.NumUAVs     = FMath::Max(Ranges.NumUAVs, Other.Ranges.NumUAVs);
    Ranges.NumSamplers = FMath::Max(Ranges.NumSamplers, Other.Ranges.NumSamplers);
    Num32BitConstants  = FMath::Max(Num32BitConstants, Other.Num32BitConstants);
}

bool FShaderResourceCount::IsCompatible(const FShaderResourceCount& Other) const
{
    if (Num32BitConstants > Other.Num32BitConstants)
    {
        return false;
    }
    if (Ranges.NumCBVs > Other.Ranges.NumCBVs)
    {
        return false;
    }
    if (Ranges.NumSRVs > Other.Ranges.NumSRVs)
    {
        return false;
    }
    if (Ranges.NumUAVs > Other.Ranges.NumUAVs)
    {
        return false;
    }
    if (Ranges.NumSamplers > Other.Ranges.NumSamplers)
    {
        return false;
    }

    return true;
}
