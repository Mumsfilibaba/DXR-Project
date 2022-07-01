#include "D3D12Shader.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12RootSignature.h"

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Shader

FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility InVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCode()
    , Visibility(InVisibility)
{
    ByteCode.BytecodeLength = InCode.SizeInBytes();
    ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);

    FMemory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);
}

FD3D12Shader::~FD3D12Shader()
{
    FMemory::Free((void*)ByteCode.pShaderBytecode);

    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength = 0;
}

template<typename TD3D12ReflectionInterface>
bool FD3D12Shader::GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, FD3D12Shader* Shader, uint32 NumBoundResources)
{
    FShaderResourceCount          ResourceCount;
    FShaderResourceCount          RTLocalResourceCount;
    TArray<FD3D12ShaderParameter> ConstantBufferParameters;
    TArray<FD3D12ShaderParameter> SamplerParameters;
    TArray<FD3D12ShaderParameter> ShaderResourceParameters;
    TArray<FD3D12ShaderParameter> UnorderedAccessParameters;

    D3D12_SHADER_INPUT_BIND_DESC ShaderBindDesc;
    for (uint32 i = 0; i < NumBoundResources; i++)
    {
        FMemory::Memzero(&ShaderBindDesc);

        if (FAILED(Reflection->GetResourceBindingDesc(i, &ShaderBindDesc)))
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

            uint32 Num32BitConstants = SizeInBytes / 4;

            if (ShaderBindDesc.Space == D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
            {
                // NOTE: For now only one binding per shader can be used for constants
                if (ShaderBindDesc.BindCount > 1 || Num32BitConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || ResourceCount.Num32BitConstants != 0)
                {
                    return false;
                }

                ResourceCount.Num32BitConstants = Num32BitConstants;
            }
            else
            {
                ConstantBufferParameters.Emplace(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, SizeInBytes);
                if (ShaderBindDesc.Space == 0)
                {
                    ResourceCount.Ranges.NumCBVs = NMath::Max(ResourceCount.Ranges.NumCBVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
                }
                else
                {
                    RTLocalResourceCount.Ranges.NumCBVs = NMath::Max(RTLocalResourceCount.Ranges.NumCBVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
                }
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            SamplerParameters.Emplace(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, 0);

            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumSamplers = NMath::Max(ResourceCount.Ranges.NumSamplers, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
            else
            {
                RTLocalResourceCount.Ranges.NumSamplers = NMath::Max(RTLocalResourceCount.Ranges.NumSamplers, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            const uint32 NumDescriptors = ShaderBindDesc.BindCount != 0 ? ShaderBindDesc.BindCount : UINT_MAX;

            ShaderResourceParameters.Emplace(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, NumDescriptors, 0);

            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumSRVs = NMath::Max(ResourceCount.Ranges.NumSRVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
            else
            {
                RTLocalResourceCount.Ranges.NumSRVs = NMath::Max(RTLocalResourceCount.Ranges.NumSRVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            UnorderedAccessParameters.Emplace(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, 0);

            if (ShaderBindDesc.Space == 0)
            {
                ResourceCount.Ranges.NumUAVs = NMath::Max(ResourceCount.Ranges.NumUAVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
            else
            {
                RTLocalResourceCount.Ranges.NumUAVs = NMath::Max(RTLocalResourceCount.Ranges.NumUAVs, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
        }
    }

    Shader->ConstantBufferParameters  = Move(ConstantBufferParameters);
    Shader->SamplerParameters         = Move(SamplerParameters);
    Shader->ShaderResourceParameters  = Move(ShaderResourceParameters);
    Shader->UnorderedAccessParameters = Move(UnorderedAccessParameters);
    Shader->ResourceCount             = ResourceCount;
    Shader->RTLocalResourceCount      = RTLocalResourceCount;

    return true;
}

bool FD3D12Shader::GetShaderReflection(FD3D12Shader* Shader)
{
    Check(Shader != nullptr);

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingShader

bool FD3D12RayTracingShader::GetRayTracingShaderReflection(FD3D12RayTracingShader* Shader)
{
    Check(Shader != nullptr);

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

    Check(LibDesc.FunctionCount > 0);

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
    FString Identifier = FuncDesc.Name;

    auto NameStart = Identifier.ReverseFindOneOf("\x1?");
    if (NameStart != FString::NPos)
    {
        NameStart++;
    }

    const auto NameEnd = Identifier.Find("@");

    Shader->Identifier = Identifier.SubString(NameStart, NameEnd - NameStart);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ComputeShader

bool FD3D12ComputeShader::Init()
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

    UINT x;
    UINT y;
    UINT z;
    Reflection->GetThreadGroupSize(&x, &y, &z);

    ThreadGroupXYZ.x = x;
    ThreadGroupXYZ.y = y;
    ThreadGroupXYZ.z = z;
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ShaderResourceCount

void FShaderResourceCount::Combine(const FShaderResourceCount& Other)
{
    Ranges.NumCBVs     = NMath::Max(Ranges.NumCBVs, Other.Ranges.NumCBVs);
    Ranges.NumSRVs     = NMath::Max(Ranges.NumSRVs, Other.Ranges.NumSRVs);
    Ranges.NumUAVs     = NMath::Max(Ranges.NumUAVs, Other.Ranges.NumUAVs);
    Ranges.NumSamplers = NMath::Max(Ranges.NumSamplers, Other.Ranges.NumSamplers);
    Num32BitConstants  = NMath::Max(Num32BitConstants, Other.Num32BitConstants);
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
