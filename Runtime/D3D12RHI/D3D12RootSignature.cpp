#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12Device.h"
#include "D3D12Shader.h"
#include "DynamicD3D12.h"
#include "D3D12RHI.h"

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
    CHECK(Visbility < ShaderVisibility_Count);
    return GD3D12ShaderVisibility[Visbility];
}


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
    CHECK(Visbility < ShaderVisibility_Count);
    return GShaderVisibility[Visbility];
}


static EResourceType GetResourceType(D3D12_DESCRIPTOR_RANGE_TYPE Type)
{
    switch (Type)
    {
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV:     return ResourceType_CBV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV:     return ResourceType_SRV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV:     return ResourceType_UAV;
    case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: return ResourceType_Sampler;
    
    default:
        CHECK(false);
        return ResourceType_Unknown;
    }
}


bool FD3D12RootSignatureLayout::IsCompatible(const FD3D12RootSignatureLayout& Other) const
{
    if (Type != Other.Type || bAllowInputAssembler != Other.bAllowInputAssembler)
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


FD3D12RootSignatureDescHelper::FD3D12RootSignatureDescHelper(const FD3D12RootSignatureLayout& RootSignatureInfo)
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

        const FShaderResourceCount& ResourceCounts = RootSignatureInfo.ResourceCounts[ShaderStage];
        if (ResourceCounts.Ranges.NumCBVs > 0)
        {
            CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            CHECK(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, ResourceCounts.Ranges.NumCBVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumSRVs > 0)
        {
            CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            CHECK(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ResourceCounts.Ranges.NumSRVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumUAVs > 0)
        {
            CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            CHECK(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, ResourceCounts.Ranges.NumUAVs, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Ranges.NumSamplers > 0)
        {
            CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGES);
            CHECK(NumRootParameters   < D3D12_MAX_ROOT_PARAMETERS);

            InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ResourceCounts.Ranges.NumSamplers, 0, Space);
            InsertDescriptorTable(GetD3D12ShaderVisibility(ShaderStage), &DescriptorRanges[NumDescriptorRanges], 1);

            NumDescriptorRanges++;

            AddFlag = false;
        }

        if (ResourceCounts.Num32BitConstants > 0)
        {
            CHECK(ResourceCounts.Num32BitConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);
            CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);

            Insert32BitConstantRange(GetD3D12ShaderVisibility(ShaderStage), ResourceCounts.Num32BitConstants, 0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);

            AddFlag = false;
        }

        if (AddFlag)
        {
            Desc.Flags |= RootSignatureFlags[ShaderStage];
        }
    }

    CHECK(RootSignatureCost <= D3D12_MAX_ROOT_PARAMETER_COST);
    D3D12_INFO("[FD3D12RootSignatureDescHelper] RootSignatureCost=%u", RootSignatureCost);

    Desc.NumParameters = NumRootParameters;
    Desc.pParameters   = RootParameters;

    // TODO: Enable Static Samplers
    Desc.NumStaticSamplers = 0;
    Desc.pStaticSamplers   = nullptr;

    if (RootSignatureInfo.bAllowInputAssembler)
    {
        Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (RootSignatureInfo.Type == ERootSignatureType::RayTracingLocal)
    {
        Desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    }
}

void FD3D12RootSignatureDescHelper::InitDescriptorRange(D3D12_DESCRIPTOR_RANGE& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace)
{
    CHECK(NumDescriptors > 0);

    OutRange.BaseShaderRegister                = BaseShaderRegister;
    OutRange.NumDescriptors                    = NumDescriptors;
    OutRange.RangeType                         = Type;
    OutRange.RegisterSpace                     = RegisterSpace;
    OutRange.OffsetInDescriptorsFromTableStart = 0;
}

void FD3D12RootSignatureDescHelper::InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE* InDescriptorRanges, uint32 InNumDescriptorRanges)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    NewParameters.ShaderVisibility                    = ShaderVisibility;
    NewParameters.DescriptorTable.NumDescriptorRanges = InNumDescriptorRanges;
    NewParameters.DescriptorTable.pDescriptorRanges   = InDescriptorRanges;

    // Each descriptor table cost 1 DWORD
    RootSignatureCost++;
}

void FD3D12RootSignatureDescHelper::Insert32BitConstantRange(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 Num32BitConstants, uint32 ShaderRegister, uint32 RegisterSpace)
{
    CHECK(Num32BitConstants > 0);

    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    NewParameters.ShaderVisibility         = ShaderVisibility;
    NewParameters.Constants.Num32BitValues = Num32BitConstants;
    NewParameters.Constants.ShaderRegister = ShaderRegister;
    NewParameters.Constants.RegisterSpace  = RegisterSpace;

    // Each constant cost 1 DWORD
    RootSignatureCost += Num32BitConstants;
}

void FD3D12RootSignatureDescHelper::InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    // Each root descriptor cost 2 DWORDs
    RootSignatureCost += 2;
}


FD3D12RootSignature::FD3D12RootSignature(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , RootSignature(nullptr)
    , RootParameterMap()
    , ConstantRootParameterIndex(-1)
{
    for (int32 CurrentStage = ShaderVisibility_All; CurrentStage < ShaderVisibility_Count; CurrentStage++)
    {
        for (int32 Index = 0; Index < ResourceType_Count; Index++)
        {
            RootParameterMap[CurrentStage].RootParameterIndicies[Index] = -1;
            RootParameterMap[CurrentStage].ResourceCount[Index]         = 0;
        }
    }
}

bool FD3D12RootSignature::Initialize(const FD3D12RootSignatureLayout& RootSignatureInfo)
{
    FD3D12RootSignatureDescHelper Desc(RootSignatureInfo);
    return Initialize(Desc.GetDesc());
}

bool FD3D12RootSignature::Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    TComPtr<ID3DBlob> SignatureBlob;

    if (!Serialize(Desc, &SignatureBlob))
    {
        return false;
    }

    InternalInitRootParameterMap(Desc);

    return InternalInit(SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize());
}

bool FD3D12RootSignature::Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    TComPtr<ID3D12RootSignatureDeserializer> Deserializer;
    HRESULT Result = FDynamicD3D12::D3D12CreateRootSignatureDeserializer(BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&Deserializer));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12RootSignature]: FAILED to Retrieve Root Signature Desc");
        return false;
    }

    const D3D12_ROOT_SIGNATURE_DESC* Desc = Deserializer->GetRootSignatureDesc();
    CHECK(Desc != nullptr);

    InternalInitRootParameterMap(*Desc);

    // Force a new serialization with Root Signature 1.0
    TComPtr<ID3DBlob> Blob;
    if (!Serialize(*Desc, &Blob))
    {
        return false;
    }

    Result = GetDevice()->GetD3D12Device()->CreateRootSignature(1, Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12RootSignature]: FAILED to Create RootSignature");
        return false;
    }

    return InternalInit(BlobWithRootSignature, BlobLengthInBytes);
}

void FD3D12RootSignature::InternalInitRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    for (uint32 Index = 0; Index < Desc.NumParameters; Index++)
    {
        const D3D12_ROOT_PARAMETER& Parameter = Desc.pParameters[Index];
        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            // NOTE: We may want to support multiple ranges
            CHECK(Parameter.DescriptorTable.NumDescriptorRanges == 1);
            const D3D12_DESCRIPTOR_RANGE& Range = Parameter.DescriptorTable.pDescriptorRanges[0];

            const uint32 ResourceType     = GetResourceType(Range.RangeType);
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            RootParameterMap[ShaderVisibility].RootParameterIndicies[ResourceType] = static_cast<int8>(Index);
            RootParameterMap[ShaderVisibility].ResourceCount[ResourceType]         = static_cast<int8>(Range.NumDescriptors);
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            CHECK(ConstantRootParameterIndex == -1);
            ConstantRootParameterIndex = Index;
        }
    }
}

bool FD3D12RootSignature::InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateRootSignature(1, BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12RootSignature]: FAILED to Create RootSignature");
        return false;
    }

    return true;
}

bool FD3D12RootSignature::Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob)
{
    TComPtr<ID3DBlob> ErrorBlob;

    HRESULT Result = FDynamicD3D12::D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, OutBlob, &ErrorBlob);
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12RootSignature]: FAILED to Serialize RootSignature. Error=%s", reinterpret_cast<const CHAR*>(ErrorBlob->GetBufferPointer()));
        return false;
    }

    return true;
}


FD3D12RootSignatureManager::FD3D12RootSignatureManager(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , RootSignatures()
    , ResourceLayouts()
{
}

FD3D12RootSignatureManager::~FD3D12RootSignatureManager()
{
    ReleaseAll();
}

bool FD3D12RootSignatureManager::Initialize()
{
    FD3D12RootSignatureLayout SimpleGraphicsKey;
    SimpleGraphicsKey.Type                = ERootSignatureType::Graphics;
    SimpleGraphicsKey.bAllowInputAssembler = true;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;

    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Vertex].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Vertex].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Vertex].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Vertex].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Pixel].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Pixel].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Pixel].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    SimpleGraphicsKey.ResourceCounts[ShaderVisibility_Pixel].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    FD3D12RootSignature* SimpleGraphicsRootSignature = CreateRootSignature(SimpleGraphicsKey);
    if (!SimpleGraphicsRootSignature)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SimpleGraphicsRootSignature->SetName("Default Simple Graphics RootSignature");
    }

    FD3D12RootSignatureLayout GraphicsKey;
    GraphicsKey.Type                = ERootSignatureType::Graphics;
    GraphicsKey.bAllowInputAssembler = true;
    GraphicsKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;

    // NOTE: Skips visibility all, however constants are still visible to all stages
    for (uint32 Index = ShaderVisibility_Vertex; Index < ShaderVisibility_Count; Index++)
    {
        GraphicsKey.ResourceCounts[Index].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
        GraphicsKey.ResourceCounts[Index].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
        GraphicsKey.ResourceCounts[Index].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
        GraphicsKey.ResourceCounts[Index].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;
    }

    FD3D12RootSignature* GraphicsRootSignature = CreateRootSignature(GraphicsKey);
    if (!GraphicsRootSignature)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        GraphicsRootSignature->SetName("Default Graphics RootSignature");
    }

    FD3D12RootSignatureLayout ComputeKey;
    ComputeKey.Type                = ERootSignatureType::Compute;
    ComputeKey.bAllowInputAssembler = false;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants  = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    ComputeKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    FD3D12RootSignature* ComputeRootSignature = CreateRootSignature(ComputeKey);
    if (!ComputeRootSignature)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ComputeRootSignature->SetName("Default Compute RootSignature");
    }

    const FD3D12RayTracingDesc& RayTracingDesc = GetDevice()->GetRayTracingDesc();
    if (!RayTracingDesc.IsSupported())
    {
        return true;
    }

    FD3D12RootSignatureLayout RTGlobalKey;
    RTGlobalKey.Type                = ERootSignatureType::RayTracingGlobal;
    RTGlobalKey.bAllowInputAssembler = false;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Num32BitConstants  = D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_CONSTANT_BUFFER_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;
    RTGlobalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_SAMPLER_STATE_COUNT;

    FD3D12RootSignature* RTGlobalRootSignature = CreateRootSignature(RTGlobalKey);
    if (!RTGlobalRootSignature)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RTGlobalRootSignature->SetName("Default Global RayTracing RootSignature");
    }

    FD3D12RootSignatureLayout RTLocalKey;
    RTLocalKey.Type                = ERootSignatureType::RayTracingLocal;
    RTLocalKey.bAllowInputAssembler = false;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumCBVs     = D3D12_DEFAULT_LOCAL_CONSTANT_BUFFER_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSRVs     = D3D12_DEFAULT_LOCAL_SHADER_RESOURCE_VIEW_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumUAVs     = D3D12_DEFAULT_LOCAL_UNORDERED_ACCESS_VIEW_COUNT;
    RTLocalKey.ResourceCounts[ShaderVisibility_All].Ranges.NumSamplers = D3D12_DEFAULT_LOCAL_SAMPLER_STATE_COUNT;

    FD3D12RootSignature* RTLocalRootSignature = CreateRootSignature(RTLocalKey);
    if (!RTLocalRootSignature)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RTLocalRootSignature->SetName("Default Local RayTracing RootSignature");
    }

    return true;
}

void FD3D12RootSignatureManager::ReleaseAll()
{
    for (FD3D12RootSignatureRef RootSignature : RootSignatures)
    {
        RootSignature.Reset();
    }

    RootSignatures.Clear();
    ResourceLayouts.Clear();
}

FD3D12RootSignature* FD3D12RootSignatureManager::GetOrCreateRootSignature(const FD3D12RootSignatureLayout& ResourceCount)
{
    CHECK(RootSignatures.Size() == ResourceLayouts.Size());

    for (int32 i = 0; i < ResourceLayouts.Size(); i++)
    {
        if (ResourceCount.IsCompatible(ResourceLayouts[i]))
        {
            return RootSignatures[i].Get();
        }
    }

    // Make sure that this root signature can be used by more than one pipeline
    FD3D12RootSignatureLayout NewResourceCount = ResourceCount;
    for (uint32 i = 0; i < ShaderVisibility_Count; i++)
    {
        FShaderResourceCount& Count = NewResourceCount.ResourceCounts[i];
        if (Count.Ranges.NumCBVs > 0)
        {
            Count.Ranges.NumCBVs = FMath::Max<uint8>(Count.Ranges.NumCBVs, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);
        }
        if (Count.Ranges.NumSRVs > 0)
        {
            Count.Ranges.NumSRVs = FMath::Max<uint8>(Count.Ranges.NumSRVs, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
        }
        if (Count.Ranges.NumUAVs > 0)
        {
            Count.Ranges.NumUAVs = FMath::Max<uint8>(Count.Ranges.NumUAVs, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
        }
        if (Count.Ranges.NumSamplers > 0)
        {
            Count.Ranges.NumSamplers = FMath::Max<uint8>(Count.Ranges.NumSamplers, D3D12_DEFAULT_SAMPLER_STATE_COUNT);
        }
    }

    return CreateRootSignature(NewResourceCount);
}

FD3D12RootSignature* FD3D12RootSignatureManager::CreateRootSignature(const FD3D12RootSignatureLayout& ResourceLayout)
{
    FD3D12RootSignatureRef NewRootSignature = new FD3D12RootSignature(GetDevice());
    if (!NewRootSignature->Initialize(ResourceLayout))
    {
        return nullptr;
    }

    D3D12_INFO("Created new root signature");

    RootSignatures.Emplace(NewRootSignature);
    ResourceLayouts.Emplace(ResourceLayout);
    return NewRootSignature.Get();
}