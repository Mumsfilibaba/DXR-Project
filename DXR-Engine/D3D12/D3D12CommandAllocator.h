#pragma once
#include "D3D12RefCountedObject.h"

class D3D12CommandAllocator : public D3D12RefCountedObject
{
public:
    D3D12CommandAllocator(D3D12Device* InDevice, ID3D12CommandAllocator* InAllocator)
        : D3D12RefCountedObject(InDevice)
        , Allocator(InAllocator)
    {
    }

    FORCEINLINE Bool Reset()
    {
        return SUCCEEDED(Allocator->Reset());
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        Allocator->SetName(WideName.c_str());
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
    {
        return Allocator.Get();
    }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};