#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12Shader.h"

#include "Core/RefCounted.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Utilities/HashUtilities.h"
#include "Core/Containers/HashTable.h"

class FD3D12RootSignature;

typedef TSharedRef<FD3D12RootSignature> FD3D12RootSignatureRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERootSignatureType

enum class ERootSignatureType
{
    Unknown          = 0,
    Graphics         = 1,
    Compute          = 2,
    RayTracingGlobal = 3,
    RayTracingLocal  = 4,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RootSignatureResourceCount

struct FD3D12RootSignatureResourceCount
{
    bool IsCompatible(const FD3D12RootSignatureResourceCount& Other) const;

    ERootSignatureType   Type = ERootSignatureType::Unknown;
    FShaderResourceCount ResourceCounts[ShaderVisibility_Count];
    bool AllowInputAssembler = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RootSignatureDescHelper

class FD3D12RootSignatureDescHelper
{
public:
    FD3D12RootSignatureDescHelper(const FD3D12RootSignatureResourceCount& RootSignatureInfo);
    ~FD3D12RootSignatureDescHelper() = default;

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

    D3D12_ROOT_PARAMETER      RootParameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE    DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGES];

    uint32 NumRootParameters   = 0;
    uint32 NumDescriptorRanges = 0;
    uint32 RootSignatureCost   = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RootSignature

class FD3D12RootSignature : public FD3D12DeviceChild, public FRefCounted
{
public:

    FD3D12RootSignature(FD3D12Device* InDevice);
    ~FD3D12RootSignature() = default;
    
public:

    static bool Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob);
    
    bool Initialize(const FD3D12RootSignatureResourceCount& RootSignatureInfo);
    bool Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc);
    bool Initialize(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    // Returns -1 if root parameter is not valid
    FORCEINLINE int32 GetRootParameterIndex(EShaderVisibility Visibility, EResourceType Type) const
    {
        return RootParameterMap[Visibility][Type];
    }

    FORCEINLINE int32 Get32BitConstantsIndex() const
    {
        return ConstantRootParameterIndex;
    }

    FORCEINLINE void SetName(const FString& Name)
    {
        FWString WideName = CharToWide(Name);
        RootSignature->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

    FORCEINLINE ID3D12RootSignature** GetAddressOfRootSignature()
    {
        return RootSignature.GetAddressOf();
    }

private:
    void CreateRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc);

    bool InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    TComPtr<ID3D12RootSignature> RootSignature;

    int32 RootParameterMap[ShaderVisibility_Count][ResourceType_Count];
    // TODO: Enable this for all shader visibilities
    int32 ConstantRootParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RootSignatureCache

class FD3D12RootSignatureCache : public FD3D12DeviceChild
{
public:
    FD3D12RootSignatureCache(FD3D12Device* Device);
    ~FD3D12RootSignatureCache();

    bool Initialize();

    void ReleaseAll();

    FD3D12RootSignature* GetOrCreateRootSignature(const FD3D12RootSignatureResourceCount& ResourceCount);

private:
    FD3D12RootSignature* CreateRootSignature(const FD3D12RootSignatureResourceCount& ResourceCount);

    // TODO: Use a hash instead, this is beacuse == operator does not make sense, use it anyway?
    TArray<FD3D12RootSignatureRef>           RootSignatures;
    TArray<FD3D12RootSignatureResourceCount> ResourceCounts;
};