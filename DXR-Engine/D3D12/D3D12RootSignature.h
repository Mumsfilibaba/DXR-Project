#pragma once
#include "D3D12DeviceChild.h"

#include "Core/RefCountedObject.h"

#include "Utilities/StringUtilities.h"

class D3D12RootSignature;

#define D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER 0
#define D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER        1
#define D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER   2
#define D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER  3
#define D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER          4
#define D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT          32
#define D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT         16
#define D3D12_DEFAULT_ONLINE_RESOURCE_DESCRIPTOR_HEAP_COUNT 2048
#define D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_HEAP_COUNT  2048

struct D3D12DefaultRootSignatures
{
    TSharedRef<D3D12RootSignature> Graphics;
    TSharedRef<D3D12RootSignature> Compute;
    TSharedRef<D3D12RootSignature> GlobalRayTracing;
    TSharedRef<D3D12RootSignature> LocalRayTracing;

    Bool CreateRootSignatures(class D3D12Device* Device);
};

class D3D12RootSignature : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12RootSignature(D3D12Device* InDevice, ID3D12RootSignature* InRootSignature)
        : D3D12DeviceChild(InDevice)
        , RootSignature(InRootSignature)
    {
        VALIDATE(RootSignature != nullptr);
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        RootSignature->SetName(WideName.c_str());
    }

    FORCEINLINE ID3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

    FORCEINLINE ID3D12RootSignature* const * GetAddressOfRootSignature() const
    {
        return RootSignature.GetAddressOf();
    }

private:
    TComPtr<ID3D12RootSignature> RootSignature;
};