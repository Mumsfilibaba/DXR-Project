#include "Core/Misc/CRC.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12RHI.h"

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

static D3D12_SHADER_VISIBILITY GetD3D12ShaderVisibilityFromShaderStage(EShaderStage Stage)
{
    switch (Stage)
    {
    case EShaderStage::Vertex:   return D3D12_SHADER_VISIBILITY_VERTEX;
    case EShaderStage::Hull:     return D3D12_SHADER_VISIBILITY_HULL;
    case EShaderStage::Domain:   return D3D12_SHADER_VISIBILITY_DOMAIN;
    case EShaderStage::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case EShaderStage::Pixel:    return D3D12_SHADER_VISIBILITY_PIXEL;
    default:                     return D3D12_SHADER_VISIBILITY_ALL;
    }
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

static D3D12_DESCRIPTOR_RANGE_TYPE GetD3D12DescriptorRangeType(EResourceType Type)
{
    switch (Type)
    {
    case ResourceType_CBV:     return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case ResourceType_SRV:     return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case ResourceType_UAV:     return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case ResourceType_Sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    
    default:
        CHECK(false);
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
static D3D12_DESCRIPTOR_RANGE_FLAGS GetDescriptorRangeFlags(EResourceType ResType)
{
#if D3D12_ENABLE_STATIC_DESCRIPTORS
    if (ResType == ResourceType_Sampler)
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    }
    else if (ResType == ResourceType_UAV)
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    }
    else
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    }
#else
    if (ResType == ResourceType_Sampler)
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
    }
    else if (ResType == ResourceType_UAV)
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    }
    else
    {
        return D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    }
#endif
}
#endif

static const EShaderVisibility GRootCBVStagePriority[] =
{
    ShaderVisibility_Vertex,
    ShaderVisibility_Pixel,
    ShaderVisibility_Geometry,
    ShaderVisibility_Hull,
    ShaderVisibility_Domain,
    ShaderVisibility_All,
};

static constexpr uint32 GRootCBVStagePriorityCount = sizeof(GRootCBVStagePriority) / sizeof(GRootCBVStagePriority[0]);

void FD3D12RegisterSet::Insert(uint16 Register)
{
    int32 Low  = 0;
    int32 High = Registers.Size() - 1;

    while (Low <= High)
    {
        const int32 Mid = (Low + High) / 2;
        if (Registers[Mid] == Register)
        {
            return;
        }
        else if (Registers[Mid] < Register)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid - 1;
        }
    }

    Registers.Insert(Low, Register);
}

void FD3D12RegisterSet::Remove(uint16 Register)
{
    int32 Low  = 0;
    int32 High = Registers.Size() - 1;

    while (Low <= High)
    {
        const int32 Mid = (Low + High) / 2;
        if (Registers[Mid] == Register)
        {
            Registers.RemoveAt(Mid);
            return;
        }
        else if (Registers[Mid] < Register)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid - 1;
        }
    }
}

bool FD3D12RegisterSet::Contains(uint16 Register) const
{
    int32 Low  = 0;
    int32 High = Registers.Size() - 1;

    while (Low <= High)
    {
        const int32 Mid = (Low + High) / 2;
        if (Registers[Mid] == Register)
        {
            return true;
        }
        else if (Registers[Mid] < Register)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid - 1;
        }
    }

    return false;
}

bool FD3D12RegisterSet::IsSubsetOf(const FD3D12RegisterSet& Other) const
{
    if (Registers.Size() > Other.Registers.Size())
    {
        return false;
    }

    int32 OtherIdx = 0;
    for (int32 i = 0; i < Registers.Size(); i++)
    {
        while (OtherIdx < Other.Registers.Size() && Other.Registers[OtherIdx] < Registers[i])
        {
            OtherIdx++;
        }

        if (OtherIdx >= Other.Registers.Size() || Other.Registers[OtherIdx] != Registers[i])
        {
            return false;
        }

        OtherIdx++;
    }

    return true;
}

FD3D12RegisterSet FD3D12RegisterSet::Union(const FD3D12RegisterSet& Other) const
{
    FD3D12RegisterSet Result;
    Result.Registers.Reserve(Registers.Size() + Other.Registers.Size());

    int32 i = 0;
    int32 j = 0;

    while (i < Registers.Size() && j < Other.Registers.Size())
    {
        if (Registers[i] < Other.Registers[j])
        {
            Result.Registers.Emplace(Registers[i++]);
        }
        else if (Registers[i] > Other.Registers[j])
        {
            Result.Registers.Emplace(Other.Registers[j++]);
        }
        else
        {
            Result.Registers.Emplace(Registers[i++]);
            j++;
        }
    }

    while (i < Registers.Size())
    {
        Result.Registers.Emplace(Registers[i++]);
    }

    while (j < Other.Registers.Size())
    {
        Result.Registers.Emplace(Other.Registers[j++]);
    }

    return Result;
}

FD3D12RootSignatureLayout::FD3D12RootSignatureLayout()
    : Type(ERootSignatureType::Unknown)
    , bAllowInputAssembler(false)
    , bAllowStreamOutput(false)
    , NumPushConstants(0)
{
}

void FD3D12RootSignatureLayout::AddStaticSampler(const FRHIStaticSamplerInfo& StaticSampler)
{
    StaticSamplers.Emplace(StaticSampler);

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        RegisterSets[Stage][ResourceType_Sampler].Remove(StaticSampler.ShaderRegister);
    }
}

void FD3D12RootSignatureLayout::AddRegister(EShaderVisibility Stage, EResourceType ResType, uint16 Register)
{
    CHECK(Stage < ShaderVisibility_Count);
    CHECK(ResType < ResourceType_Count);
    
    RegisterSets[Stage][ResType].Insert(Register);
}

void FD3D12RootSignatureLayout::AddContiguousRegisters(EShaderVisibility Stage, EResourceType ResType, uint8 Count)
{
    for (uint8 i = 0; i < Count; i++)
    {
        AddRegister(Stage, ResType, i);
    }
}

void FD3D12RootSignatureLayout::ComputeRootCBVs()
{
    uint32 Budget = D3D12_TARGET_ROOT_SIGNATURE_DWORD_COST;

    if (NumPushConstants > 0)
    {
        Budget -= Math::Min<uint32>(Budget, NumPushConstants);
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        for (uint32 ResType = ResourceType_SRV; ResType < ResourceType_Count; ResType++)
        {
            if (!RegisterSets[Stage][ResType].IsEmpty())
            {
                Budget -= Math::Min<uint32>(Budget, 1);
            }
        }
    }

    for (uint32 PriorityIdx = 0; PriorityIdx < GRootCBVStagePriorityCount && Budget >= 2; PriorityIdx++)
    {
        const EShaderVisibility  Stage        = GRootCBVStagePriority[PriorityIdx];
        const FD3D12RegisterSet& CBVRegisters = RegisterSets[Stage][ResourceType_CBV];

        for (int32 RegIdx = 0; RegIdx < static_cast<int32>(CBVRegisters.GetCount()) && Budget >= 2; RegIdx++)
        {
            if (RootCBVSets[Stage].GetCount() >= FD3D12ShaderStage::MaxRootCBVsPerStage)
            {
                break;
            }

            RootCBVSets[Stage].Insert(CBVRegisters.Registers[RegIdx]);
            Budget -= 2;
        }
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        const FD3D12RegisterSet& CBVRegisters = RegisterSets[Stage][ResourceType_CBV];

        bool bHasNonRootCBVs = false;
        for (int32 i = 0; i < static_cast<int32>(CBVRegisters.GetCount()); i++)
        {
            if (!RootCBVSets[Stage].Contains(CBVRegisters.Registers[i]))
            {
                bHasNonRootCBVs = true;
                break;
            }
        }

        if (bHasNonRootCBVs)
        {
            Budget -= Math::Min<uint32>(Budget, 1);
        }
    }
}

uint32 FD3D12RootSignatureLayout::ComputeCost() const
{
    uint32 Cost = NumPushConstants;

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        Cost += RootCBVSets[Stage].GetCount() * 2;

        for (uint32 ResType = 0; ResType < ResourceType_Count; ResType++)
        {
            if (ResType == ResourceType_CBV)
            {
                bool bHasNonRootCBVs = false;

                const FD3D12RegisterSet& CBVRegisters = RegisterSets[Stage][ResType];
                for (int32 i = 0; i < static_cast<int32>(CBVRegisters.GetCount()); i++)
                {
                    if (!RootCBVSets[Stage].Contains(CBVRegisters.Registers[i]))
                    {
                        bHasNonRootCBVs = true;
                        break;
                    }
                }

                if (bHasNonRootCBVs)
                {
                    Cost++;
                }
            }
            else
            {
                if (!RegisterSets[Stage][ResType].IsEmpty())
                {
                    Cost++;
                }
            }
        }
    }

    return Cost;
}

bool FD3D12RootSignatureLayout::IsCompatible(const FD3D12RootSignatureLayout& Other) const
{
    if (Type != Other.Type || bAllowInputAssembler != Other.bAllowInputAssembler || bAllowStreamOutput != Other.bAllowStreamOutput)
    {
        return false;
    }

    if (NumPushConstants > Other.NumPushConstants)
    {
        return false;
    }

    if (StaticSamplers.Size() != Other.StaticSamplers.Size())
    {
        return false;
    }

    for (int32 i = 0; i < StaticSamplers.Size(); ++i)
    {
        if (StaticSamplers[i].ShaderVisibility != Other.StaticSamplers[i].ShaderVisibility || 
            StaticSamplers[i].ShaderRegister != Other.StaticSamplers[i].ShaderRegister)
        {
            return false;
        }
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        for (uint32 ResType = 0; ResType < ResourceType_Count; ResType++)
        {
            if (!RegisterSets[Stage][ResType].IsSubsetOf(Other.RegisterSets[Stage][ResType]))
            {
                return false;
            }
        }

        if (!RootCBVSets[Stage].IsSubsetOf(Other.RootCBVSets[Stage]))
        {
            return false;
        }
    }

    return true;
}

FD3D12DescriptorTableMapping::FD3D12DescriptorTableMapping()
    : NumSlots(0)
{
    FMemory::Memzero(SlotToRegister, sizeof(SlotToRegister));
}

void FD3D12DescriptorTableMapping::Build(const FD3D12RegisterSet& Registers)
{
    NumSlots = static_cast<uint8>(Math::Min<uint32>(Registers.GetCount(), D3D12_CACHED_DESCRIPTORS_COUNT));
    for (uint8 i = 0; i < NumSlots; i++)
    {
        SlotToRegister[i] = Registers.Registers[i];
    }
}

int8 FD3D12DescriptorTableMapping::GetSlotForRegister(uint16 Register) const
{
    int32 Low  = 0;
    int32 High = static_cast<int32>(NumSlots) - 1;

    while (Low <= High)
    {
        const int32 Mid = (Low + High) / 2;
        if (SlotToRegister[Mid] == Register)
        {
            return static_cast<int8>(Mid);
        }
        else if (SlotToRegister[Mid] < Register)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid - 1;
        }
    }
    return -1;
}

uint16 FD3D12DescriptorTableMapping::GetRegisterForSlot(uint8 Slot) const
{
    CHECK(Slot < NumSlots);
    return SlotToRegister[Slot];
}

FD3D12ShaderStage::FD3D12ShaderStage()
    : NumRootCBVs(0)
{
    for (int32 i = 0; i < ResourceType_Count; i++)
    {
        RootParameterIndicies[i] = -1;
        ResourceCounts[i]        = 0;
        NumRootDescriptors[i]    = 0;
    }

    for (int32 i = 0; i < MaxRootCBVsPerStage; i++)
    {
        RootCBVParameterIndex[i] = -1;
        RootCBVRegister[i]       = 0;
    }

    FMemory::Memzero(RootDescriptors, sizeof(RootDescriptors));
}

void FD3D12ShaderStage::SetDescriptorTableIndex(EResourceType Type, int8 RootParameterIndex, int8 DescriptorCount)
{
    RootParameterIndicies[Type] = RootParameterIndex;
    ResourceCounts[Type]        = DescriptorCount;
}

void FD3D12ShaderStage::AddRootCBV(int8 RootParameterIndex, uint16 Register)
{
    CHECK(NumRootCBVs < MaxRootCBVsPerStage);

    RootCBVParameterIndex[NumRootCBVs] = RootParameterIndex;
    RootCBVRegister[NumRootCBVs]       = Register;
    NumRootCBVs++;
}

int8 FD3D12ShaderStage::GetRootCBVParameterIndex(uint16 Register) const
{
    for (uint8 i = 0; i < NumRootCBVs; i++)
    {
        if (RootCBVRegister[i] == Register)
        {
            return RootCBVParameterIndex[i];
        }
    }

    return -1;
}

bool FD3D12ShaderStage::IsRootCBV(uint16 Register) const
{
    return GetRootCBVParameterIndex(Register) != -1;
}

uint16 FD3D12ShaderStage::GetRootCBVRegister(uint8 Index) const
{
    CHECK(Index < NumRootCBVs);
    return RootCBVRegister[Index];
}

int8 FD3D12ShaderStage::GetRootCBVParameterIndexBySlot(uint8 Index) const
{
    CHECK(Index < NumRootCBVs);
    return RootCBVParameterIndex[Index];
}

void FD3D12ShaderStage::AddRootDescriptor(EResourceType Type, int8 RootParameterIndex, uint16 Register)
{
    CHECK(Type < ResourceType_Count);
    CHECK(NumRootDescriptors[Type] < MaxRootDescriptorsPerStage);

    FRootDescriptorEntry& Entry = RootDescriptors[Type][NumRootDescriptors[Type]];
    Entry.ParameterIndex = RootParameterIndex;
    Entry.Register       = Register;
    NumRootDescriptors[Type]++;
}

int8 FD3D12ShaderStage::GetRootDescriptorParameterIndex(EResourceType Type, uint16 Register) const
{
    CHECK(Type < ResourceType_Count);
    for (uint8 i = 0; i < NumRootDescriptors[Type]; i++)
    {
        if (RootDescriptors[Type][i].Register == Register)
        {
            return RootDescriptors[Type][i].ParameterIndex;
        }
    }
    return -1;
}

FD3D12RootSignatureDescHelper::FD3D12RootSignatureDescHelper(const FD3D12RootSignatureLayout& Layout)
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    : VersionedDesc()
#else
    : Desc()
#endif
    , RootParameters()
    , DescriptorRanges()
    , NumRootParameters(0)
    , NumDescriptorRanges(0)
    , RootSignatureCost(0)
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

    D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    const uint32 Space = (Layout.GetType() == ERootSignatureType::RayTracingLocal) ? D3D12_SHADER_REGISTER_SPACE_RT_LOCAL : 0;

    if (Layout.GetNumPushConstants() > 0)
    {
        CHECK(Layout.GetNumPushConstants() <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);
        CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);
        Insert32BitConstantRange(D3D12_SHADER_VISIBILITY_ALL, Layout.GetNumPushConstants(), 0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
    }

    const bool bIsLocalRootSignature = (Layout.GetType() == ERootSignatureType::RayTracingLocal);

    for (uint32 ShaderStage = 0; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        bool bIsStageUsed = false;
        
        const D3D12_SHADER_VISIBILITY D3D12Visibility = GetD3D12ShaderVisibility(ShaderStage);
        if (bIsLocalRootSignature)
        {
            const FD3D12RegisterSet& CBVRegisters = Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResourceType_CBV);
            for (uint32 i = 0; i < CBVRegisters.GetCount(); i++)
            {
                CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);
            #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                InsertRootCBV(D3D12Visibility, CBVRegisters.Registers[i], Space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
            #else
                InsertRootCBV(D3D12Visibility, CBVRegisters.Registers[i], Space);
            #endif
                bIsStageUsed = true;
            }

            const FD3D12RegisterSet& SRVRegisters = Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResourceType_SRV);
            for (uint32 i = 0; i < SRVRegisters.GetCount(); i++)
            {
                CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);
            #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                InsertRootSRV(D3D12Visibility, SRVRegisters.Registers[i], Space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
            #else
                InsertRootSRV(D3D12Visibility, SRVRegisters.Registers[i], Space);
            #endif
                bIsStageUsed = true;
            }

            const FD3D12RegisterSet& UAVRegisters = Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResourceType_UAV);
            for (uint32 i = 0; i < UAVRegisters.GetCount(); i++)
            {
                CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);
            #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                InsertRootUAV(D3D12Visibility, UAVRegisters.Registers[i], Space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
            #else
                InsertRootUAV(D3D12Visibility, UAVRegisters.Registers[i], Space);
            #endif
                bIsStageUsed = true;
            }

            CHECK(Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResourceType_Sampler).IsEmpty());
        }
        else
        {
            const EResourceType DescriptorTableTypes[] = { ResourceType_SRV, ResourceType_UAV, ResourceType_Sampler };
            for (EResourceType ResType : DescriptorTableTypes)
            {
                const FD3D12RegisterSet& Registers = Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResType);
                if (!Registers.IsEmpty())
                {
                    const uint32 RangeStart = NumDescriptorRanges;
            #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                    const uint32 NumRanges = BuildDescriptorRangesForRegisterSet(Registers, GetD3D12DescriptorRangeType(ResType), Space, GetDescriptorRangeFlags(ResType));
            #else
                    const uint32 NumRanges = BuildDescriptorRangesForRegisterSet(Registers, GetD3D12DescriptorRangeType(ResType), Space);
            #endif
                    InsertDescriptorTable(D3D12Visibility, &DescriptorRanges[RangeStart], NumRanges);
                    bIsStageUsed = true;
                }
            }

            const FD3D12RegisterSet& RootCBVRegisters = Layout.GetRootCBVRegisters(static_cast<EShaderVisibility>(ShaderStage));
            for (uint32 i = 0; i < RootCBVRegisters.GetCount(); i++)
            {
                CHECK(NumRootParameters < D3D12_MAX_ROOT_PARAMETERS);
            #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                InsertRootCBV(D3D12Visibility, RootCBVRegisters.Registers[i], Space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
            #else
                InsertRootCBV(D3D12Visibility, RootCBVRegisters.Registers[i], Space);
            #endif
                bIsStageUsed = true;
            }

            const FD3D12RegisterSet& CBVRegisters = Layout.GetRegisters(static_cast<EShaderVisibility>(ShaderStage), ResourceType_CBV);
            FD3D12RegisterSet TableCBVRegisters;
            for (uint32 i = 0; i < CBVRegisters.GetCount(); i++)
            {
                if (!RootCBVRegisters.Contains(CBVRegisters.Registers[i]))
                {
                    TableCBVRegisters.Insert(CBVRegisters.Registers[i]);
                }
            }

            if (!TableCBVRegisters.IsEmpty())
            {
                const uint32 RangeStart = NumDescriptorRanges;
        #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
                const uint32 NumRanges = BuildDescriptorRangesForRegisterSet(TableCBVRegisters, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, Space, GetDescriptorRangeFlags(ResourceType_CBV));
        #else
                const uint32 NumRanges = BuildDescriptorRangesForRegisterSet(TableCBVRegisters, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, Space);
        #endif
                InsertDescriptorTable(D3D12Visibility, &DescriptorRanges[RangeStart], NumRanges);
                bIsStageUsed = true;
            }
        }

        if (!bIsStageUsed && Layout.GetNumPushConstants() == 0)
        {
            Flags |= RootSignatureFlags[ShaderStage];
        }
    }

    CHECK(RootSignatureCost <= D3D12_MAX_ROOT_PARAMETER_COST);

    {
        const uint32 DWordCost = Layout.ComputeCost();

        uint32 NumTables       = 0;
        uint32 NumRootCBVTotal = 0;

        for (uint32 s = 0; s < ShaderVisibility_Count; s++)
        {
            NumRootCBVTotal += Layout.GetRootCBVRegisters(static_cast<EShaderVisibility>(s)).GetCount();
            for (uint32 t = 0; t < ResourceType_Count; t++)
            {
                if (!Layout.GetRegisters(static_cast<EShaderVisibility>(s), static_cast<EResourceType>(t)).IsEmpty())
                {
                    if (t != ResourceType_CBV || !Layout.GetRegisters(static_cast<EShaderVisibility>(s), static_cast<EResourceType>(t)).IsSubsetOf(Layout.GetRootCBVRegisters(static_cast<EShaderVisibility>(s))))
                    {
                        NumTables++;
                    }
                }
            }
        }

        D3D12_INFO("[FD3D12RootSignatureDescHelper] RootSignature: %u DWORDs (%u tables, %u root CBVs, %u push constants)", DWordCost, NumTables, NumRootCBVTotal, Layout.GetNumPushConstants());
    }

    if (Layout.GetAllowInputAssembler())
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (Layout.GetType() == ERootSignatureType::RayTracingLocal)
    {
        Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    }

    if (Layout.GetAllowStreamOutput())
    {
        Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
    }

    NumStaticSamplers = 0;

    const auto& LayoutStaticSamplers = Layout.GetStaticSamplers();
    for (int32 i = 0; i < LayoutStaticSamplers.Size() && NumStaticSamplers < 16; ++i)
    {
        const FRHIStaticSamplerInfo& Entry = LayoutStaticSamplers[i];
        D3D12_STATIC_SAMPLER_DESC& Desc = StaticSamplers[NumStaticSamplers];
        Desc.Filter           = ConvertSamplerFilter(Entry.Filter);
        Desc.AddressU         = ConvertSamplerMode(Entry.AddressU);
        Desc.AddressV         = ConvertSamplerMode(Entry.AddressV);
        Desc.AddressW         = ConvertSamplerMode(Entry.AddressW);
        Desc.MipLODBias       = Entry.MipLODBias;
        Desc.MaxAnisotropy    = Entry.MaxAnisotropy;
        Desc.ComparisonFunc   = ConvertComparisonFunc(Entry.ComparisonFunc);
        Desc.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        Desc.MinLOD           = Entry.MinLOD;
        Desc.MaxLOD           = Entry.MaxLOD;
        Desc.ShaderRegister   = Entry.ShaderRegister;
        Desc.RegisterSpace    = 0;
        Desc.ShaderVisibility = GetD3D12ShaderVisibilityFromShaderStage(Entry.ShaderVisibility);

    #if D3D12_USE_VERSIONED_ROOT_SIGNATURES
        D3D12_STATIC_SAMPLER_DESC1& Desc1 = StaticSamplers1[NumStaticSamplers];
        Desc1.Filter           = Desc.Filter;
        Desc1.AddressU         = Desc.AddressU;
        Desc1.AddressV         = Desc.AddressV;
        Desc1.AddressW         = Desc.AddressW;
        Desc1.MipLODBias       = Desc.MipLODBias;
        Desc1.MaxAnisotropy    = Desc.MaxAnisotropy;
        Desc1.ComparisonFunc   = Desc.ComparisonFunc;
        Desc1.BorderColor      = Desc.BorderColor;
        Desc1.MinLOD           = Desc.MinLOD;
        Desc1.MaxLOD           = Desc.MaxLOD;
        Desc1.ShaderRegister   = Desc.ShaderRegister;
        Desc1.RegisterSpace    = Desc.RegisterSpace;
        Desc1.ShaderVisibility = Desc.ShaderVisibility;
        Desc1.Flags            = D3D12_SAMPLER_FLAG_NONE;
    #endif

        NumStaticSamplers++;
    }

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    if (GD3D12RootSignatureVersion >= D3D_ROOT_SIGNATURE_VERSION_1_2)
    {
        VersionedDesc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_2;
        VersionedDesc.Desc_1_2.NumParameters     = NumRootParameters;
        VersionedDesc.Desc_1_2.pParameters       = RootParameters;
        VersionedDesc.Desc_1_2.NumStaticSamplers = NumStaticSamplers;
        VersionedDesc.Desc_1_2.pStaticSamplers   = NumStaticSamplers > 0 ? StaticSamplers1 : nullptr;
        VersionedDesc.Desc_1_2.Flags             = Flags;
    }
    else
    {
        VersionedDesc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
        VersionedDesc.Desc_1_1.NumParameters     = NumRootParameters;
        VersionedDesc.Desc_1_1.pParameters       = RootParameters;
        VersionedDesc.Desc_1_1.NumStaticSamplers = NumStaticSamplers;
        VersionedDesc.Desc_1_1.pStaticSamplers   = NumStaticSamplers > 0 ? StaticSamplers : nullptr;
        VersionedDesc.Desc_1_1.Flags             = Flags;
    }
#else
    Desc.NumParameters     = NumRootParameters;
    Desc.pParameters       = RootParameters;
    Desc.NumStaticSamplers = NumStaticSamplers;
    Desc.pStaticSamplers   = NumStaticSamplers > 0 ? StaticSamplers : nullptr;
    Desc.Flags             = Flags;
#endif
}

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
uint32 FD3D12RootSignatureDescHelper::BuildDescriptorRangesForRegisterSet(const FD3D12RegisterSet& Registers, D3D12_DESCRIPTOR_RANGE_TYPE RangeType, uint32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS RangeFlags)
{
    uint32 NumRangesCreated = 0;
    uint32 DescriptorOffset = 0;

    int32 i = 0;
    while (i < static_cast<int32>(Registers.GetCount()))
    {
        const uint16 RangeStart = Registers.Registers[i];
        
        uint16 RangeEnd = RangeStart;
        while (i + 1 < static_cast<int32>(Registers.GetCount()) && Registers.Registers[i + 1] == RangeEnd + 1)
        {
            RangeEnd++;
            i++;
        }

        const uint32 NumDescriptors = RangeEnd - RangeStart + 1;
        CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGE_SIZE);

        InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], RangeType, NumDescriptors, RangeStart, Space, RangeFlags, DescriptorOffset);

        NumDescriptorRanges++;
        NumRangesCreated++;
        DescriptorOffset += NumDescriptors;

        i++;
    }

    return NumRangesCreated;
}

void FD3D12RootSignatureDescHelper::InitDescriptorRange(D3D12_DESCRIPTOR_RANGE1& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace, D3D12_DESCRIPTOR_RANGE_FLAGS RangeFlags, uint32 OffsetInTable)
{
    CHECK(NumDescriptors > 0);

    OutRange.BaseShaderRegister                = BaseShaderRegister;
    OutRange.NumDescriptors                    = NumDescriptors;
    OutRange.RangeType                         = Type;
    OutRange.RegisterSpace                     = RegisterSpace;
    OutRange.Flags                             = RangeFlags;
    OutRange.OffsetInDescriptorsFromTableStart = OffsetInTable;
}

void FD3D12RootSignatureDescHelper::InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE1* InDescriptorRanges, uint32 InNumDescriptorRanges)
{
    D3D12_ROOT_PARAMETER1& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    NewParameters.ShaderVisibility                    = ShaderVisibility;
    NewParameters.DescriptorTable.NumDescriptorRanges = InNumDescriptorRanges;
    NewParameters.DescriptorTable.pDescriptorRanges   = InDescriptorRanges;

    RootSignatureCost++;
}

void FD3D12RootSignatureDescHelper::InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS DescFlags)
{
    D3D12_ROOT_PARAMETER1& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;
    NewParameters.Descriptor.Flags          = DescFlags;

    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS DescFlags)
{
    D3D12_ROOT_PARAMETER1& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;
    NewParameters.Descriptor.Flags          = DescFlags;

    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS DescFlags)
{
    D3D12_ROOT_PARAMETER1& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;
    NewParameters.Descriptor.Flags          = DescFlags;

    RootSignatureCost += 2;
}
#else // !D3D12_USE_VERSIONED_ROOT_SIGNATURES
uint32 FD3D12RootSignatureDescHelper::BuildDescriptorRangesForRegisterSet(const FD3D12RegisterSet& Registers, D3D12_DESCRIPTOR_RANGE_TYPE RangeType, uint32 Space)
{
    uint32 NumRangesCreated = 0;
    uint32 DescriptorOffset = 0;

    int32 i = 0;
    while (i < static_cast<int32>(Registers.GetCount()))
    {
        const uint16 RangeStart = Registers.Registers[i];
        
        uint16 RangeEnd = RangeStart;
        while (i + 1 < static_cast<int32>(Registers.GetCount()) && Registers.Registers[i + 1] == RangeEnd + 1)
        {
            RangeEnd++;
            i++;
        }

        const uint32 NumDescriptors = RangeEnd - RangeStart + 1;
        CHECK(NumDescriptorRanges < D3D12_MAX_DESCRIPTOR_RANGE_SIZE);

        InitDescriptorRange(DescriptorRanges[NumDescriptorRanges], RangeType, NumDescriptors, RangeStart, Space, DescriptorOffset);

        NumDescriptorRanges++;
        NumRangesCreated++;
        DescriptorOffset += NumDescriptors;

        i++;
    }

    return NumRangesCreated;
}

void FD3D12RootSignatureDescHelper::InitDescriptorRange(D3D12_DESCRIPTOR_RANGE& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace, uint32 OffsetInTable)
{
    CHECK(NumDescriptors > 0);

    OutRange.BaseShaderRegister                = BaseShaderRegister;
    OutRange.NumDescriptors                    = NumDescriptors;
    OutRange.RangeType                         = Type;
    OutRange.RegisterSpace                     = RegisterSpace;
    OutRange.OffsetInDescriptorsFromTableStart = OffsetInTable;
}

void FD3D12RootSignatureDescHelper::InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE* InDescriptorRanges, uint32 InNumDescriptorRanges)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    NewParameters.ShaderVisibility                    = ShaderVisibility;
    NewParameters.DescriptorTable.NumDescriptorRanges = InNumDescriptorRanges;
    NewParameters.DescriptorTable.pDescriptorRanges   = InDescriptorRanges;

    RootSignatureCost++;
}

void FD3D12RootSignatureDescHelper::InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    RootSignatureCost += 2;
}

void FD3D12RootSignatureDescHelper::InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace)
{
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
    NewParameters.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    NewParameters.ShaderVisibility          = ShaderVisibility;
    NewParameters.Descriptor.ShaderRegister = ShaderRegister;
    NewParameters.Descriptor.RegisterSpace  = RegisterSpace;

    RootSignatureCost += 2;
}
#endif // D3D12_USE_VERSIONED_ROOT_SIGNATURES

void FD3D12RootSignatureDescHelper::Insert32BitConstantRange(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 NumShaderConstants, uint32 ShaderRegister, uint32 RegisterSpace)
{
    CHECK(NumShaderConstants > 0);

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    D3D12_ROOT_PARAMETER1& NewParameters = RootParameters[NumRootParameters++];
#else
    D3D12_ROOT_PARAMETER& NewParameters = RootParameters[NumRootParameters++];
#endif
    NewParameters.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    NewParameters.ShaderVisibility         = ShaderVisibility;
    NewParameters.Constants.Num32BitValues = NumShaderConstants;
    NewParameters.Constants.ShaderRegister = ShaderRegister;
    NewParameters.Constants.RegisterSpace  = RegisterSpace;

    RootSignatureCost += NumShaderConstants;
}

bool FD3D12RootSignature::HasDenyFlag(EShaderVisibility Stage) const
{
    static const D3D12_ROOT_SIGNATURE_FLAGS DenyFlags[ShaderVisibility_Count] =
    {
        D3D12_ROOT_SIGNATURE_FLAG_NONE,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
    };

    CHECK(Stage < ShaderVisibility_Count);
    return (Flags & DenyFlags[Stage]) != 0;
}

FD3D12RootSignature::FD3D12RootSignature(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , RootSignature(nullptr)
    , ShaderStages()
    , TableMappings()
    , ConstantRootParameterIndex(-1)
    , Num32BitConstants(0)
    , Flags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
    , Hash(0)
{
}

bool FD3D12RootSignature::Initialize(const FD3D12RootSignatureLayout& Layout)
{
    FD3D12RootSignatureDescHelper DescHelper(Layout);
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    if (!Initialize(DescHelper.GetVersionedDesc()))
#else
    if (!Initialize(DescHelper.GetDesc()))
#endif
    {
        return false;
    }

    if (Layout.GetType() == ERootSignatureType::RayTracingLocal)
    {
        return true;
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        for (uint32 ResType = 0; ResType < ResourceType_Count; ResType++)
        {
            const FD3D12RegisterSet& Registers = Layout.GetRegisters(static_cast<EShaderVisibility>(Stage), static_cast<EResourceType>(ResType));
            if (ResType == ResourceType_CBV)
            {
                FD3D12RegisterSet TableRegisters;

                const FD3D12RegisterSet& RootCBVs = Layout.GetRootCBVRegisters(static_cast<EShaderVisibility>(Stage));
                for (uint32 i = 0; i < Registers.GetCount(); i++)
                {
                    if (!RootCBVs.Contains(Registers.Registers[i]))
                    {
                        TableRegisters.Insert(Registers.Registers[i]);
                    }
                }

                TableMappings[Stage][ResType].Build(TableRegisters);
            }
            else
            {
                TableMappings[Stage][ResType].Build(Registers);
            }
        }
    }

    return true;
}

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES

bool FD3D12RootSignature::Initialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc)
{
    TComPtr<ID3DBlob> SignatureBlob;

    if (!Serialize(Desc, &SignatureBlob))
    {
        return false;
    }

    const D3D12_ROOT_PARAMETER1* Parameters    = nullptr;
    uint32                       NumParameters = 0;

    if (Desc.Version == D3D_ROOT_SIGNATURE_VERSION_1_2)
    {
        Flags         = Desc.Desc_1_2.Flags;
        Parameters    = Desc.Desc_1_2.pParameters;
        NumParameters = Desc.Desc_1_2.NumParameters;
    }
    else
    {
        Flags         = Desc.Desc_1_1.Flags;
        Parameters    = Desc.Desc_1_1.pParameters;
        NumParameters = Desc.Desc_1_1.NumParameters;
    }

    InternalInitRootParameterMap(Parameters, NumParameters);

    return InternalInit(SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize());
}

bool FD3D12RootSignature::Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    TComPtr<ID3D12VersionedRootSignatureDeserializer> Deserializer;
    HRESULT Result = D3D12Functions::D3D12CreateVersionedRootSignatureDeserializer(BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&Deserializer));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Retrieve Versioned Root Signature Desc");
        return false;
    }

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* VersionedDesc = nullptr;
    Result = Deserializer->GetRootSignatureDescAtVersion(GD3D12RootSignatureVersion, &VersionedDesc);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to get Root Signature desc at version");
        return false;
    }

    CHECK(VersionedDesc != nullptr);

    const D3D12_ROOT_PARAMETER1* Parameters    = nullptr;
    uint32                       NumParameters = 0;

    if (VersionedDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_2)
    {
        Flags         = VersionedDesc->Desc_1_2.Flags;
        Parameters    = VersionedDesc->Desc_1_2.pParameters;
        NumParameters = VersionedDesc->Desc_1_2.NumParameters;
    }
    else
    {
        Flags         = VersionedDesc->Desc_1_1.Flags;
        Parameters    = VersionedDesc->Desc_1_1.pParameters;
        NumParameters = VersionedDesc->Desc_1_1.NumParameters;
    }

    InternalInitRootParameterMap(Parameters, NumParameters);
    InternalInitTableMappingsFromDesc(Parameters, NumParameters);

    return InternalInit(BlobWithRootSignature, BlobLengthInBytes);
}

#else // !D3D12_USE_VERSIONED_ROOT_SIGNATURES

bool FD3D12RootSignature::Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    TComPtr<ID3DBlob> SignatureBlob;

    if (!Serialize(Desc, &SignatureBlob))
    {
        return false;
    }

    Flags = Desc.Flags;
    InternalInitRootParameterMap(Desc);

    return InternalInit(SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize());
}

bool FD3D12RootSignature::Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    TComPtr<ID3D12RootSignatureDeserializer> Deserializer;
    HRESULT Result = D3D12Functions::D3D12CreateRootSignatureDeserializer(BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&Deserializer));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Retrieve Root Signature Desc");
        return false;
    }

    const D3D12_ROOT_SIGNATURE_DESC* Desc = Deserializer->GetRootSignatureDesc();
    CHECK(Desc != nullptr);

    Flags = Desc->Flags;
    InternalInitRootParameterMap(*Desc);
    InternalInitTableMappingsFromDesc(*Desc);

    TComPtr<ID3DBlob> Blob;
    if (!Serialize(*Desc, &Blob))
    {
        return false;
    }

    Result = GetDevice()->GetD3D12Device()->CreateRootSignature(1, Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Create RootSignature");
        return false;
    }

    return InternalInit(BlobWithRootSignature, BlobLengthInBytes);
}

#endif // D3D12_USE_VERSIONED_ROOT_SIGNATURES

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES

void FD3D12RootSignature::InternalInitRootParameterMap(const D3D12_ROOT_PARAMETER1* Parameters, uint32 NumParameters)
{
    for (uint32 Index = 0; Index < NumParameters; Index++)
    {
        const D3D12_ROOT_PARAMETER1& Parameter = Parameters[Index];
        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            uint32 TotalDescriptors = 0;

            D3D12_DESCRIPTOR_RANGE_TYPE TableRangeType = Parameter.DescriptorTable.pDescriptorRanges[0].RangeType;
            for (uint32 RangeIdx = 0; RangeIdx < Parameter.DescriptorTable.NumDescriptorRanges; RangeIdx++)
            {
                TotalDescriptors += Parameter.DescriptorTable.pDescriptorRanges[RangeIdx].NumDescriptors;
            }

            const uint32 ResourceType     = GetResourceType(TableRangeType);
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].SetDescriptorTableIndex(static_cast<EResourceType>(ResourceType), static_cast<int8>(Index), static_cast<int8>(Math::Min<uint32>(TotalDescriptors, 127)));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            CHECK(ConstantRootParameterIndex == -1);
            ConstantRootParameterIndex = Index;
            Num32BitConstants          = Parameter.Constants.Num32BitValues;
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootCBV(static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_CBV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_SRV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_UAV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
    }
}

void FD3D12RootSignature::InternalInitTableMappingsFromDesc(const D3D12_ROOT_PARAMETER1* Parameters, uint32 NumParameters)
{
    FD3D12RegisterSet RegisterSets[ShaderVisibility_Count][ResourceType_Count];
    FD3D12RegisterSet RootCBVRegisters[ShaderVisibility_Count];

    for (uint32 Index = 0; Index < NumParameters; Index++)
    {
        const D3D12_ROOT_PARAMETER1& Parameter = Parameters[Index];
        const uint32 Stage = GetShaderVisibility(Parameter.ShaderVisibility);

        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            for (uint32 RangeIdx = 0; RangeIdx < Parameter.DescriptorTable.NumDescriptorRanges; RangeIdx++)
            {
                const D3D12_DESCRIPTOR_RANGE1& Range = Parameter.DescriptorTable.pDescriptorRanges[RangeIdx];
                const uint32 ResType = GetResourceType(Range.RangeType);

                for (uint32 Reg = Range.BaseShaderRegister; Reg < Range.BaseShaderRegister + Range.NumDescriptors; Reg++)
                {
                    RegisterSets[Stage][ResType].Insert(static_cast<uint16>(Reg));
                }
            }
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            RootCBVRegisters[Stage].Insert(static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        for (uint32 ResType = 0; ResType < ResourceType_Count; ResType++)
        {
            if (ResType == ResourceType_CBV)
            {
                FD3D12RegisterSet TableRegisters;
                const FD3D12RegisterSet& Registers = RegisterSets[Stage][ResType];

                for (uint32 i = 0; i < Registers.GetCount(); i++)
                {
                    if (!RootCBVRegisters[Stage].Contains(Registers.Registers[i]))
                    {
                        TableRegisters.Insert(Registers.Registers[i]);
                    }
                }

                TableMappings[Stage][ResType].Build(TableRegisters);
            }
            else
            {
                TableMappings[Stage][ResType].Build(RegisterSets[Stage][ResType]);
            }
        }
    }
}

#else // !D3D12_USE_VERSIONED_ROOT_SIGNATURES

void FD3D12RootSignature::InternalInitRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    for (uint32 Index = 0; Index < Desc.NumParameters; Index++)
    {
        const D3D12_ROOT_PARAMETER& Parameter = Desc.pParameters[Index];
        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            uint32 TotalDescriptors = 0;

            D3D12_DESCRIPTOR_RANGE_TYPE TableRangeType = Parameter.DescriptorTable.pDescriptorRanges[0].RangeType;
            for (uint32 RangeIdx = 0; RangeIdx < Parameter.DescriptorTable.NumDescriptorRanges; RangeIdx++)
            {
                TotalDescriptors += Parameter.DescriptorTable.pDescriptorRanges[RangeIdx].NumDescriptors;
            }

            const uint32 ResourceType     = GetResourceType(TableRangeType);
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].SetDescriptorTableIndex(static_cast<EResourceType>(ResourceType), static_cast<int8>(Index), static_cast<int8>(Math::Min<uint32>(TotalDescriptors, 127)));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            CHECK(ConstantRootParameterIndex == -1);
            ConstantRootParameterIndex = Index;
            Num32BitConstants          = Parameter.Constants.Num32BitValues;
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootCBV(static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_CBV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_SRV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
        {
            const uint32 ShaderVisibility = GetShaderVisibility(Parameter.ShaderVisibility);
            ShaderStages[ShaderVisibility].AddRootDescriptor(ResourceType_UAV, static_cast<int8>(Index), static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
    }
}

void FD3D12RootSignature::InternalInitTableMappingsFromDesc(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
    FD3D12RegisterSet RegisterSets[ShaderVisibility_Count][ResourceType_Count];
    FD3D12RegisterSet RootCBVRegisters[ShaderVisibility_Count];

    for (uint32 Index = 0; Index < Desc.NumParameters; Index++)
    {
        const D3D12_ROOT_PARAMETER& Parameter = Desc.pParameters[Index];
        const uint32 Stage = GetShaderVisibility(Parameter.ShaderVisibility);

        if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            for (uint32 RangeIdx = 0; RangeIdx < Parameter.DescriptorTable.NumDescriptorRanges; RangeIdx++)
            {
                const D3D12_DESCRIPTOR_RANGE& Range = Parameter.DescriptorTable.pDescriptorRanges[RangeIdx];
                const uint32 ResType = GetResourceType(Range.RangeType);

                for (uint32 Reg = Range.BaseShaderRegister; Reg < Range.BaseShaderRegister + Range.NumDescriptors; Reg++)
                {
                    RegisterSets[Stage][ResType].Insert(static_cast<uint16>(Reg));
                }
            }
        }
        else if (Parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            RootCBVRegisters[Stage].Insert(static_cast<uint16>(Parameter.Descriptor.ShaderRegister));
        }
    }

    for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
    {
        for (uint32 ResType = 0; ResType < ResourceType_Count; ResType++)
        {
            if (ResType == ResourceType_CBV)
            {
                FD3D12RegisterSet TableRegisters;
                const FD3D12RegisterSet& Registers = RegisterSets[Stage][ResType];

                for (uint32 i = 0; i < Registers.GetCount(); i++)
                {
                    if (!RootCBVRegisters[Stage].Contains(Registers.Registers[i]))
                    {
                        TableRegisters.Insert(Registers.Registers[i]);
                    }
                }

                TableMappings[Stage][ResType].Build(TableRegisters);
            }
            else
            {
                TableMappings[Stage][ResType].Build(RegisterSets[Stage][ResType]);
            }
        }
    }
}

#endif // D3D12_USE_VERSIONED_ROOT_SIGNATURES

bool FD3D12RootSignature::InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateRootSignature(1, BlobWithRootSignature, BlobLengthInBytes, IID_PPV_ARGS(&RootSignature));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Create RootSignature");
        return false;
    }
    else
    {
        Hash = CRC32::Generate(BlobWithRootSignature, BlobLengthInBytes);
        return true;
    }
}

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
bool FD3D12RootSignature::Serialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob)
{
    TComPtr<ID3DBlob> ErrorBlob;

    HRESULT Result = D3D12Functions::D3D12SerializeVersionedRootSignature(&Desc, OutBlob, &ErrorBlob);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Serialize Versioned RootSignature. Error=%s", reinterpret_cast<const CHAR*>(ErrorBlob->GetBufferPointer()));
        return false;
    }

    return true;
}
#else
bool FD3D12RootSignature::Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob)
{
    TComPtr<ID3DBlob> ErrorBlob;

    HRESULT Result = D3D12Functions::D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, OutBlob, &ErrorBlob);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RootSignature]: FAILED to Serialize RootSignature. Error=%s", reinterpret_cast<const CHAR*>(ErrorBlob->GetBufferPointer()));
        return false;
    }

    return true;
}
#endif

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

void FD3D12RootSignatureManager::ReleaseAll()
{
    for (FD3D12RootSignatureRef RootSignature : RootSignatures)
    {
        RootSignature.Reset();
    }

    RootSignatures.Clear();
    ResourceLayouts.Clear();
}

FD3D12RootSignature* FD3D12RootSignatureManager::GetOrCreateRootSignature(const FD3D12RootSignatureLayout& Layout)
{
    CHECK(RootSignatures.Size() == ResourceLayouts.Size());

    for (int32 i = 0; i < ResourceLayouts.Size(); i++)
    {
        if (Layout.IsCompatible(ResourceLayouts[i]))
        {
            return RootSignatures[i].Get();
        }
    }

    return CreateRootSignature(Layout);
}

FD3D12RootSignature* FD3D12RootSignatureManager::CreateRootSignature(const FD3D12RootSignatureLayout& Layout)
{
    FD3D12RootSignatureRef NewRootSignature = new FD3D12RootSignature(GetDevice());
    if (!NewRootSignature->Initialize(Layout))
    {
        return nullptr;
    }

    D3D12_INFO("Created new root signature");

    RootSignatures.Emplace(NewRootSignature);
    ResourceLayouts.Emplace(Layout);
    return NewRootSignature.Get();
}
