#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Shader.h"
#include "D3D12RefCounted.h"
#include "Core/Containers/Map.h"

typedef TSharedRef<class FD3D12RootSignature> FD3D12RootSignatureRef;

enum class ERootSignatureType
{
    Unknown          = 0,
    Graphics         = 1,
    Compute          = 2,
    RayTracingGlobal = 3,
    RayTracingLocal  = 4,
};

struct FD3D12RootSignatureLayout
{
    FD3D12RootSignatureLayout()
        : Type(ERootSignatureType::Unknown)
        , bAllowInputAssembler(false)
    {
    }

    bool IsCompatible(const FD3D12RootSignatureLayout& Other) const;

    FShaderResourceCount ResourceCounts[ShaderVisibility_Count];
    ERootSignatureType   Type;
    bool                 bAllowInputAssembler;
};

class FD3D12RootSignatureDescHelper
{
public:
    FD3D12RootSignatureDescHelper(const FD3D12RootSignatureLayout& RootSignatureInfo);

    const uint32 GetRootSignatureCost() const { return RootSignatureCost; }
    const D3D12_ROOT_SIGNATURE_DESC& GetDesc() const { return Desc; }

private:
    static void InitDescriptorRange(D3D12_DESCRIPTOR_RANGE& OutRange, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32 NumDescriptors, uint32 BaseShaderRegister, uint32 RegisterSpace);

    void InsertDescriptorTable(D3D12_SHADER_VISIBILITY ShaderVisibility, const D3D12_DESCRIPTOR_RANGE* DescriptorRanges, uint32 NumDescriptorRanges);
    void Insert32BitConstantRange(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 Num32BitConstants, uint32 ShaderRegister, uint32 RegisterSpace);
    void InsertRootCBV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);
    void InsertRootSRV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);
    void InsertRootUAV(D3D12_SHADER_VISIBILITY ShaderVisibility, uint32 ShaderRegister, uint32 RegisterSpace);

    D3D12_ROOT_SIGNATURE_DESC Desc;

    D3D12_ROOT_PARAMETER   RootParameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGES];

    uint32 NumRootParameters   = 0;
    uint32 NumDescriptorRanges = 0;
    uint32 RootSignatureCost   = 0;
};

class FD3D12RootSignature : public FD3D12DeviceChild, public FD3D12RefCounted
{
    struct FShaderStage
    {
        int8 RootParameterIndicies[ResourceType_Count];
        int8 ResourceCount[ResourceType_Count];
    };

public:
    FD3D12RootSignature(FD3D12Device* InDevice);
    ~FD3D12RootSignature() = default;

    static bool Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob);
    
    bool Initialize(const FD3D12RootSignatureLayout& RootSignatureInfo);
    bool Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc);
    bool Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    void SetDebugName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        RootSignature->SetName(*WideName);
    }

    ID3D12RootSignature* GetD3D12RootSignature() const { return RootSignature.Get(); }
    ID3D12RootSignature** GetD3D12RootSignatureAddress() { return RootSignature.GetAddressOf(); }

    int32 GetRootParameterIndex(EShaderVisibility Visibility, EResourceType Type) const
    {
        return static_cast<int32>(RootParameterMap[Visibility].RootParameterIndicies[Type]);
    }

    int32 GetMaxResourceCount(EShaderVisibility Visibility, EResourceType Type) const
    {
        return static_cast<int32>(RootParameterMap[Visibility].ResourceCount[Type]);
    }

    int32 Get32BitConstantsIndex() const
    {
        return ConstantRootParameterIndex;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    void InternalInitRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc);
    bool InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    TComPtr<ID3D12RootSignature> RootSignature;
    FShaderStage                 RootParameterMap[ShaderVisibility_Count];
    int32                        ConstantRootParameterIndex;
    uint64                       Hash;
};

class FD3D12RootSignatureManager : public FD3D12DeviceChild
{
public:
    FD3D12RootSignatureManager(FD3D12Device* Device);
    ~FD3D12RootSignatureManager();

    bool Initialize();
    void ReleaseAll();
    FD3D12RootSignature* GetOrCreateRootSignature(const FD3D12RootSignatureLayout& ResourceCount);

private:
    FD3D12RootSignature* CreateRootSignature(const FD3D12RootSignatureLayout& ResourceCount);

    // TODO: Use a hash instead, this is beacuse == operator does not make sense, use it anyway?
    TArray<FD3D12RootSignatureRef>    RootSignatures;
    TArray<FD3D12RootSignatureLayout> ResourceLayouts;
};