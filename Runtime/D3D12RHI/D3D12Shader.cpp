#include "Core/Math/Math.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RootSignature.h"

static ED3D12BindingType GetD3D12BindingType(EShaderResourceClass ResourceClass)
{
    switch (ResourceClass)
    {
        case EShaderResourceClass::SRV:     return ED3D12BindingType::SRV;
        case EShaderResourceClass::UAV:     return ED3D12BindingType::UAV;
        case EShaderResourceClass::Sampler: return ED3D12BindingType::Sampler;
        default:                            return ED3D12BindingType::ConstantBuffer;
    }
}

static ED3D12NullDescriptorType GetNullDescriptorType(const FShaderResourceBinding& Binding)
{
    switch (Binding.Type)
    {
        case EShaderResourceType::StructuredBuffer:
        case EShaderResourceType::RWStructuredBuffer:    return ED3D12NullDescriptorType::StructuredBuffer;
        case EShaderResourceType::ByteAddressBuffer:
        case EShaderResourceType::RWByteAddressBuffer:   return ED3D12NullDescriptorType::RawBuffer;
        case EShaderResourceType::AccelerationStructure: return ED3D12NullDescriptorType::AccelerationStructure;
        case EShaderResourceType::TypedBuffer:
        case EShaderResourceType::RWTypedBuffer:         return ED3D12NullDescriptorType::TypedBuffer;
        default:                                         break;
    }

    switch (Binding.Dimension)
    {
        case EShaderResourceDimension::Texture1D:        return ED3D12NullDescriptorType::Texture1D;
        case EShaderResourceDimension::Texture1DArray:   return ED3D12NullDescriptorType::Texture1DArray;
        case EShaderResourceDimension::Texture2DArray:   return ED3D12NullDescriptorType::Texture2DArray;
        case EShaderResourceDimension::Texture2DMS:      return ED3D12NullDescriptorType::Texture2DMS;
        case EShaderResourceDimension::Texture2DMSArray: return ED3D12NullDescriptorType::Texture2DMSArray;
        case EShaderResourceDimension::Texture3D:        return ED3D12NullDescriptorType::Texture3D;
        case EShaderResourceDimension::TextureCube:      return ED3D12NullDescriptorType::TextureCube;
        case EShaderResourceDimension::TextureCubeArray: return ED3D12NullDescriptorType::TextureCubeArray;
        default:                                         return ED3D12NullDescriptorType::Texture2D;
    }
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode()
    : ByteCode()
{
}

FD3D12ShaderBytecode::FD3D12ShaderBytecode(TArrayView<const uint8> InCode)
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

FD3D12ShaderBytecode::~FD3D12ShaderBytecode()
{
    Memory::Free(ByteCode.pShaderBytecode);

    ByteCode.pShaderBytecode = nullptr;
    ByteCode.BytecodeLength  = 0;
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

bool FD3D12Shader::Initialize(const FShaderCodeView& InCode)
{
    ByteCode = FD3D12ShaderBytecode(InCode.GetNativeCode());

    // The DXIL container starts with "DXBC" followed by a 16-byte checksum
    if (ByteCode.GetCodeSize() < 20)
    {
        ByteCodeHash = FD3D12ShaderHash();
        return false;
    }

    const uint8* CodeData = static_cast<const uint8*>(ByteCode.GetCode()) + 4;
    ByteCodeHash = *reinterpret_cast<const FD3D12ShaderHash*>(CodeData);

    BuildBindingInfo(InCode, EShaderBindingSpace::Global, BindingInfo);

    const FShaderReflectionInfo& Info = InCode.GetInfo();

    Flags                  = Info.RequiredFeatures;
    bContainsRootSignature = Info.HasFlag(EShaderReflectionFlags::HasEmbeddedRootSignature);
    return ValidateRequiresFlags(Flags, InCode.GetEntryPoint());
}

void FD3D12Shader::BuildBindingInfo(const FShaderCodeView& InCode, EShaderBindingSpace Space, FD3D12ShaderBindingInfo& OutBindingInfo)
{
    FD3D12ShaderBindingInfo NewBindingInfo;

    const TArrayView<const FShaderResourceBinding> Bindings = InCode.GetBindings();
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        const FShaderResourceBinding& Binding = Bindings[Index];
        if (Binding.Space != Space)
        {
            continue;
        }

        // Buffer<T> in a local root signature is bound like a texture SRV
        const bool bIsTexture = Space == EShaderBindingSpace::RayTracingLocal && (Binding.Type == EShaderResourceType::Texture || Binding.Type == EShaderResourceType::TypedBuffer);
        NewBindingInfo.AddBinding(GetD3D12BindingType(GetShaderResourceClass(Binding.Type)), Binding.Register, InCode.GetBindingName(Index), bIsTexture, GetNullDescriptorType(Binding));
    }

    if (Space == EShaderBindingSpace::Global)
    {
        NewBindingInfo.NumPushConstants = Math::DivideByMultiple<uint32>(InCode.GetInfo().ShaderConstantsSize, sizeof(uint32));
    }

    OutBindingInfo = ::Move(NewBindingInfo);
}

bool FD3D12Shader::ValidateRequiresFlags(ED3D12ShaderFlags InFlags, const CHAR* InShaderName)
{
    // Logs each missing capability and returns false if any required feature is missing.
    // Per-flag check is intentional so we surface every problem in a single shader-load.
    bool bAllSatisfied = true;

    // Release graphics and compute shaders carry no entry point
    MAYBE_UNUSED const CHAR* SafeName = (InShaderName != nullptr && InShaderName[0] != '\0') ? InShaderName : "<unnamed>";

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

FD3D12VertexShaderRHI::FD3D12VertexShaderRHI(FD3D12Device* InDevice)
    : FRHIVertexShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Vertex)
{
}

FD3D12VertexShaderRHI::~FD3D12VertexShaderRHI() = default;

void* FD3D12VertexShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12VertexShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12HullShaderRHI::FD3D12HullShaderRHI(FD3D12Device* InDevice)
    : FRHIHullShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Hull)
{
}

FD3D12HullShaderRHI::~FD3D12HullShaderRHI() = default;

void* FD3D12HullShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12HullShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12DomainShaderRHI::FD3D12DomainShaderRHI(FD3D12Device* InDevice)
    : FRHIDomainShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Domain)
{
}

FD3D12DomainShaderRHI::~FD3D12DomainShaderRHI() = default;

void* FD3D12DomainShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12DomainShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12GeometryShaderRHI::FD3D12GeometryShaderRHI(FD3D12Device* InDevice)
    : FRHIGeometryShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Geometry)
{
}

FD3D12GeometryShaderRHI::~FD3D12GeometryShaderRHI() = default;

void* FD3D12GeometryShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12GeometryShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12PixelShaderRHI::FD3D12PixelShaderRHI(FD3D12Device* InDevice)
    : FRHIPixelShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Pixel)
{
}

FD3D12PixelShaderRHI::~FD3D12PixelShaderRHI() = default;

void* FD3D12PixelShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12PixelShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12MeshShaderRHI::FD3D12MeshShaderRHI(FD3D12Device* InDevice)
    : FRHIMeshShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Mesh)
{
}

FD3D12MeshShaderRHI::~FD3D12MeshShaderRHI() = default;

void* FD3D12MeshShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12MeshShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12AmplificationShaderRHI::FD3D12AmplificationShaderRHI(FD3D12Device* InDevice)
    : FRHIAmplificationShader()
    , FD3D12GraphicsShader(InDevice, EShaderVisibility::Amplification)
{
}

FD3D12AmplificationShaderRHI::~FD3D12AmplificationShaderRHI() = default;

void* FD3D12AmplificationShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12AmplificationShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12GraphicsShader*>(this);
}

FD3D12RayTracingShader::FD3D12RayTracingShader(FD3D12Device* InDevice)
    : FD3D12Shader(InDevice, EShaderVisibility::All)
{
}

FD3D12RayTracingShader::~FD3D12RayTracingShader() = default;

bool FD3D12RayTracingShader::Initialize(const FShaderCodeView& InCode)
{
    if (!FD3D12Shader::Initialize(InCode))
    {
        return false;
    }

    BuildBindingInfo(InCode, EShaderBindingSpace::RayTracingLocal, LocalBindingInfo);

    Identifier = InCode.GetEntryPoint();
    if (Identifier.IsEmpty())
    {
        D3D12_ERROR_CRITICAL("[FD3D12RayTracingShader]: The shader code has no export name");
        return false;
    }

    return true;
}

FD3D12RayGenShaderRHI::FD3D12RayGenShaderRHI(FD3D12Device* InDevice)
    : FRHIRayGenShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayGenShaderRHI::~FD3D12RayGenShaderRHI() = default;

void* FD3D12RayGenShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayGenShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12RayAnyHitShaderRHI::FD3D12RayAnyHitShaderRHI(FD3D12Device* InDevice)
    : FRHIRayAnyHitShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayAnyHitShaderRHI::~FD3D12RayAnyHitShaderRHI() = default;

void* FD3D12RayAnyHitShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayAnyHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12RayClosestHitShaderRHI::FD3D12RayClosestHitShaderRHI(FD3D12Device* InDevice)
    : FRHIRayClosestHitShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayClosestHitShaderRHI::~FD3D12RayClosestHitShaderRHI() = default;

void* FD3D12RayClosestHitShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayClosestHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12RayMissShaderRHI::FD3D12RayMissShaderRHI(FD3D12Device* InDevice)
    : FRHIRayMissShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayMissShaderRHI::~FD3D12RayMissShaderRHI() = default;

void* FD3D12RayMissShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayMissShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12RayIntersectionShaderRHI::FD3D12RayIntersectionShaderRHI(FD3D12Device* InDevice)
    : FRHIRayIntersectionShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayIntersectionShaderRHI::~FD3D12RayIntersectionShaderRHI() = default;

void* FD3D12RayIntersectionShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayIntersectionShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12RayCallableShaderRHI::FD3D12RayCallableShaderRHI(FD3D12Device* InDevice)
    : FRHIRayCallableShader()
    , FD3D12RayTracingShader(InDevice)
{
}

FD3D12RayCallableShaderRHI::~FD3D12RayCallableShaderRHI() = default;

void* FD3D12RayCallableShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12RayCallableShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12RayTracingShader*>(this);
}

FD3D12ComputeShaderRHI::FD3D12ComputeShaderRHI(FD3D12Device* InDevice)
    : FRHIComputeShader()
    , FD3D12Shader(InDevice, EShaderVisibility::All)
{
}

FD3D12ComputeShaderRHI::~FD3D12ComputeShaderRHI() = default;

void* FD3D12ComputeShaderRHI::GetRHINativeHandle()
{
    return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode());
}

void* FD3D12ComputeShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FD3D12Shader*>(this);
}
