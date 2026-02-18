#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

FD3D12Heap::FD3D12Heap(FD3D12Device* InDevice)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Heap(nullptr)
    , Desc()
    , ResidencyHandle()
    , bShouldDeferredRelease(true)
{
}

void FD3D12Heap::SetHeap(const TComPtr<ID3D12Heap>& InNativeHeap)
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
    if (Heap)
    {
        if (FD3D12Device* LocalDevice = GetDevice())
        {
            if (FD3D12ResidencyManager* ResidencyManager = LocalDevice->GetResidencyManager())
            {
                if (ResidencyHandle.IsValid())
                {
                    ResidencyManager->UnregisterPageable(ResidencyHandle);
                }
            }
        }
    }

    ResidencyHandle = {};
    Heap.Reset();
}

void FD3D12Heap::DeferredRelease()
{
    FD3D12RHI::DeferDeletion(this);
}
