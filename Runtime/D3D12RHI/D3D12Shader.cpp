#include "Core/Misc/CRC.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RHIShaderCompiler.h"
#include "D3D12RHI/D3D12RootSignature.h"

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

FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCode()
    , ShaderVisibility(InShaderVisibility)
{
}

FD3D12Shader::~FD3D12Shader()
{
    FMemory::Free(ByteCode.pShaderBytecode);

    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
}

bool FD3D12Shader::Initialize(const TArray<uint8>& InCode)
{
	// Allocate byte-code
	ByteCode.BytecodeLength  = InCode.SizeInBytes();
	ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);

	// Copy byte-code
	FMemory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);

	// The beginning of the DXIL container has the following layout
	//   - Bytes 0–3 are always set to the string "DXBC"
	//   - Bytes 4–19 are a 16-byte checksum
	if (ByteCode.BytecodeLength >= 20)
	{
		const uint8* CodeData = InCode.Data() + 4;
		ByteCodeHash = *reinterpret_cast<const FD3D12ShaderHash*>(CodeData);
        return true;
	}
	else
	{
		ByteCodeHash = FD3D12ShaderHash();
        return false;
	}
}

template<typename TD3D12ReflectionInterface>
bool FD3D12Shader::GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, uint32 NumBoundResources)
{
    FShaderResourceCount NewResourceCount;
    FShaderResourceCount NewLocalRayTracingResourceCount;

    for (uint32 i = 0; i < NumBoundResources; i++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderBindDesc;
        FMemory::Memzero(&ShaderBindDesc);
        
        if (FAILED(Reflection->GetResourceBindingDesc(i, &ShaderBindDesc)))
        {
            continue;
        }

        if (!IsLegalRegisterSpace(ShaderBindDesc))
        {
            D3D12_ERROR_CRITICAL("Shader Parameter '%s' has register space '%u' specified, which is invalid.", ShaderBindDesc.Name, ShaderBindDesc.Space);
            return false;
        }

        if (ShaderBindDesc.Type == D3D_SIT_CBUFFER)
        {
            uint32 SizeInBytes = 0;
            if (ID3D12ShaderReflectionConstantBuffer* BufferVar = Reflection->GetConstantBufferByName(ShaderBindDesc.Name))
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
                const uint8 Num32BitConstants = static_cast<uint8>(SizeInBytes) / static_cast<uint8>(sizeof(uint32));
                if (ShaderBindDesc.BindCount > 1 || Num32BitConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || NewResourceCount.Num32BitConstants != 0)
                {
                    return false;
                }

                NewResourceCount.Num32BitConstants = Num32BitConstants;
            }
            else
            {
                if (ShaderBindDesc.Space == 0)
                {
                    NewResourceCount.Ranges.NumCBVs = Math::Max<uint8>(NewResourceCount.Ranges.NumCBVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
                }
                else
                {
                    NewLocalRayTracingResourceCount.Ranges.NumCBVs = Math::Max<uint8>(NewLocalRayTracingResourceCount.Ranges.NumCBVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
                }
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            if (ShaderBindDesc.Space == 0)
            {
                NewResourceCount.Ranges.NumSamplers = Math::Max<uint8>(NewResourceCount.Ranges.NumSamplers, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                NewLocalRayTracingResourceCount.Ranges.NumSamplers = Math::Max<uint8>(NewLocalRayTracingResourceCount.Ranges.NumSamplers, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            if (ShaderBindDesc.Space == 0)
            {
                NewResourceCount.Ranges.NumSRVs = Math::Max<uint8>(NewResourceCount.Ranges.NumSRVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                NewLocalRayTracingResourceCount.Ranges.NumSRVs = Math::Max<uint8>(NewLocalRayTracingResourceCount.Ranges.NumSRVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            if (ShaderBindDesc.Space == 0)
            {
                NewResourceCount.Ranges.NumUAVs = Math::Max<uint8>(NewResourceCount.Ranges.NumUAVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
            else
            {
                NewLocalRayTracingResourceCount.Ranges.NumUAVs = Math::Max<uint8>(NewLocalRayTracingResourceCount.Ranges.NumUAVs, uint8(ShaderBindDesc.BindPoint + ShaderBindDesc.BindCount));
            }
        }
    }

    ResourceCount = NewResourceCount;
    LocalRayTracingResourceCount = NewLocalRayTracingResourceCount;
    return true;
}

bool FD3D12GraphicsShader::Initialize(const TArray<uint8>& InCode)
{
	if (!FD3D12Shader::Initialize(InCode))
	{
		return false;
	}

	TComPtr<ID3D12ShaderReflection> Reflection;
	if (!GD3D12ShaderCompiler->GetReflection(this, &Reflection))
	{
		return false;
	}

	D3D12_SHADER_DESC ShaderDesc;
	FMemory::Memzero(&ShaderDesc);

	HRESULT Result = Reflection->GetDesc(&ShaderDesc);
	if (FAILED(Result))
	{
		return false;
	}

	if (!GetShaderResourceBindings(Reflection.Get(), ShaderDesc.BoundResources))
	{
		D3D12_ERROR_CRITICAL("[D3D12BaseShader]: Error when analysing shader parameters");
		return false;
	}

	if (GD3D12ShaderCompiler->HasRootSignature(this))
	{
		bContainsRootSignature = true;
	}

	return true;
}

bool FD3D12ComputeShader::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D12Shader::Initialize(InCode))
    {
        return false;
    }

    TComPtr<ID3D12ShaderReflection> Reflection;
    if (!GD3D12ShaderCompiler->GetReflection(this, &Reflection))
    {
        return false;
    }

    D3D12_SHADER_DESC ShaderDesc;
	FMemory::Memzero(&ShaderDesc);

    HRESULT Result = Reflection->GetDesc(&ShaderDesc);
    if (FAILED(Result))
    {
        return false;
    }

    if (!GetShaderResourceBindings(Reflection.Get(), ShaderDesc.BoundResources))
    {
        D3D12_ERROR_CRITICAL("[D3D12BaseComputeShader]: Error when analysing shader parameters");
        return false;
    }

    if (GD3D12ShaderCompiler->HasRootSignature(this))
    {
        bContainsRootSignature = true;
    }

    return true;
}

bool FD3D12RayTracingShader::Initialize(const TArray<uint8>& InCode)
{
	if (!FD3D12Shader::Initialize(InCode))
	{
		return false;
	}

	TComPtr<ID3D12LibraryReflection> Reflection;
	if (!GD3D12ShaderCompiler->GetLibraryReflection(this, &Reflection))
	{
		return false;
	}

	D3D12_LIBRARY_DESC LibraryDesc;
	FMemory::Memzero(&LibraryDesc);

	HRESULT Result = Reflection->GetDesc(&LibraryDesc);
	if (FAILED(Result))
	{
		return false;
	}

	if (LibraryDesc.FunctionCount > 0)
	{
        D3D12_ERROR("[FD3D12RayTracingShader]: No functions in shader-library");
		return false;
	}

	// Make sure that the first shader is the one we wanted
	ID3D12FunctionReflection* Function = Reflection->GetFunctionByIndex(0);

	D3D12_FUNCTION_DESC FunctionDesc;
	FMemory::Memzero(&FunctionDesc);

	Function->GetDesc(&FunctionDesc);
	if (FAILED(Result))
	{
		return false;
	}

	if (!GetShaderResourceBindings(Function, FunctionDesc.BoundResources))
	{
		D3D12_ERROR_CRITICAL("[FD3D12RayTracingShader]: Error when analysing shader parameters");
		return false;
	}

	// HACK: Since the NVIDIA driver can't handle these names, we have to change the names :(
	const FString FuncIdentifier = FunctionDesc.Name;

	int32 NameStart = FuncIdentifier.FindLastCharWithPredicate([](CHAR Char) -> bool
	{
		return (Char == '\x1') || (Char == '?');
	});

	if (NameStart != FString::InvalidIndex)
	{
		NameStart++;
	}

	const int32 NameEnd = FuncIdentifier.Find("@");
	Identifier = FuncIdentifier.SubString(NameStart, NameEnd - NameStart);
	return true;
}

void FShaderResourceCount::Combine(const FShaderResourceCount& Other)
{
    Ranges.NumCBVs     = Math::Max(Ranges.NumCBVs, Other.Ranges.NumCBVs);
    Ranges.NumSRVs     = Math::Max(Ranges.NumSRVs, Other.Ranges.NumSRVs);
    Ranges.NumUAVs     = Math::Max(Ranges.NumUAVs, Other.Ranges.NumUAVs);
    Ranges.NumSamplers = Math::Max(Ranges.NumSamplers, Other.Ranges.NumSamplers);
    Num32BitConstants  = Math::Max(Num32BitConstants, Other.Num32BitConstants);
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
