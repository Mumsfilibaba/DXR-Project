#include "Core/Misc/CRC.h"
#include "Core/RefCountedBase.h"
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

static bool IsBufferSRV(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_BYTEADDRESS || Type == D3D_SIT_STRUCTURED || Type == D3D_SIT_RTACCELERATIONSTRUCTURE;
}

static bool IsBufferUAV(D3D_SHADER_INPUT_TYPE Type)
{
    return Type == D3D_SIT_UAV_RWBYTEADDRESS || Type == D3D_SIT_UAV_RWSTRUCTURED;
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

class FExistingBlob : public IDxcBlob, public FRefCountedBase
{
public:
	FExistingBlob(LPVOID InData, SIZE_T InSizeInBytes)
		: SizeInBytes(InSizeInBytes)
		, Data(nullptr)
	{
		Data = FMemory::Malloc(SizeInBytes);
		FMemory::Memcpy(Data, InData, SizeInBytes);
	}

	~FExistingBlob()
	{
		FMemory::Free(Data);
	}

    virtual ULONG AddRef()  override final { return static_cast<ULONG>(FRefCountedBase::AddRef()); }
    virtual ULONG Release() override final { return static_cast<ULONG>(FRefCountedBase::Release()); }

	virtual SIZE_T GetBufferSize()    override final { return SizeInBytes; }
	virtual LPVOID GetBufferPointer() override final { return Data; }

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

private:
	SIZE_T SizeInBytes;
	LPVOID Data;
};

FD3D12ShaderBytecode::FD3D12ShaderBytecode()
    : ByteCode()
{
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode(const TArray<uint8>& InCode)
    : ByteCode()
{
    ByteCode.BytecodeLength  = InCode.SizeInBytes();
    ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);
    FMemory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode(const FD3D12ShaderBytecode& Other)
    : ByteCode()
{
    if (Other.ByteCode.pShaderBytecode)
    {
        ByteCode.BytecodeLength  = Other.ByteCode.BytecodeLength;
        ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);
        FMemory::Memcpy((void*)ByteCode.pShaderBytecode, Other.ByteCode.pShaderBytecode, ByteCode.BytecodeLength);
    }
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode(FD3D12ShaderBytecode&& Other)
    : ByteCode(Other.ByteCode)
{
    Other.ByteCode.pShaderBytecode = nullptr;
    Other.ByteCode.BytecodeLength  = 0;
}

FD3D12ShaderBytecode& FD3D12ShaderBytecode::operator=(const FD3D12ShaderBytecode& Other)
{
    if (this != &Other)
    {
        FMemory::Free(ByteCode.pShaderBytecode);
        ByteCode = {};

        if (Other.ByteCode.pShaderBytecode)
        {
            ByteCode.BytecodeLength  = Other.ByteCode.BytecodeLength;
            ByteCode.pShaderBytecode = FMemory::Malloc(ByteCode.BytecodeLength);

            FMemory::Memcpy((void*)ByteCode.pShaderBytecode, Other.ByteCode.pShaderBytecode, ByteCode.BytecodeLength);
        }
    }

    return *this;
}

FD3D12ShaderBytecode& FD3D12ShaderBytecode::operator=(FD3D12ShaderBytecode&& Other)
{
    if (this != &Other)
    {
        FMemory::Free(ByteCode.pShaderBytecode);

        ByteCode = Other.ByteCode;

        Other.ByteCode.pShaderBytecode = nullptr;
        Other.ByteCode.BytecodeLength  = 0;
    }

    return *this;
}

FD3D12ShaderBytecode::~FD3D12ShaderBytecode()
{
    FMemory::Free(ByteCode.pShaderBytecode);

    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
}

FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCodeHash()
    , ShaderVisibility(InShaderVisibility)
    , BindingInfo()
    , bContainsRootSignature(false)
{
}

FD3D12Shader::~FD3D12Shader()
{
}

bool FD3D12Shader::Initialize(const TArray<uint8>& InCode)
{
	ByteCode = FD3D12ShaderBytecode(InCode);

	// The beginning of the DXIL container has the following layout
	//   - Bytes 0-3 are always set to the string "DXBC"
	//   - Bytes 4-19 are a 16-byte checksum
	if (ByteCode.GetCodeSize() >= 20)
	{
		const uint8* CodeData = static_cast<const uint8*>(ByteCode.GetCode()) + 4;
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


bool FD3D12Shader::GetShaderResourceBindings(ID3D12ShaderReflection* Reflection, uint32 NumBoundResources)
{
    FD3D12ShaderBindingInfo NewBindingInfo;

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

        if (IsRayTracingLocalSpace(ShaderBindDesc.Space))
        {
            continue;
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
                const uint8 NumShaderConstants = static_cast<uint8>(SizeInBytes) / static_cast<uint8>(sizeof(uint32));
                if (ShaderBindDesc.BindCount > 1 || NumShaderConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || NewBindingInfo.NumPushConstants != 0)
                {
                    return false;
                }

                NewBindingInfo.NumPushConstants = NumShaderConstants;
            }
            else
            {
                NewBindingInfo.AddBinding(D3D12BindingType_ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            NewBindingInfo.AddBinding(D3D12BindingType_Sampler, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(D3D12BindingType_SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(D3D12BindingType_UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else
        {
            D3D12_ERROR_CRITICAL("Unhandled shader resource type '%u' for parameter '%s' at register %u, space %u. This binding will be missing from the root signature.",
                ShaderBindDesc.Type, ShaderBindDesc.Name, ShaderBindDesc.BindPoint, ShaderBindDesc.Space);
            return false;
        }
    }

    BindingInfo = ::Move(NewBindingInfo);
    return true;
}

bool FD3D12RayTracingShader::GetShaderResourceBindings(ID3D12FunctionReflection* Reflection, uint32 NumBoundResources)
{
    FD3D12ShaderBindingInfo NewBindingInfo;
    FD3D12ShaderBindingInfo NewLocalBindingInfo;

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

        const bool bIsLocalSpace = IsRayTracingLocalSpace(ShaderBindDesc.Space);

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
                const uint8 NumShaderConstants = static_cast<uint8>(SizeInBytes) / static_cast<uint8>(sizeof(uint32));
                if (ShaderBindDesc.BindCount > 1 || NumShaderConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT || NewBindingInfo.NumPushConstants != 0)
                {
                    return false;
                }

                NewBindingInfo.NumPushConstants = NumShaderConstants;
            }
            else if (bIsLocalSpace)
            {
                NewLocalBindingInfo.AddBinding(D3D12BindingType_ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(D3D12BindingType_ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            if (bIsLocalSpace)
            {
                D3D12_ERROR_CRITICAL("Shader Parameter '%s': Samplers are not supported in RT local root signatures (space %u). Only buffer resources are allowed.", ShaderBindDesc.Name, ShaderBindDesc.Space);
                return false;
            }

            NewBindingInfo.AddBinding(D3D12BindingType_Sampler, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            if (bIsLocalSpace)
            {
                if (!IsBufferSRV(ShaderBindDesc.Type))
                {
                    D3D12_ERROR_CRITICAL("Shader Parameter '%s': Texture SRVs are not supported in RT local root signatures (space %u). Only buffer SRVs are allowed.", ShaderBindDesc.Name, ShaderBindDesc.Space);
                    return false;
                }

                NewLocalBindingInfo.AddBinding(D3D12BindingType_SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(D3D12BindingType_SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            if (bIsLocalSpace)
            {
                if (!IsBufferUAV(ShaderBindDesc.Type) && !(ShaderBindDesc.Type == D3D_SIT_UAV_RWTYPED && ShaderBindDesc.Dimension == D3D_SRV_DIMENSION_BUFFER))
                {
                    D3D12_ERROR_CRITICAL("Shader Parameter '%s': Texture UAVs are not supported in RT local root signatures (space %u). Only buffer UAVs are allowed.", ShaderBindDesc.Name, ShaderBindDesc.Space);
                    return false;
                }

                NewLocalBindingInfo.AddBinding(D3D12BindingType_UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(D3D12BindingType_UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
    }

    BindingInfo      = ::Move(NewBindingInfo);
    LocalBindingInfo = ::Move(NewLocalBindingInfo);
    return true;
}

bool FD3D12GraphicsShader::Initialize(const TArray<uint8>& InCode)
{
	if (!FD3D12Shader::Initialize(InCode))
	{
		return false;
	}

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.GetCode(), ByteCode.GetCodeSize());

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

bool FD3D12ComputeShader::Initialize(const TArray<uint8>& InCode)
{
    if (!FD3D12Shader::Initialize(InCode))
    {
        return false;
    }

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.GetCode(), ByteCode.GetCodeSize());

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

    TComPtr<IDxcBlob> ShaderBlob = new FExistingBlob((LPVOID)ByteCode.GetCode(), ByteCode.GetCodeSize());

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

