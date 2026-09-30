#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

FD3D12Heap::FD3D12Heap(FD3D12Device* InDevice, ID3D12Heap* InHeap)
    : FRefCountedBase()
    , FD3D12DeviceChild(InDevice)
    , Heap(InHeap)
    , Desc(InHeap ? InHeap->GetDesc() : D3D12_HEAP_DESC{})
    , ResidencyHandle()
    , bShouldDeferredRelease(true)
{
}

void FD3D12Heap::SetDebugName(const String& Name)
{
    D3D12SetDebugName(Heap.Get(), Name);
}

FD3D12Heap::~FD3D12Heap()
{
    EndResidencyTracking();
}

void FD3D12Heap::StartResidencyTracking()
{
    if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
    {
        ResidencyHandle.Initialize(Heap.Get(), Desc.SizeInBytes);
        ResidencyManager->BeginTrackingObject(&ResidencyHandle);
    }
}

void FD3D12Heap::EndResidencyTracking()
{
    if (ResidencyHandle.IsInitialized())
    {
        if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
        {
            ResidencyManager->EndTrackingObject(&ResidencyHandle);
        }
    }
}

void FD3D12Heap::DeferredRelease()
{
    FD3D12DeviceRHI::DeferDeletion(this);
}
