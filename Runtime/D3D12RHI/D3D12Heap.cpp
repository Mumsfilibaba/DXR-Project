#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12RHI.h"

FD3D12Heap::FD3D12Heap(FD3D12Device* InDevice)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , Desc()
    , ResidencyHandle()
    , bDeferDeletion(true)
{
}

void FD3D12Heap::SetNative(const TComPtr<ID3D12Heap>& InNativeHeap)
{
    Heap = InNativeHeap;
    Desc = Heap ? Heap->GetDesc() : D3D12_HEAP_DESC{};
}

void FD3D12Heap::SetDebugName(const FString& Name)
{
    if (Heap)
    {
        HRESULT Result = Heap->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Size(), *Name);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set heap name");
        }

        FStringWide WideName = CharToWide(Name);
        Result = Heap->SetName(*WideName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set heap name");
        }
    }
}

FD3D12Heap::~FD3D12Heap()
{
    ReleaseResource();
}

void FD3D12Heap::ReleaseResource()
{
    if (bDeferDeletion && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(this);
        return;
    }
    
    // Immediate deletion
    Heap.Reset();
}
