#include "D3D12Shader.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12RootSignature.h"

D3D12BaseShader::D3D12BaseShader(D3D12Device* InDevice, const TArray<UInt8>& InCode, EShaderVisibility InVisibility)
    : D3D12DeviceChild(InDevice)
    , ByteCode()
    , Visibility(InVisibility)
{
    ByteCode.BytecodeLength  = InCode.SizeInBytes();
    ByteCode.pShaderBytecode = Memory::Malloc(ByteCode.BytecodeLength);
    
    Memory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);
}

D3D12BaseShader::~D3D12BaseShader()
{
    Memory::Free((void*)ByteCode.pShaderBytecode);
    
    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
}

static Bool IsShaderResourceView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_TEXTURE || Type == D3D_SIT_BYTEADDRESS || Type == D3D_SIT_STRUCTURED || Type == D3D_SIT_RTACCELERATIONSTRUCTURE;
}

static Bool IsUnorderedAccessView(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_UAV_RWTYPED || Type == D3D_SIT_UAV_RWBYTEADDRESS || Type == D3D_SIT_UAV_RWSTRUCTURED;
}

static Bool IsRayTracingLocalSpace(UInt32 RegisterSpace)
{
    return RegisterSpace == D3D12_SHADER_REGISTER_SPACE_RT_LOCAL;
}

static Bool IsLegalRegisterSpace(const D3D12_SHADER_INPUT_BIND_DESC& ShaderBindDesc)
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

template<typename TD3D12ReflectionInterface>
Bool D3D12BaseShader::GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, D3D12BaseShader* Shader, UInt32 NumBoundResources)
{
    ShaderResourceCount          ResourceCount;
    TArray<D3D12ShaderParameter> ConstantBufferParameters;
    TArray<D3D12ShaderParameter> SamplerParameters;
    TArray<D3D12ShaderParameter> ShaderResourceParameters;
    TArray<D3D12ShaderParameter> UnorderedAccessParameters;

    D3D12_SHADER_INPUT_BIND_DESC ShaderBindDesc;
    for (UInt32 i = 0; i < NumBoundResources; i++)
    {
        Memory::Memzero(&ShaderBindDesc);

        if (FAILED(Reflection->GetResourceBindingDesc(i, &ShaderBindDesc)))
        {
            continue;
        }

        if (!IsLegalRegisterSpace(ShaderBindDesc))
        {
            LOG_ERROR("Shader Parameter '" + std::string(ShaderBindDesc.Name) + "' has register space '" + std::to_string(ShaderBindDesc.Space) + "' specified, which is invalid.");
            return false;
        }

        if (ShaderBindDesc.Type == D3D_SIT_CBUFFER)
        {
            UInt32 SizeInBytes = 0;

            ID3D12ShaderReflectionConstantBuffer* BufferVar = Reflection->GetConstantBufferByName(ShaderBindDesc.Name);
            if (BufferVar)
            {
                D3D12_SHADER_BUFFER_DESC BufferDesc;
                if (SUCCEEDED(BufferVar->GetDesc(&BufferDesc)))
                {
                    SizeInBytes = BufferDesc.Size;
                }
            }

            UInt32 Num32BitConstants = SizeInBytes / 4;
            ConstantBufferParameters.EmplaceBack(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, SizeInBytes);
            
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
                
                ResourceCount.Ranges.ConstantBufferViewCount = Math::Max(ResourceCount.Ranges.ConstantBufferViewCount, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            SamplerParameters.EmplaceBack(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, 0);
            ResourceCount.Ranges.SamplerCount = Math::Max(ResourceCount.Ranges.SamplerCount, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            const UInt32 NumDescriptors = ShaderBindDesc.BindCount != 0 ? ShaderBindDesc.BindCount : UINT_MAX;

            ShaderResourceParameters.EmplaceBack(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, NumDescriptors, 0);
            ResourceCount.Ranges.ShaderResourceViewCount = Math::Max(ResourceCount.Ranges.ShaderResourceViewCount, ShaderBindDesc.BindPoint + NumDescriptors);
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            UnorderedAccessParameters.EmplaceBack(ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space, ShaderBindDesc.BindCount, 0);
            ResourceCount.Ranges.UnorderedAccessViewCount = Math::Max(ResourceCount.Ranges.UnorderedAccessViewCount, ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount);
        }
    }

    Shader->ConstantBufferParameters  = Move(ConstantBufferParameters);
    Shader->SamplerParameters         = Move(SamplerParameters);
    Shader->ShaderResourceParameters  = Move(ShaderResourceParameters);
    Shader->UnorderedAccessParameters = Move(UnorderedAccessParameters);
    Shader->ResourceCount             = ResourceCount;

    return true;
}

Bool D3D12BaseShader::GetShaderReflection(D3D12BaseShader* Shader)
{
    Assert(Shader != nullptr);

    TComPtr<ID3D12ShaderReflection> Reflection;
    if (!gD3D12ShaderCompiler->GetReflection(Shader, &Reflection))
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
        LOG_ERROR("[D3D12BaseShader]: Error when analysing shader parameters");
        return false;
    }

    if (gD3D12ShaderCompiler->HasRootSignature(Shader))
    {
        Shader->ContainsRootSignature = true;
    }

    return true;
}

Bool D3D12BaseRayTracingShader::GetRayTracingShaderReflection(D3D12BaseRayTracingShader* Shader)
{
    Assert(Shader != nullptr);

    TComPtr<ID3D12LibraryReflection> Reflection;
    if (!gD3D12ShaderCompiler->GetLibraryReflection(Shader, &Reflection))
    {
        return false;
    }

    D3D12_LIBRARY_DESC LibDesc;
    Memory::Memzero(&LibDesc);

    HRESULT Result = Reflection->GetDesc(&LibDesc);
    if (FAILED(Result))
    {
        return false;
    }

    Assert(LibDesc.FunctionCount > 0);

    // Make sure that the first shader is the one we wanted
    ID3D12FunctionReflection* Function = Reflection->GetFunctionByIndex(0);

    D3D12_FUNCTION_DESC FuncDesc;
    Memory::Memzero(&FuncDesc);

    Function->GetDesc(&FuncDesc);
    if (FAILED(Result))
    {
        return false;
    }

    if (!GetShaderResourceBindings(Function, Shader, FuncDesc.BoundResources))
    {
        LOG_ERROR("[D3D12BaseRayTracingShader]: Error when analysing shader parameters");
        return false;
    }

    // NOTE: Since the Nvidia driver can't handle these names, we have to change the names :(
    std::string Identifier = FuncDesc.Name;

    auto NameStart = Identifier.find_last_of("\x1?");
    if (NameStart != std::string::npos)
    {
        NameStart++;
    }

    auto NameEnd = Identifier.find_first_of("@");

    Shader->Identifier = Identifier.substr(NameStart, NameEnd - NameStart);
    return true;
}

Bool D3D12BaseComputeShader::Init()
{
    TComPtr<ID3D12ShaderReflection> Reflection;
    if (!gD3D12ShaderCompiler->GetReflection(this, &Reflection))
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
        LOG_ERROR("[D3D12BaseComputeShader]: Error when analysing shader parameters");
        return false;
    }

    if (gD3D12ShaderCompiler->HasRootSignature(this))
    {
        ContainsRootSignature = true;
    }

    Reflection->GetThreadGroupSize(&ThreadGroupXYZ.x, &ThreadGroupXYZ.y, &ThreadGroupXYZ.z);

    return true;
}

Bool ShaderResourceCount::IsCompatible(const ShaderResourceCount& Other) const
{
    if (Num32BitConstants > Other.Num32BitConstants)
    {
        return false;
    }
    if (Ranges.ConstantBufferViewCount > Other.Ranges.ConstantBufferViewCount)
    {
        return false;
    }
    if (Ranges.ShaderResourceViewCount > Other.Ranges.ShaderResourceViewCount)
    {
        return false;
    }
    if (Ranges.UnorderedAccessViewCount > Other.Ranges.UnorderedAccessViewCount)
    {
        return false;
    }
    if (Ranges.SamplerCount > Other.Ranges.SamplerCount)
    {
        return false;
    }

    return true;
}
