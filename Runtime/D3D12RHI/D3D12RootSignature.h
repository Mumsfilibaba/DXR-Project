#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12Shader.h"

#include "Core/RefCounted.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Utilities/HashUtilities.h"
#include "Core/Containers/HashTable.h"

class CD3D12RootSignature;

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
// SD3D12RootSignatureResourceCount

struct SD3D12RootSignatureResourceCount
{
    bool IsCompatible(const SD3D12RootSignatureResourceCount& Other) const;

    ERootSignatureType   Type = ERootSignatureType::Unknown;
    SShaderResourceCount ResourceCounts[ShaderVisibility_Count];
    bool                 bAllowInputAssembler = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignatureDescHelper

class CD3D12RootSignatureDescHelper
{
public:
    CD3D12RootSignatureDescHelper(const SD3D12RootSignatureResourceCount& RootSignatureInfo);
    ~CD3D12RootSignatureDescHelper() = default;

    FORCEINLINE const D3D12_ROOT_SIGNATURE_DESC& GetDesc() const { return Desc; }

private:

    static void InitDescriptorRange( D3D12_DESCRIPTOR_RANGE& OutRange
                                   , D3D12_DESCRIPTOR_RANGE_TYPE Type
                                   , uint32 NumDescriptors
                                   , uint32 BaseShaderRegister
                                   , uint32 RegisterSpace);
    
    static void InitDescriptorTable( D3D12_ROOT_PARAMETER& OutParameter
                                   , D3D12_SHADER_VISIBILITY ShaderVisibility
                                   , const D3D12_DESCRIPTOR_RANGE* DescriptorRanges
                                   , uint32 NumDescriptorRanges);

    static void Init32BitConstantRange( D3D12_ROOT_PARAMETER& OutParameter
                                      , D3D12_SHADER_VISIBILITY ShaderVisibility
                                      , uint32 Num32BitConstants
                                      , uint32 ShaderRegister
                                      , uint32 RegisterSpace);

    D3D12_ROOT_SIGNATURE_DESC Desc;
    D3D12_ROOT_PARAMETER      Parameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE    DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGES];

    uint32 NumDescriptorRanges = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignature

class CD3D12RootSignature : public CD3D12DeviceObject, public CRefCounted
{
public:

    CD3D12RootSignature(CD3D12Device* InDevice);
    ~CD3D12RootSignature() = default;

    bool Initialize(const SD3D12RootSignatureResourceCount& RootSignatureInfo);
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

    FORCEINLINE void SetName(const String& Name)
    {
        RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.CStr());
    }

    FORCEINLINE ID3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

    FORCEINLINE ID3D12RootSignature** GetAddressOfRootSignature()
    {
        return RootSignature.GetAddressOf();
    }

    static bool Serialize(const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob);

private:
    void CreateRootParameterMap(const D3D12_ROOT_SIGNATURE_DESC& Desc);

    bool InternalInit(const void* BlobWithRootSignature, uint64 BlobLengthInBytes);

    TComPtr<ID3D12RootSignature> RootSignature;
    int32 RootParameterMap[ShaderVisibility_Count][ResourceType_Count];

    // TODO: Enable this for all shader visibilities
    int32 ConstantRootParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RootSignatureCache

class CD3D12RootSignatureCache : public CD3D12DeviceObject
{
public:
    CD3D12RootSignatureCache(CD3D12Device* Device);
    ~CD3D12RootSignatureCache();

    bool Initialize();
    void ReleaseAll();

    CD3D12RootSignature* GetOrCreateRootSignature(const SD3D12RootSignatureResourceCount& ResourceCount);

    static CD3D12RootSignatureCache& Get();

private:
    CD3D12RootSignature* CreateRootSignature(const SD3D12RootSignatureResourceCount& ResourceCount);

    // TODO: Use a hash instead, this is beacuse == operator does not make sense, use it anyway?
    TArray<TSharedRef<CD3D12RootSignature>>  RootSignatures;
    TArray<SD3D12RootSignatureResourceCount> ResourceCounts;

    static CD3D12RootSignatureCache* Instance;
};