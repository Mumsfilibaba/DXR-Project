#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Containers/Map.h"
#include "RHI/RHISamplerState.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Shader.h"

typedef TSharedRef<class FD3D12RootSignature> FD3D12RootSignatureRef;

enum class ERootSignatureType : uint8
{
    Unknown          = 0,
    Graphics         = 1,
    Compute          = 2,
    RayTracingGlobal = 3,
    RayTracingLocal  = 4,
};

struct FD3D12RegisterSet
{
    FD3D12RegisterSet Union(const FD3D12RegisterSet& Other) const;
    
    void Insert(uint16 Register);
    void Remove(uint16 Register);

    bool IsSubsetOf(const FD3D12RegisterSet& Other) const;
    bool Contains(uint16 Register) const;
 
    FORCEINLINE uint32 GetCount() const
    {
        return Registers.Size();
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Registers.IsEmpty();
    }
    
    TArray<uint16> Registers;
};

class FD3D12RootSignatureLayout
{
public:
    FD3D12RootSignatureLayout();
    ~FD3D12RootSignatureLayout();

    void AddRegister(EShaderVisibility::Type Stage, EResourceType::Type ResType, uint16 Register);
    void AddContiguousRegisters(EShaderVisibility::Type Stage, EResourceType::Type ResType, uint8 Count);
    void AddStaticSampler(const FRHIStaticSamplerInfo& StaticSampler);
    void AddLocalTableSRVRegister(EShaderVisibility::Type Stage, uint16 Register);
    void AddLocalTableUAVRegister(EShaderVisibility::Type Stage, uint16 Register);
    void AddLocalTableSamplerRegister(EShaderVisibility::Type Stage, uint16 Register);

    void   ComputeRootCBVs();
    uint32 ComputeCost() const;

    bool IsCompatible(const FD3D12RootSignatureLayout& Other) const;
    
    FORCEINLINE const FD3D12RegisterSet& GetRootCBVRegisters(EShaderVisibility::Type Stage) const
    {
        return RootCBVSets[Stage];
    }

    FORCEINLINE const FD3D12RegisterSet& GetRegisters(EShaderVisibility::Type Stage, EResourceType::Type ResType) const
    {
        return RegisterSets[Stage][ResType];
    }

    FORCEINLINE const FD3D12RegisterSet& GetLocalTableSRVRegisters(EShaderVisibility::Type Stage) const
    {
        return LocalTableSRVSets[Stage];
    }

    FORCEINLINE const FD3D12RegisterSet& GetLocalTableUAVRegisters(EShaderVisibility::Type Stage) const
    {
        return LocalTableUAVSets[Stage];
    }

    FORCEINLINE const FD3D12RegisterSet& GetLocalTableSamplerRegisters(EShaderVisibility::Type Stage) const
    {
        return LocalTableSamplerSets[Stage];
    }

    FORCEINLINE void SetType(ERootSignatureType InType) 
    {
        Type = InType;
    }

    FORCEINLINE void SetAllowInputAssembler(bool bInAllowInputAssembler)
    {
        bAllowInputAssembler = bInAllowInputAssembler;
    }

    FORCEINLINE void SetNumPushConstants(uint8 Count)
    {
        NumPushConstants = Count;
    }

    FORCEINLINE void SetAllowStreamOutput(bool bInAllowStreamOutput)
    {
        bAllowStreamOutput = bInAllowStreamOutput;
    }

    FORCEINLINE void SetDirectlyIndexedResourceHeap(bool bInDirectlyIndexed)
    {
        bDirectlyIndexedResourceHeap = bInDirectlyIndexed;
    }

    FORCEINLINE void SetDirectlyIndexedSamplerHeap(bool bInDirectlyIndexed)
    {
        bDirectlyIndexedSamplerHeap = bInDirectlyIndexed;
    }

    FORCEINLINE ERootSignatureType GetType() const
    {
        return Type;
    }
    
    FORCEINLINE bool GetAllowInputAssembler() const
    {
        return bAllowInputAssembler;
    }

    FORCEINLINE bool GetAllowStreamOutput() const
    {
        return bAllowStreamOutput;
    }

    FORCEINLINE bool GetDirectlyIndexedResourceHeap() const
    {
        return bDirectlyIndexedResourceHeap;
    }

    FORCEINLINE bool GetDirectlyIndexedSamplerHeap() const
    {
        return bDirectlyIndexedSamplerHeap;
    }
    
    FORCEINLINE uint8 GetNumPushConstants() const
    {
        return NumPushConstants;
    }

    FORCEINLINE const TArray<FRHIStaticSamplerInfo>& GetStaticSamplers() const
    {
        return StaticSamplers;
    }

private:
    FD3D12RegisterSet             RegisterSets[EShaderVisibility::Count][EResourceType::Count];
    FD3D12RegisterSet             RootCBVSets[EShaderVisibility::Count];
    FD3D12RegisterSet             LocalTableSRVSets[EShaderVisibility::Count];
    FD3D12RegisterSet             LocalTableUAVSets[EShaderVisibility::Count];
    FD3D12RegisterSet             LocalTableSamplerSets[EShaderVisibility::Count];
    TArray<FRHIStaticSamplerInfo> StaticSamplers;
    uint8                         NumPushConstants;
    bool                          bAllowInputAssembler;
    bool                          bAllowStreamOutput;
    bool                          bDirectlyIndexedResourceHeap;
    bool                          bDirectlyIndexedSamplerHeap;
    ERootSignatureType            Type;
};

class FD3D12DescriptorTableMapping
{
public:
    FD3D12DescriptorTableMapping();
    ~FD3D12DescriptorTableMapping();

    void Build(const FD3D12RegisterSet& Registers);
    
    int8   GetSlotForRegister(uint16 Register) const;
    uint16 GetRegisterForSlot(uint8 Slot)      const;

    FORCEINLINE uint8 GetNumSlots() const
    {
        return NumSlots;
    }

private:
    uint16 SlotToRegister[D3D12_CACHED_DESCRIPTORS_COUNT];
    uint8  NumSlots;
};

class FD3D12RootSignatureDescHelper
{
public:
    FD3D12RootSignatureDescHelper(const FD3D12RootSignatureLayout& Layout);
    ~FD3D12RootSignatureDescHelper();

    FORCEINLINE uint32 GetRootSignatureCost() const
    {
        return RootSignatureCost;
    }

#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    FORCEINLINE const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& GetVersionedDesc() const
    {
        return VersionedDesc;
    }
#else
    FORCEINLINE const D3D12_ROOT_SIGNATURE_DESC& GetDesc() const
    {
        return Desc;
    }
#endif

private:
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    static void InitDescriptorRange(D3D12_DESCRIPTOR_RANGE1& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace, D3D12_DESCRIPTOR_RANGE_FLAGS Flags, uint32 OffsetInTable);
#else
    static void InitDescriptorRange(D3D12_DESCRIPTOR_RANGE& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace, uint32 OffsetInTable);
#endif

private:
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    void InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE1* DescriptorRanges, uint32 NumDescriptorRanges);
    void InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS Flags);
    void InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS Flags);
    void InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace, D3D12_ROOT_DESCRIPTOR_FLAGS Flags);
    uint32 BuildDescriptorRangesForRegisterSet(const FD3D12RegisterSet& Registers, D3D12_DESCRIPTOR_RANGE_TYPE RangeType, uint32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags);
#else
    void InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE* DescriptorRanges, uint32 NumDescriptorRanges);
    void InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);
    void InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);
    void InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);
    uint32 BuildDescriptorRangesForRegisterSet(const FD3D12RegisterSet& Registers, D3D12_DESCRIPTOR_RANGE_TYPE RangeType, uint32 Space);
#endif
    void Insert32BitConstantRange(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 NumShaderConstants, uint32 ShaderRegister, uint32 RegisterSpace);

private:
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    D3D12_ROOT_PARAMETER1                RootParameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE1              DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGE_SIZE];
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC  VersionedDesc;
#else
    D3D12_ROOT_PARAMETER                 RootParameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE               DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGE_SIZE];
    D3D12_ROOT_SIGNATURE_DESC            Desc;
#endif
    uint32                               NumRootParameters;
    uint32                               NumDescriptorRanges;
    uint32                               RootSignatureCost;
    D3D12_STATIC_SAMPLER_DESC            StaticSamplers[16];
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    D3D12_STATIC_SAMPLER_DESC1           StaticSamplers1[16];
#endif
    uint32                               NumStaticSamplers;
};

class FD3D12ShaderStage
{
    struct FRootDescriptorEntry
    {
        uint16 Register;
        int8   ParameterIndex;
    };

public:
    static constexpr int32 MaxRootCBVsPerStage        = 6;
    static constexpr int32 MaxRootDescriptorsPerStage = D3D12_MAX_LOCAL_ROOT_DESCRIPTORS;

public:
    FD3D12ShaderStage();
    ~FD3D12ShaderStage();

    void AddRootDescriptor(EResourceType::Type Type, int8 RootParameterIndex, uint16 Register);
    void AddRootCBV(int8 RootParameterIndex, uint16 Register);
    
    bool IsRootCBV(uint16 Register) const;

    void SetDescriptorTableIndex(EResourceType::Type Type, int8 RootParameterIndex, int8 DescriptorCount);

    int8   GetRootCBVParameterIndex(uint16 Register)   const;
    int8   GetRootCBVParameterIndexBySlot(uint8 Index) const;
    uint16 GetRootCBVRegister(uint8 Index)             const;
    int8   GetRootDescriptorParameterIndex(EResourceType::Type Type, uint16 Register) const;
    
    FORCEINLINE int8 GetRootParameterIndex(EResourceType::Type Type) const
    {
        return RootParameterIndicies[Type];
    }

    FORCEINLINE int8 GetNumResources(EResourceType::Type Type) const
    {
        return ResourceCounts[Type];
    }

    FORCEINLINE uint8 GetNumRootCBVs() const
    {
        return NumRootCBVs;
    }
    
    FORCEINLINE uint8 GetNumRootDescriptors(EResourceType::Type Type) const
    {
        return NumRootDescriptors[Type];
    }

private:
    int8                 RootParameterIndicies[EResourceType::Count];
    int8                 ResourceCounts[EResourceType::Count];
    int8                 RootCBVParameterIndex[MaxRootCBVsPerStage];
    uint16               RootCBVRegister[MaxRootCBVsPerStage];
    uint8                NumRootCBVs;
    FRootDescriptorEntry RootDescriptors[EResourceType::Count][MaxRootDescriptorsPerStage];
    uint8                NumRootDescriptors[EResourceType::Count];
};

class FD3D12RootSignature : public FD3D12DeviceChild, public FRefCountedBase
{
public:
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    static bool Serialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob);
#else
    static bool Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob);
#endif
    
public:
    FD3D12RootSignature(FD3D12Device* InDevice);
    ~FD3D12RootSignature();

    bool Initialize(const FD3D12RootSignatureLayout& Layout);
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    bool Initialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Desc);
#else
    bool Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc);
#endif
    bool Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    bool HasDenyFlag(EShaderVisibility::Type Stage) const;

    FORCEINLINE bool IsRootCBV(EShaderVisibility::Type Stage, uint16 Register) const
    {
        return ShaderStages[Stage].IsRootCBV(Register);
    }

    FORCEINLINE void SetDebugName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        RootSignature->SetName(*WideName);
    }

    FORCEINLINE ID3D12RootSignature*  GetD3D12RootSignature() const
    {
        return RootSignature.Get();
    }

    FORCEINLINE ID3D12RootSignature** GetD3D12RootSignatureAddress()
    {
        return RootSignature.GetAddressOf();
    }

    FORCEINLINE int32 GetRootParameterIndex(EShaderVisibility::Type Visibility, EResourceType::Type Type) const
    {
        return static_cast<int32>(ShaderStages[Visibility].GetRootParameterIndex(Type));
    }

    FORCEINLINE int32 GetMaxResourceCount(EShaderVisibility::Type Visibility, EResourceType::Type Type) const
    {
        return static_cast<int32>(ShaderStages[Visibility].GetNumResources(Type));
    }

    FORCEINLINE int32 Get32BitConstantsIndex() const
    {
        return ConstantRootParameterIndex;
    }

    FORCEINLINE uint32 GetNum32BitConstants() const
    {
        return Num32BitConstants;
    }

    FORCEINLINE int8 GetSlotForRegister(EShaderVisibility::Type Stage, EResourceType::Type Type, uint16 Register) const
    {
        return TableMappings[Stage][Type].GetSlotForRegister(Register);
    }

    FORCEINLINE int32 GetRootCBVParameterIndex(EShaderVisibility::Type Stage, uint16 Register) const
    {
        return ShaderStages[Stage].GetRootCBVParameterIndex(Register);
    }

    FORCEINLINE D3D12_ROOT_SIGNATURE_FLAGS GetFlags() const
    {
        return Flags;
    }

    FORCEINLINE uint64 GetHash() const
    {
        return Hash;
    }

    FORCEINLINE const FD3D12DescriptorTableMapping& GetDescriptorTableMapping(EShaderVisibility::Type Stage, EResourceType::Type Type) const
    {
        return TableMappings[Stage][Type];
    }

    FORCEINLINE const FD3D12ShaderStage& GetShaderStage(EShaderVisibility::Type Visibility) const
    {
        return ShaderStages[Visibility];
    }

private:
#if D3D12_USE_VERSIONED_ROOT_SIGNATURES
    void InternalInitRootParameterMap(const D3D12_ROOT_PARAMETER1* Parameters, uint32 NumParameters);
    void InternalInitTableMappingsFromDesc(const D3D12_ROOT_PARAMETER1* Parameters, uint32 NumParameters);
#else
    void InternalInitRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc);
    void InternalInitTableMappingsFromDesc(const D3D12_ROOT_SIGNATURE_DESC& Desc);
#endif
    bool InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    TComPtr<ID3D12RootSignature> RootSignature;
    FD3D12ShaderStage            ShaderStages[EShaderVisibility::Count];
    FD3D12DescriptorTableMapping TableMappings[EShaderVisibility::Count][EResourceType::Count];
    int32                        ConstantRootParameterIndex;
    uint32                       Num32BitConstants;
    D3D12_ROOT_SIGNATURE_FLAGS   Flags;
    uint64                       Hash;
};

class FD3D12RootSignatureManager : public FD3D12DeviceChild
{
public:
    FD3D12RootSignatureManager(FD3D12Device* Device);
    ~FD3D12RootSignatureManager();

    FD3D12RootSignature* GetOrCreateRootSignature(const FD3D12RootSignatureLayout& Layout);
    
    void ReleaseAll();

private:
    FD3D12RootSignature* CreateRootSignature(const FD3D12RootSignatureLayout& Layout);

    TArray<FD3D12RootSignatureRef>    RootSignatures;
    TArray<FD3D12RootSignatureLayout> ResourceLayouts;
};
