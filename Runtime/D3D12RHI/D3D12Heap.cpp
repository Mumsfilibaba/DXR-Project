#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

FD3D12Heap::FD3D12Heap(FD3D12Device* InDevice, ID3D12Heap* InHeap)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Heap(InHeap)
    , Desc(InHeap ? InHeap->GetDesc() : D3D12_HEAP_DESC{})
    , ResidencyHandle()
    , bShouldDeferredRelease(true)
{
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
    EndResidencyTracking();
}

void FD3D12Heap::StartResidencyTracking()
{
    if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
    {
        ResidencyHandle = ResidencyManager->RegisterPageable(Heap.Get(), Desc.SizeInBytes, false);
        ResidencyManager->TouchPageable(ResidencyHandle);
    }
}

void FD3D12Heap::EndResidencyTracking()
{
    if (ResidencyHandle.IsValid())
    {
        if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
        {
            ResidencyManager->UnregisterPageable(ResidencyHandle);
        }

        ResidencyHandle = {};
    }
}

void FD3D12Heap::DeferredRelease()
{
    FD3D12RHI::DeferDeletion(this);
}
