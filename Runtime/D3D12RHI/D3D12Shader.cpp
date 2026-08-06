#include "Core/Math/Math.h"
#include "Core/Misc/CRC.h"
#include "Core/RefCountedBase.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12Device.h"
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
    return RegisterSpace == D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL;
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

static bool ValidatePushConstantBinding(const D3D12_SHADER_INPUT_BIND_DESC& ShaderBindDesc, uint32 SizeInBytes, uint32 ExistingNumPushConstants, uint32& OutNumPushConstants)
{
    constexpr uint32 BytesPerConstant = sizeof(uint32);
    MAYBE_UNUSED constexpr uint32 MaxSizeInBytes = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT * BytesPerConstant;

    if (ShaderBindDesc.BindCount > 1)
    {
        D3D12_ERROR_CRITICAL("Shader Parameter '%s' is an array of %u 32-bit constant buffers, only a single constant buffer is supported in register space %u.",
            ShaderBindDesc.Name, ShaderBindDesc.BindCount, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
        return false;
    }

    if (ExistingNumPushConstants != 0)
    {
        D3D12_ERROR_CRITICAL("Shader Parameter '%s' declares a second 32-bit constant buffer, only one is supported per shader.", ShaderBindDesc.Name);
        return false;
    }

    if (SizeInBytes == 0)
    {
        D3D12_ERROR_CRITICAL("Shader Parameter '%s' at register %u is a 32-bit constant buffer, but its size could not be retrieved from reflection.",
            ShaderBindDesc.Name, ShaderBindDesc.BindPoint);
        return false;
    }

    const uint32 NumShaderConstants = Math::DivideByMultiple(SizeInBytes, BytesPerConstant);
    if (NumShaderConstants > D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT)
    {
        D3D12_ERROR_CRITICAL("Shader Parameter '%s' is %u bytes (%u 32-bit constants), which exceeds the maximum of %u constants (%u bytes).",
            ShaderBindDesc.Name, SizeInBytes, NumShaderConstants, D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT, MaxSizeInBytes);
        return false;
    }

    OutNumPushConstants = NumShaderConstants;
    return true;
}


#ifndef MAKEFOURCC
    #define MAKEFOURCC(a, b, c, d) (unsigned int)((unsigned char)(a) | ((unsigned char)(b) << 8) | ((unsigned char)(c) << 16) | ((unsigned char)(d) << 24))
#endif

enum DxilFourCC : uint32
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
		Data = Memory::Malloc(SizeInBytes);
		Memory::Memcpy(Data, InData, SizeInBytes);
	}

	~FExistingBlob()
	{
		Memory::Free(Data);
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
    ByteCode.pShaderBytecode = Memory::Malloc(ByteCode.BytecodeLength);
    Memory::Memcpy((void*)ByteCode.pShaderBytecode, InCode.Data(), ByteCode.BytecodeLength);
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode(const FD3D12ShaderBytecode& Other)
    : ByteCode()
{
    if (Other.ByteCode.pShaderBytecode)
    {
        ByteCode.BytecodeLength  = Other.ByteCode.BytecodeLength;
        ByteCode.pShaderBytecode = Memory::Malloc(ByteCode.BytecodeLength);
        Memory::Memcpy((void*)ByteCode.pShaderBytecode, Other.ByteCode.pShaderBytecode, ByteCode.BytecodeLength);
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
        Memory::Free(ByteCode.pShaderBytecode);
        ByteCode = {};

        if (Other.ByteCode.pShaderBytecode)
        {
            ByteCode.BytecodeLength  = Other.ByteCode.BytecodeLength;
            ByteCode.pShaderBytecode = Memory::Malloc(ByteCode.BytecodeLength);

            Memory::Memcpy((void*)ByteCode.pShaderBytecode, Other.ByteCode.pShaderBytecode, ByteCode.BytecodeLength);
        }
    }

    return *this;
}

FD3D12ShaderBytecode& FD3D12ShaderBytecode::operator=(FD3D12ShaderBytecode&& Other)
{
    if (this != &Other)
    {
        Memory::Free(ByteCode.pShaderBytecode);

        ByteCode = Other.ByteCode;

        Other.ByteCode.pShaderBytecode = nullptr;
        Other.ByteCode.BytecodeLength  = 0;
    }

    return *this;
}

FD3D12ShaderBytecode::~FD3D12ShaderBytecode()
{
    Memory::Free(ByteCode.pShaderBytecode);

    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
}

FD3D12Shader::FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility)
    : FD3D12DeviceChild(InDevice)
    , ByteCodeHash()
    , ShaderVisibility(InShaderVisibility)
    , BindingInfo()
    , Flags(ED3D12ShaderFlags::None)
    , bContainsRootSignature(false)
{
}

FD3D12Shader::~FD3D12Shader()
{
}

ED3D12ShaderFlags FD3D12Shader::TranslateD3D12ShaderRequires(uint64 Mask)
{
    ED3D12ShaderFlags Result = ED3D12ShaderFlags::None;

    if ((Mask & D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing;
    }

    if ((Mask & D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing;
    }

    if ((Mask & D3D_SHADER_REQUIRES_EARLY_DEPTH_STENCIL) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresEarlyDepthStencil;
    }

    if ((Mask & D3D_SHADER_REQUIRES_STENCIL_REF) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresStencilRef;
    }

    if ((Mask & D3D_SHADER_REQUIRES_INNER_COVERAGE) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresInnerCoverage;
    }

    if ((Mask & D3D_SHADER_REQUIRES_ROVS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresROVs;
    }

    if ((Mask & D3D_SHADER_REQUIRES_WAVE_OPS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresWaveOps;
    }

    if ((Mask & D3D_SHADER_REQUIRES_INT64_OPS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresInt64Ops;
    }

    if ((Mask & D3D_SHADER_REQUIRES_NATIVE_16BIT_OPS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresNative16BitOps;
    }

    if ((Mask & D3D_SHADER_REQUIRES_BARYCENTRICS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresBarycentrics;
    }

    if ((Mask & D3D_SHADER_REQUIRES_VIEW_ID) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresViewID;
    }

    if ((Mask & D3D_SHADER_REQUIRES_SHADING_RATE) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresShadingRate;
    }

    if ((Mask & D3D_SHADER_REQUIRES_RAYTRACING_TIER_1_1) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresRaytracingTier1_1;
    }

    if ((Mask & D3D_SHADER_REQUIRES_SAMPLER_FEEDBACK) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresSamplerFeedback;
    }

    if ((Mask & D3D_SHADER_REQUIRES_TILED_RESOURCES) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresTiledResources;
    }

    if ((Mask & D3D_SHADER_REQUIRES_TYPED_UAV_LOAD_ADDITIONAL_FORMATS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresTypedUAVLoadAdditionalFormats;
    }

    if ((Mask & D3D_SHADER_REQUIRES_VIEWPORT_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER_FEEDING_RASTERIZER) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresVPAndRTArrayIndexFromAnyShader;
    }

    if ((Mask & D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_TYPED_RESOURCE) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresAtomicInt64OnTypedResource;
    }

    if ((Mask & D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_GROUP_SHARED) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresAtomicInt64OnGroupShared;
    }

    if ((Mask & D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_DESCRIPTOR_HEAP_RESOURCE) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresAtomicInt64OnDescriptorHeapResource;
    }

    if ((Mask & D3D_SHADER_REQUIRES_WAVE_MMA) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresWaveMMA;
    }

    if ((Mask & D3D_SHADER_REQUIRES_DERIVATIVES_IN_MESH_AND_AMPLIFICATION_SHADERS) != 0)
    {
        Result |= ED3D12ShaderFlags::RequiresDerivativesInMeshAndAmpShaders;
    }

    return Result;
}

bool FD3D12Shader::ValidateRequiresFlags(ED3D12ShaderFlags InFlags, const CHAR* InShaderName)
{
    // Logs each missing capability and returns false if any required feature is missing.
    // Per-flag check is intentional so we surface every problem in a single shader-load.
    bool bAllSatisfied = true;

    MAYBE_UNUSED const CHAR* SafeName = (InShaderName != nullptr) ? InShaderName : "<unnamed>";

    auto ReportMissing = [&](const CHAR* InFeature)
    {
        UNREFERENCED_VARIABLE(InFeature);
        D3D12_ERROR("[FD3D12Shader] Shader '%s' requires '%s' which the current device does not support", SafeName, InFeature);
        bAllSatisfied = false;
    };

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing) && !GD3D12SupportsBindless)
    {
        ReportMissing("ResourceDescriptorHeap indexing (SM6.6 dynamic resources)");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing) && !GD3D12SupportsBindless)
    {
        ReportMissing("SamplerDescriptorHeap indexing (SM6.6 dynamic resources)");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresStencilRef) && !GD3D12PSSpecifiedStencilRefSupported)
    {
        ReportMissing("PS-specified SV_StencilRef");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresInnerCoverage) && GD3D12ConservativeRasterizationTier < D3D12_CONSERVATIVE_RASTERIZATION_TIER_3)
    {
        ReportMissing("Inner Coverage (Conservative Raster Tier 3)");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresROVs) && !GD3D12RasterizerOrderViewsSupported)
    {
        ReportMissing("Rasterizer Ordered Views");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresWaveOps) && !GD3D12WaveOpsSupported)
    {
        ReportMissing("Wave Intrinsics");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresInt64Ops) && !GD3D12Int64ShaderOpsSupported)
    {
        ReportMissing("64-bit integer shader ops");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresNative16BitOps) && !GD3D12Native16BitShaderOpsSupported)
    {
        ReportMissing("Native 16-bit shader ops");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresBarycentrics) && !GD3D12BarycentricsSupported)
    {
        ReportMissing("SV_Barycentrics");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresViewID) && GD3D12ViewInstancingTier == D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
    {
        ReportMissing("View Instancing (SV_ViewID)");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresShadingRate) && GD3D12VariableRateShadingTier < D3D12_VARIABLE_SHADING_RATE_TIER_2)
    {
        ReportMissing("Variable Rate Shading Tier 2 (per-primitive SV_ShadingRate)");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresRaytracingTier1_1) && GD3D12RayTracingTier < D3D12_RAYTRACING_TIER_1_1)
    {
        ReportMissing("Ray Tracing Tier 1.1");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresSamplerFeedback) && GD3D12SamplerFeedbackTier == D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED)
    {
        ReportMissing("Sampler Feedback");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresTiledResources) && GD3D12TiledResourcesTier == D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED)
    {
        ReportMissing("Tiled Resources");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresTypedUAVLoadAdditionalFormats) && !GD3D12TypedUAVLoadAdditionalFormats)
    {
        ReportMissing("Typed UAV Load Additional Formats");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresVPAndRTArrayIndexFromAnyShader) && GD3D12ViewInstancingTier == D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
    {
        ReportMissing("Viewport and RT Array Index from any shader feeding rasterizer");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresAtomicInt64OnTypedResource) && !GD3D12AtomicInt64OnTypedResourceSupported)
    {
        ReportMissing("Atomic Int64 on typed resources");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresAtomicInt64OnGroupShared) && !GD3D12AtomicInt64OnGroupSharedSupported)
    {
        ReportMissing("Atomic Int64 on group-shared memory");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresAtomicInt64OnDescriptorHeapResource) && !GD3D12AtomicInt64OnDescriptorHeapResourceSupported)
    {
        ReportMissing("Atomic Int64 on descriptor-heap resources");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresWaveMMA) && GD3D12WaveMMATier == D3D12_WAVE_MMA_TIER_NOT_SUPPORTED)
    {
        ReportMissing("Wave MMA");
    }

    if (IsEnumFlagSet(InFlags, ED3D12ShaderFlags::RequiresDerivativesInMeshAndAmpShaders) && !GD3D12DerivativesInMeshAndAmpShadersSupported)
    {
        ReportMissing("Derivatives in Mesh / Amplification shaders");
    }

    return bAllSatisfied;
}

FD3D12GraphicsShader::FD3D12GraphicsShader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility)
    : FD3D12Shader(InDevice, InShaderVisibility)
{
}

FD3D12GraphicsShader::~FD3D12GraphicsShader() = default;

FD3D12RayTracingShader::FD3D12RayTracingShader(FD3D12Device* InDevice)
    : FD3D12Shader(InDevice, EShaderVisibility::All)
{
}

FD3D12RayTracingShader::~FD3D12RayTracingShader() = default;

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

bool FD3D12Shader::ReadShaderFeatureFlags(const TComPtr<IDxcBlob>& ShaderBlob, uint64& OutFlags)
{
    OutFlags = 0;

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

    uint32 PartIndex = 0;
    Result = Reflection->FindFirstPartKind(DFCC_FeatureInfo, &PartIndex);
    if (FAILED(Result))
    {
        return true;
    }

    TComPtr<IDxcBlob> PartBlob;
    Result = Reflection->GetPartContent(PartIndex, &PartBlob);
    if (FAILED(Result) || !PartBlob)
    {
        return true;
    }

    if (PartBlob->GetBufferSize() >= sizeof(uint64) && PartBlob->GetBufferPointer())
    {
        Memory::Memcpy(&OutFlags, PartBlob->GetBufferPointer(), sizeof(uint64));
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
                uint32 NumShaderConstants = 0;
                if (!ValidatePushConstantBinding(ShaderBindDesc, SizeInBytes, NewBindingInfo.NumPushConstants, NumShaderConstants))
                {
                    return false;
                }

                NewBindingInfo.NumPushConstants = NumShaderConstants;
            }
            else
            {
                NewBindingInfo.AddBinding(ED3D12BindingType::ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            NewBindingInfo.AddBinding(ED3D12BindingType::Sampler, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(ED3D12BindingType::SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
        }
        else if (IsUnorderedAccessView(ShaderBindDesc.Type))
        {
            NewBindingInfo.AddBinding(ED3D12BindingType::UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
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
                uint32 NumShaderConstants = 0;
                if (!ValidatePushConstantBinding(ShaderBindDesc, SizeInBytes, NewBindingInfo.NumPushConstants, NumShaderConstants))
                {
                    return false;
                }

                NewBindingInfo.NumPushConstants = NumShaderConstants;
            }
            else if (bIsLocalSpace)
            {
                NewLocalBindingInfo.AddBinding(ED3D12BindingType::ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(ED3D12BindingType::ConstantBuffer, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (ShaderBindDesc.Type == D3D_SIT_SAMPLER)
        {
            if (bIsLocalSpace)
            {
                NewLocalBindingInfo.AddBinding(ED3D12BindingType::Sampler, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(ED3D12BindingType::Sampler, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
        }
        else if (IsShaderResourceView(ShaderBindDesc.Type))
        {
            if (bIsLocalSpace)
            {
                const bool bIsTexture = !IsBufferSRV(ShaderBindDesc.Type);
                NewLocalBindingInfo.AddBinding(ED3D12BindingType::SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name, bIsTexture);
            }
            else
            {
                NewBindingInfo.AddBinding(ED3D12BindingType::SRV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
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

                NewLocalBindingInfo.AddBinding(ED3D12BindingType::UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
            }
            else
            {
                NewBindingInfo.AddBinding(ED3D12BindingType::UAV, static_cast<uint16>(ShaderBindDesc.BindPoint), ShaderBindDesc.Name);
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

	Flags |= TranslateD3D12ShaderRequires(static_cast<uint64>(Reflection->GetRequiresFlags()));

	if (!ValidateRequiresFlags(Flags, nullptr))
	{
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

    Flags |= TranslateD3D12ShaderRequires(static_cast<uint64>(Reflection->GetRequiresFlags()));

    if (!ValidateRequiresFlags(Flags, nullptr))
    {
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

	if (LibraryDesc.FunctionCount == 0)
	{
        D3D12_ERROR("[FD3D12RayTracingShader]: No functions in shader-library");
		return false;
	}

	// Make sure that the first shader is the one we wanted
	ID3D12FunctionReflection* Function = Reflection->GetFunctionByIndex(0);

	D3D12_FUNCTION_DESC FunctionDesc = {};
	Result = Function->GetDesc(&FunctionDesc);
	if (FAILED(Result))
	{
		return false;
	}

	if (!GetShaderResourceBindings(Function, FunctionDesc.BoundResources))
	{
		D3D12_ERROR_CRITICAL("[FD3D12RayTracingShader]: Error when analysing shader parameters");
		return false;
	}

	uint64 FeatureFlags = 0;
	if (ReadShaderFeatureFlags(ShaderBlob, FeatureFlags))
	{
		Flags |= TranslateD3D12ShaderRequires(FeatureFlags);
	}

	if (!ValidateRequiresFlags(Flags, FunctionDesc.Name))
	{
		return false;
	}

	// HACK: Since the NVIDIA driver can't handle these names, we have to change the names :(
	const String FuncIdentifier = FunctionDesc.Name;

	int32 NameStart = FuncIdentifier.FindLastCharWithPredicate([](CHAR Char) -> bool
	{
		return (Char == '\x1') || (Char == '?');
	});

	if (NameStart != String::InvalidIndex)
	{
		NameStart++;
	}

	const int32 NameEnd = FuncIdentifier.Find("@");
	Identifier = FuncIdentifier.SubString(NameStart, NameEnd - NameStart);
	return true;
}

FD3D12VertexShaderRHI::FD3D12VertexShaderRHI(FD3D12Device* InDevice)
    : FRHIVertexShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Vertex)
{
}

FD3D12VertexShaderRHI::~FD3D12VertexShaderRHI() = default;

FD3D12HullShaderRHI::FD3D12HullShaderRHI(FD3D12Device* InDevice)
    : FRHIHullShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Hull)
{
}

FD3D12HullShaderRHI::~FD3D12HullShaderRHI() = default;

FD3D12DomainShaderRHI::FD3D12DomainShaderRHI(FD3D12Device* InDevice)
    : FRHIDomainShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Domain)
{
}

FD3D12DomainShaderRHI::~FD3D12DomainShaderRHI() = default;

FD3D12GeometryShaderRHI::FD3D12GeometryShaderRHI(FD3D12Device* InDevice)
    : FRHIGeometryShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Geometry)
{
}

FD3D12GeometryShaderRHI::~FD3D12GeometryShaderRHI() = default;

FD3D12PixelShaderRHI::FD3D12PixelShaderRHI(FD3D12Device* InDevice)
    : FRHIPixelShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Pixel)
{
}

FD3D12PixelShaderRHI::~FD3D12PixelShaderRHI() = default;

FD3D12MeshShaderRHI::FD3D12MeshShaderRHI(FD3D12Device* InDevice)
    : FRHIMeshShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Mesh)
{
}

FD3D12MeshShaderRHI::~FD3D12MeshShaderRHI() = default;

FD3D12AmplificationShaderRHI::FD3D12AmplificationShaderRHI(FD3D12Device* InDevice)
    : FRHIAmplificationShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Amplification)
{
}

FD3D12AmplificationShaderRHI::~FD3D12AmplificationShaderRHI() = default;

FD3D12RayGenShaderRHI::FD3D12RayGenShaderRHI(FD3D12Device* InDevice)
    : FRHIRayGenShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayGenShaderRHI::~FD3D12RayGenShaderRHI() = default;

FD3D12RayAnyHitShaderRHI::FD3D12RayAnyHitShaderRHI(FD3D12Device* InDevice)
    : FRHIRayAnyHitShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayAnyHitShaderRHI::~FD3D12RayAnyHitShaderRHI() = default;

FD3D12RayClosestHitShaderRHI::FD3D12RayClosestHitShaderRHI(FD3D12Device* InDevice)
    : FRHIRayClosestHitShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayClosestHitShaderRHI::~FD3D12RayClosestHitShaderRHI() = default;

FD3D12RayMissShaderRHI::FD3D12RayMissShaderRHI(FD3D12Device* InDevice)
    : FRHIRayMissShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayMissShaderRHI::~FD3D12RayMissShaderRHI() = default;

FD3D12RayIntersectionShaderRHI::FD3D12RayIntersectionShaderRHI(FD3D12Device* InDevice)
    : FRHIRayIntersectionShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayIntersectionShaderRHI::~FD3D12RayIntersectionShaderRHI() = default;

FD3D12RayCallableShaderRHI::FD3D12RayCallableShaderRHI(FD3D12Device* InDevice)
    : FRHIRayCallableShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayCallableShaderRHI::~FD3D12RayCallableShaderRHI() = default;

FD3D12ComputeShaderRHI::FD3D12ComputeShaderRHI(FD3D12Device* InDevice)
    : FRHIComputeShader()
    , FD3D12Shader(InDevice, EShaderVisibility::All)
    , ThreadGroupXYZ(0, 0, 0)
{
}

FD3D12ComputeShaderRHI::~FD3D12ComputeShaderRHI() = default;

void* FD3D12VertexShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12HullShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12DomainShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12GeometryShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12PixelShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12MeshShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12AmplificationShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayGenShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayAnyHitShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayClosestHitShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayMissShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayIntersectionShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayCallableShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12ComputeShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12VertexShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12HullShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12DomainShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12GeometryShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12PixelShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12MeshShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12AmplificationShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

void* FD3D12RayGenShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12RayAnyHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12RayClosestHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12RayMissShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12RayIntersectionShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12RayCallableShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

void* FD3D12ComputeShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12Shader*>(this);
}
