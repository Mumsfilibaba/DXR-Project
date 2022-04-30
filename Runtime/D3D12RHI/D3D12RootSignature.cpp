#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12RHIShader.h"
#include "D3D12FunctionPointers.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12_SHADER_VISIBILITY

static D3D12_SHADER_VISIBILITY GD3D12ShaderVisibility[ShaderVisibility_Count] =
{
    D3D12_SHADER_VISIBILITY_ALL,
    D3D12_SHADER_VISIBILITY_VERTEX,
    D3D12_SHADER_VISIBILITY_HULL,
    D3D12_SHADER_VISIBILITY_DOMAIN,
    D3D12_SHADER_VISIBILITY_GEOMETRY,
    D3D12_SHADER_VISIBILITY_PIXEL,
};

static D3D12_SHADER_VISIBILITY GetD3D12ShaderVisibility(uint32 Visbility)
{
    Assert(Visbility < ShaderVisibility_Count);
    return GD3D12ShaderVisibility[Visbility];
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ShaderVisibility

static EShaderVisibility GShaderVisibility[ShaderVisibility_Count] =
{
    ShaderVisibility_All,
    ShaderVisibility_Vertex,
    ShaderVisibility_Hull,
    ShaderVisibility_Domain,
    ShaderVisibility_Geometry,
    ShaderVisibility_Pixel
};

static EShaderVisibility GetShaderVisibility(uint32 Visbility)
{
    Assert(Visbility < ShaderVisibility_Count);
    return GShaderVisibility[Visbility];
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetResourceType

static EResourceType GetResourceType(D3D12_DESCRIPTOR_RANGE_TYPE Type)
{
    switch (Type)
    {
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV:     return ResourceType_CBV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV:     return ResourceType_SRV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV:     return ResourceType_UAV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: return ResourceType_Sampler;
    
    default:
        Assert(false);
        return ResourceType_Unknown;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SD3D12RootSignatureResourceCount

bool SD3D12RootSignatureResourceCount::IsCompatible(const SD3D12RootSignatureResourceCount& Other) const
{
    if (Type != Other.Type || AllowInputAssembler != Other.AllowInputAssembler)
    {
        return false;
    }

    for (uint32 i = 0; i < ShaderVisibility_Count; i++)
    {
        if (!ResourceCounts[i].IsCompatible(Other.ResourceCounts[i]))
        {
            return false;
        }
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignatureDescHelper

CD3D12RootSignatureDescHelper::CD3D12RootSignatureDescHelper(const SD3D12RootSignatureResourceCount& RootSignatureInfo)
    : Desc()
    , RootParameters()
    , DescriptorRanges()
    , NumDescriptorRanges(0)
{
    const D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags[ShaderVisibility_Count] =
    {
        D3D12_ROOT_SIGNATURE_FLAG_NONE,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
    };

    // NOTE: This can crash if the pipeline is using to many tables, max is 64
    const uint32 Space = (RootSignatureInfo.Type == ERootSignatureType::RayTracingLocal) ? D3D12_SHADER_REGISTER_SPACE_RT_LOCAL : 0;
    
    for (uint32 ShaderStage = 0; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        bool AddFlag = true;

        const SShaderResourceCount& ResourceCounts = RootSignatureInfo.ResourceCounts[ShaderStage];
        if (ResourceCounts.Ranges.NumCBVs > 0)
        {
            Assert(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            Assert(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, ResourceCounts.Ranges.NumCBVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumSRVs > 0)
        {
            Assert(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            Assert(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ResourceCounts.Ranges.NumSRVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumUAVs > 0)
        {
            Assert(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            Assert(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, ResourceCounts.Ranges.NumUAVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumSamplers > 0)
        {
            Assert(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            Assert(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ResourceCounts.Ranges.NumSamplers, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Num32BitConstants > 0)
        {
            Assert(ResourceCounts.Num32BitConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);
            Assert(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);

            Insert32BitConstantRange(GetD3D12ShaderVisibility(ShaderStage), ResourceCounts.Num32BitConstants, 0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);

            AddFlag = false;
        }

        if (AddFlag)
        {
            Desc.Flags |= RootSignatureFlags[ShaderStage];
        }
    }

    Assert(RootSignatureCost <= D3D12_MAX_ROOT_PARAMETER_COST);

    Desc.NumParameters = NumRootParameters;
    Desc.pParameters   = RootParameters;

    // TODO: Enable Static Samplers
    Desc.NumStaticSamplers = 0;
    Desc.pStaticSamplers   = nullptr;

    if (RootSignatureInfo.AllowInputAssembler)
    {
        Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (RootSignatureInfo.Type == ERootSignatureType::RayTracingLocal)
    {
        Desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    }
}

void CD3D12RootSignatureDescHelper::InitDescriptorRange(D3D12_DESCRIPTOR_RANGE& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace)
{
    Assert(NumDescriptors > 0);

    OutRange.BaseShaderRegister                = BaseShaderRegister;
    OutRange.NumDescriptors                    = NumDescriptors;
    OutRange.RangeType                         = Type;
    OutRange.RegisterSpace                     = RegisterSpace;
    OutRange.OffsetInDescriptorsFromTableStart = 0;
}

void CD3D12RootSignatureDescHelper::InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE* InDescriptorRanges, uint32 InNumDescriptorRanges)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    NewParameters.ShaderVisibility                    = ShaderVisibility;
    NewParameters.DescriptorTable.NumDescriptorRanges = InNumDescriptorRanges;
    NewParameters.DescriptorTable.pDescriptorRanges   = InDescriptorRanges;

    // Each descriptor table cost 1 DWORD
    RootSignatureCost++;
}

void CD3D12RootSignatureDescHelper::Insert32BitConstantRange(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 Num32BitConstants, uint32 ShaderRegister, uint32 RegisterSpace)
{
    Assert(Num32BitConstants > 0);

    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    NewParameters.ShaderVisibility         = ShaderVisibility;
    NewParameters.Constants.Num32BitValues = Num32BitConstants;
    NewParameters.Constants.ShaderRegister = ShaderRegister;
    NewParameters.Constants.RegisterSpace  = RegisterSpace;

    // Each constant cost 1 DWORD
    RootSignatureCost += Num32BitConstants;
}

void CD3D12RootSignatureDescHelper::InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}

void CD3D12RootSignatureDescHelper::InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}

void CD3D12RootSignatureDescHelper::InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignature

CD3D12RootSignature::CD3D12RootSignature(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , RootSignature(nullptr)
    , RootParameterMap()
    , ConstantRootParameterIndex(-1)
{
    constexpr uint32 NumElements = sizeof(RootParameterMap) / sizeof(uint32);

    int32* Ptr = reinterpret_cast<int32*>(&RootParameterMap);
    for (uint32 i = 0; i < NumElements; i++)
    {
        *(Ptr++) = -1;
    }
}

bool CD3D12RootSignature::Init(const SD3D12RootSignatureResourceCount& RootSignatureInfo)
{
    CD3D12RootSignatureDescHelper Desc(RootSignatureInfo);
    return Init(Desc.GetDesc());
}

bool CD3D12RootSignature::Init(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    TComPtr<ID3DBlob> SignatureBlob;

    if (!Serialize(Desc, &SignatureBlob))
    {
        return false;
    }

    CreateRootParameterMap(Desc);

    return InternalInit(SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize());
}

bool CD3D12RootSignature::Init(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    TComPtr<ID3D12RootSignatureDeserializer> Deserializer;
    HRESULT Result = ND3D12Functions::D3D12CreateRootSignatureDeserializer(BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&Deserializer));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12RootSignature]: FAILED to Retrieve Root Signature Desc");

        CDebug::DebugBreak();
        return false;
    }

    const D3D12_ROOT_SIGNATURE_DESC* Desc = Deserializer->GetRootSignatureDesc();
    Assert(Desc != nullptr);

    CreateRootParameterMap(*Desc);

    // Force a new serialization with Root Signature 1.0
    TComPtr<ID3DBlob> Blob;
    if (!Serialize(*Desc, &Blob))
    {
        return false;
    }

    Result = GetDevice()->CreateRootSignature(1, Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12RootSignature]: FAILED to Create RootSignature");

        CDebug::DebugBreak();
        return false;
    }

    return InternalInit(BlobWithRootSignature, BlobLengthInBytes);
}

void CD3D12RootSignature::CreateRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    for (uint32 i = 0; i < Desc.NumParameters; i++)
    {
        const D3D12_ROOT_PARAMETER& Parameter = Desc.pParameters[i];
        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);

            // NOTE: We may want to support multiple ranges
            Assert(Parameter.DescriptorTable.NumDescriptorRanges == 1);
            D3D12_DESCRIPTOR_RANGE Range = Parameter.DescriptorTable.pDescriptorRanges[0];

            uint32 ResourceType = GetResourceType(Range.RangeType);
            RootParameterMap[ShaderVisibility][ResourceType] = i;
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            Assert(ConstantRootParameterIndex == -1);
            ConstantRootParameterIndex = i;
        }
    }
}

bool CD3D12RootSignature::InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    HRESULT Result = GetDevice()->CreateRootSignature(1, BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12RootSignature]: FAILED to Create RootSignature");

        CDebug::DebugBreak();
        return false;
    }

    return true;
}

bool CD3D12RootSignature::Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob)
{
    TComPtr<ID3DBlob> ErrorBlob;

    HRESULT Result = ND3D12Functions::D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, OutBlob, &ErrorBlob);
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12RootSignature]: FAILED to Serialize RootSignature");
        LOG_ERROR(reinterpret_cast<const char*>(ErrorBlob->GetBufferPointer()));

        CDebug::DebugBreak();
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignatureCache

CD3D12RootSignatureCache* CD3D12RootSignatureCache::Instance = nullptr;

CD3D12RootSignatureCache::CD3D12RootSignatureCache(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , RootSignatures()
    , ResourceCounts()
{
    Instance = this;
}

CD3D12RootSignatureCache::~CD3D12RootSignatureCache()
{
    ReleaseAll();
    Instance = nullptr;
}

bool CD3D12RootSignatureCache::Init()
{
    SD3D12RootSignatureResourceCount GraphicsKey;
    GraphicsKey.Type                = ERootSignatureType::Graphics;
    GraphicsKey.AllowInputAssembler = true;
    GraphicsKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;

    // NOTE: Skips visibility all, however constants are still visible to all stages
    for (uint32 i = 1; i < ShaderVisibility_Count; i++)
    {
        GraphicsKey.ResourceCounts[i].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
        GraphicsKey.ResourceCounts[i].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
        GraphicsKey.ResourceCounts[i].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
        GraphicsKey.ResourceCounts[i].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;
    }

    CD3D12RootSignature* GraphicsRootSignature = CreateRootSignature(GraphicsKey);
    if (!GraphicsRootSignature)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        GraphicsRootSignature->SetName("Default Graphics RootSignature");
    }

    SD3D12RootSignatureResourceCount ComputeKey;
    ComputeKey.Type                = ERootSignatureType::Compute;
    ComputeKey.AllowInputAssembler = false;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants  = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    CD3D12RootSignature* ComputeRootSignature = CreateRootSignature(ComputeKey);
    if (!ComputeRootSignature)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ComputeRootSignature->SetName("Default Compute RootSignature");
    }

    if (GetDevice()->GetRayTracingTier() == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        return true;
    }

    SD3D12RootSignatureResourceCount RTGlobalKey;
    RTGlobalKey.Type                = ERootSignatureType::RayTracingGlobal;
    RTGlobalKey.AllowInputAssembler = false;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants  = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    CD3D12RootSignature* RTGlobalRootSignature = CreateRootSignature(RTGlobalKey);
    if (!RTGlobalRootSignature)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RTGlobalRootSignature->SetName("Default Global RayTracing RootSignature");
    }

    SD3D12RootSignatureResourceCount RTLocalKey;
    RTLocalKey.Type                = ERootSignatureType::RayTracingLocal;
    RTLocalKey.AllowInputAssembler = false;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_LOCAL_CONSTANT_BUFFER_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_LOCAL_SHADER_RESOURCE_VIEW_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_LOCAL_UNORDERED_ACCESS_VIEW_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_LOCAL_SAMPLER_STATE_COUNT;

    CD3D12RootSignature* RTLocalRootSignature = CreateRootSignature(RTLocalKey);
    if (!RTLocalRootSignature)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RTLocalRootSignature->SetName("Default Local RayTracing RootSignature");
    }

    return true;
}

void CD3D12RootSignatureCache::ReleaseAll()
{
    for (TSharedRef<CD3D12RootSignature> RootSignature : RootSignatures)
    {
        RootSignature.Reset();
    }

    RootSignatures.Clear();
    ResourceCounts.Clear();
}

CD3D12RootSignature* CD3D12RootSignatureCache::GetOrCreateRootSignature(const SD3D12RootSignatureResourceCount& ResourceCount)
{
    Assert(RootSignatures.Size() == ResourceCounts.Size());

    for (int32 i = 0; i < ResourceCounts.Size(); i++)
    {
        if (ResourceCount.IsCompatible(ResourceCounts[i]))
        {
            return RootSignatures[i].Get();
        }
    }

    // Make sure that this root signature can be used by more than one pipeline
    SD3D12RootSignatureResourceCount NewResourceCount = ResourceCount;
    for (uint32 i = 0; i < ShaderVisibility_Count; i++)
    {
        SShaderResourceCount& Count = NewResourceCount.ResourceCounts[i];
        if (Count.Ranges.NumCBVs > 0)
        {
            Count.Ranges.NumCBVs = NMath::Max<uint32>(Count.Ranges.NumCBVs, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
        }
        if (Count.Ranges.NumSRVs > 0)
        {
            Count.Ranges.NumSRVs = NMath::Max<uint32>(Count.Ranges.NumSRVs, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
        }
        if (Count.Ranges.NumUAVs > 0)
        {
            Count.Ranges.NumUAVs = NMath::Max<uint32>(Count.Ranges.NumUAVs, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
        }
        if (Count.Ranges.NumSamplers > 0)
        {
            Count.Ranges.NumSamplers = NMath::Max<uint32>(Count.Ranges.NumSamplers, D3D12_DEFAULT_SAMPLER_STATE_COUNT);
        }
    }

    return CreateRootSignature(NewResourceCount);
}

CD3D12RootSignatureCache& CD3D12RootSignatureCache::Get()
{
    Assert(Instance != nullptr);
    return *Instance;
}

CD3D12RootSignature* CD3D12RootSignatureCache::CreateRootSignature(const SD3D12RootSignatureResourceCount& ResourceCount)
{
    TSharedRef<CD3D12RootSignature> NewRootSignature = dbg_new CD3D12RootSignature(GetDevice());
    if (!NewRootSignature->Init(ResourceCount))
    {
        return nullptr;
    }

    LOG_INFO("Created new root signature");

    RootSignatures.Emplace(NewRootSignature);
    ResourceCounts.Emplace(ResourceCount);
    return NewRootSignature.Get();
}