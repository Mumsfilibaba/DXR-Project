#include "Core/Misc/CRC.h"
#include "Core/Threading/Atomic.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12Loader.h"
#include "RHI/ShaderCompiler.h"

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


#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) (unsigned int)((unsigned char)(a) | ((unsigned char)(b) << 8) | ((unsigned char)(c) << 16) | ((unsigned char)(d) << 24))
#endif

enum DxilFourCC
{
	DFCC_Container               = MAKEFOURCC('D', 'X', 'B', 'C'),
	DFCC_ResourceDef             = MAKEFOURCC('R', 'D', 'E', 'F'),
	DFCC_InputSignature          = MAKEFOURCC('I', 'S', 'G', '1'),
	DFCC_OutputSignature         = MAKEFOURCC('O', 'S', 'G', '1'),
	DFCC_PatchConstantSignature  = MAKEFOURCC('P', 'S', 'G', '1'),
	DFCC_ShaderStatistics        = MAKEFOURCC('S', 'T', 'A', 'T'),
	DFCC_ShaderDebugInfoDXIL     = MAKEFOURCC('I', 'L', 'D', 'B'),
	DFCC_ShaderDebugName         = MAKEFOURCC('I', 'L', 'D', 'N'),
	DFCC_FeatureInfo             = MAKEFOURCC('S', 'F', 'I', '0'),
	DFCC_PrivateData             = MAKEFOURCC('P', 'R', 'I', 'V'),
	DFCC_RootSignature           = MAKEFOURCC('R', 'T', 'S', '0'),
	DFCC_DXIL                    = MAKEFOURCC('D', 'X', 'I', 'L'),
	DFCC_PipelineStateValidation = MAKEFOURCC('P', 'S', 'V', '0'),
	DFCC_RuntimeData             = MAKEFOURCC('R', 'D', 'A', 'T'),
	DFCC_ShaderHash              = MAKEFOURCC('H', 'A', 'S', 'H'),
};

#undef MAKEFOURCC

class FExistingBlob : public IDxcBlob
{
public:
	FExistingBlob(LPVOID InData, SIZE_T InSizeInBytes)
		: SizeInBytes(InSizeInBytes)
		, Data(nullptr)
		, References(1)
	{
		Data = FMemory::Malloc(SizeInBytes);
		FMemory::Memcpy(Data, InData, SizeInBytes);
	}

	~FExistingBlob()
	{
		FMemory::Free(Data);
	}

	virtual LPVOID GetBufferPointer() override final { return Data; }
	virtual SIZE_T GetBufferSize() override final { return SizeInBytes; }

	virtual HRESULT QueryInterface(REFIID Riid, LPVOID* ppvObject) override final
	{
		if (!ppvObject)
		{
			return E_INVALIDARG;
		}

		*ppvObject = nullptr;

		if (Riid == __uuidof(IUnknown) || Riid == __uuidof(ID3DBlob) || Riid == __uuidof(IDxcBlob))
		{
			*ppvObject = reinterpret_cast<LPVOID>(this);
			AddRef();
			return NOERROR;
		}

		return E_NOINTERFACE;
	}

	virtual ULONG AddRef() override final
	{
		const uint32 NewRefCount = ++References;
		return static_cast<ULONG>(NewRefCount);
	}

	virtual ULONG Release() override final
	{
		const uint32 NewRefCount = --References;
		if (NewRefCount == 0)
		{
			delete this;
		}

		return static_cast<ULONG>(NewRefCount);
	}

private:
	SIZE_T SizeInBytes;
	LPVOID Data;
	FAtomicUInt32 References;
};

FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCode()
    , ByteCodeHash()
    , ShaderVisibility(InShaderVisibility)
    , ResourceCount()
    , LocalRayTracingResourceCount()
    , bContainsRootSignature(false)
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
	//   - Bytes 0-3 are always set to the string "DXBC"
	//   - Bytes 4-19 are a 16-byte checksum
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

bool FD3D12Shader::IsRootSignatureInShaderBlob(const TComPtr<IDxcBlob>& ShaderBlob)
{
    TComPtr<IDxcContainerReflection> Reflection;
    HRESULT Result = D3D12Functions::DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&Reflection));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Shader]: FAILED to create IDxcContainerReflection");
        return false;
    }

    Result = Reflection->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Shader]: Reflection were not able to load shader");
        return false;
    }

    uint32 PartIndex;
    Result = Reflection->FindFirstPartKind(DFCC_RootSignature, &PartIndex);
    if (FAILED(Result))
    {
        return false;
    }

    return true;
}

bool FD3D12Shader::GetReflectionInterface(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject)
{
    TComPtr<IDxcContainerReflection> ReflectionInterface;
    HRESULT Result = D3D12Functions::DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&ReflectionInterface));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Shader]: FAILED to create ReflectionInterface");
        return false;
    }

    Result = ReflectionInterface->Load(ShaderBlob.Get());
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Shader]: FAILED to get reflection of shader");
        return false;
    }

    uint32 PartIndex;
    Result = ReflectionInterface->FindFirstPartKind(DFCC_DXIL, &PartIndex);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Shader]: Shader does not contain valid DXIL part");
        return false;
    }

    Result = ReflectionInterface->GetPartReflection(PartIndex, iid, ppvObject);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Shader]: FAILED to get DXIL object");
        return false;
    }

    return true;
}

template<typename TD3D12ReflectionInterface>
bool FD3D12Shader::GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, uint32 NumBoundResources)
{
    FShaderResourceCount NewResourceCount;
    FShaderResourceCount NewLocalRayTracingResourceCount;

    for (uint32 i = 0; i < NumBoundResources; i++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderBindDesc = {};
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
                const uint8 NumShaderConstants = static_cast<uint8>(SizeInBytes) / static_cast<uint8>(sizeof(uint32));
                if (ShaderBindDesc.BindCount > 1 || NumShaderConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || NewResourceCount.NumShaderConstants != 0)
                {
                    return false;
                }

                NewResourceCount.NumShaderConstants = NumShaderConstants;
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

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.pShaderBytecode, static_cast<uint64>(ByteCode.BytecodeLength));

	TComPtr<ID3D12ShaderReflection> Reflection;
	if (!GetReflectionInterface(ShaderBlob, IID_PPV_ARGS(&Reflection)))
	{
		return false;
	}

	D3D12_SHADER_DESC ShaderDesc = {};
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

	if (IsRootSignatureInShaderBlob(ShaderBlob))
	{
		bContainsRootSignature = true;
	}

	return true;
}

bool FD3D12ComputeShaderRHI::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D12Shader::Initialize(InCode))
    {
        return false;
    }

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.pShaderBytecode, static_cast<uint64>(ByteCode.BytecodeLength));

    TComPtr<ID3D12ShaderReflection> Reflection;
	if (!GetReflectionInterface(ShaderBlob, IID_PPV_ARGS(&Reflection)))
    {
        return false;
    }

    D3D12_SHADER_DESC ShaderDesc = {};
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

    if (IsRootSignatureInShaderBlob(ShaderBlob))
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

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.pShaderBytecode, static_cast<uint64>(ByteCode.BytecodeLength));

	TComPtr<ID3D12LibraryReflection> Reflection;
	if (!GetReflectionInterface(ShaderBlob, IID_PPV_ARGS(&Reflection)))
	{
		return false;
	}

	D3D12_LIBRARY_DESC LibraryDesc = {};
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

	D3D12_FUNCTION_DESC FunctionDesc = {};
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
    NumShaderConstants = Math::Max(NumShaderConstants, Other.NumShaderConstants);
}

bool FShaderResourceCount::IsCompatible(const FShaderResourceCount& Other) const
{
    if (NumShaderConstants > Other.NumShaderConstants)
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
