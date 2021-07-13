#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Device.h"
#include "D3D12Shader.h"

#include "Core/RefCountedObject.h"

#include "Utilities/StringUtilities.h"
#include "Utilities/HashUtilities.h"

#include <unordered_map>

class D3D12RootSignature;

enum class ERootSignatureType
{
    Unknown = 0,
    Graphics = 1,
    Compute = 2,
    RayTracingGlobal = 3,
    RayTracingLocal = 4,
};

struct D3D12RootSignatureResourceCount
{
    bool IsCompatible( const D3D12RootSignatureResourceCount& Other ) const;

    ERootSignatureType  Type = ERootSignatureType::Unknown;
    ShaderResourceCount ResourceCounts[ShaderVisibility_Count];
    bool AllowInputAssembler = false;
};

struct D3D12RootSignatureDescHelper
{
public:
    D3D12RootSignatureDescHelper( const D3D12RootSignatureResourceCount& RootSignatureInfo );
    ~D3D12RootSignatureDescHelper() = default;

    const D3D12_ROOT_SIGNATURE_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    static void InitDescriptorRange(
        D3D12_DESCRIPTOR_RANGE& OutRange,
        D3D12_DESCRIPTOR_RANGE_TYPE Type,
        uint32 NumDescriptors,
        uint32 BaseShaderRegister,
        uint32 RegisterSpace );

    static void InitDescriptorTable(
        D3D12_ROOT_PARAMETER& OutParameter,
        D3D12_SHADER_VISIBILITY ShaderVisibility,
        const D3D12_DESCRIPTOR_RANGE* DescriptorRanges,
        uint32 NumDescriptorRanges );

    static void Init32BitConstantRange(
        D3D12_ROOT_PARAMETER& OutParameter,
        D3D12_SHADER_VISIBILITY ShaderVisibility,
        uint32 Num32BitConstants,
        uint32 ShaderRegister,
        uint32 RegisterSpace );

    D3D12_ROOT_SIGNATURE_DESC Desc;
    D3D12_ROOT_PARAMETER      Parameters[D3D12_MAX_ROOT_PARAMETERS];
    D3D12_DESCRIPTOR_RANGE    DescriptorRanges[D3D12_MAX_DESCRIPTOR_RANGES];
    uint32 NumDescriptorRanges = 0;
};

class D3D12RootSignature : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12RootSignature( D3D12Device* InDevice );
    ~D3D12RootSignature() = default;

    bool Init( const D3D12RootSignatureResourceCount& RootSignatureInfo );
    bool Init( const D3D12_ROOT_SIGNATURE_DESC& Desc );
    bool Init( const void* BlobWithRootSignature, uint64 BlobLengthInBytes );

    // Returns -1 if root parameter is not valid
    int32 GetRootParameterIndex( EShaderVisibility Visibility, EResourceType Type ) const
    {
        return RootParameterMap[Visibility][Type];
    }

    int32 Get32BitConstantsIndex() const
    {
        return ConstantRootParameterIndex;
    }

    void SetName( const std::string& Name )
    {
        std::wstring WideName = ConvertToWide( Name );
        RootSignature->SetName( WideName.c_str() );
    }

    ID3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }
    ID3D12RootSignature* const* GetAddressOfRootSignature() const
    {
        return RootSignature.GetAddressOf();
    }

    static bool Serialize( const D3D12_ROOT_SIGNATURE_DESC& Desc, ID3DBlob** OutBlob );

private:
    void CreateRootParameterMap( const D3D12_ROOT_SIGNATURE_DESC& Desc );
    bool InternalInit( const void* BlobWithRootSignature, uint64 BlobLengthInBytes );

    TComPtr<ID3D12RootSignature> RootSignature;
    int32 RootParameterMap[ShaderVisibility_Count][ResourceType_Count];
    // TODO: Enable this for all shader visibilities
    int32 ConstantRootParameterIndex;
};

class D3D12RootSignatureCache : public D3D12DeviceChild
{
public:
    D3D12RootSignatureCache( D3D12Device* Device );
    ~D3D12RootSignatureCache();

    bool Init();
    void ReleaseAll();

    D3D12RootSignature* GetOrCreateRootSignature( const D3D12RootSignatureResourceCount& ResourceCount );

    static D3D12RootSignatureCache& Get();

private:
    D3D12RootSignature* CreateRootSignature( const D3D12RootSignatureResourceCount& ResourceCount );

    // TODO: Use a hash instead, this is beacuse == operator does not make sense, use it anyway?
    TArray<TSharedRef<D3D12RootSignature>>        RootSignatures;
    TArray<D3D12RootSignatureResourceCount> ResourceCounts;

    static D3D12RootSignatureCache* Instance;
};