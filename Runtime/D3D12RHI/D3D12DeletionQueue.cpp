#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12DeletionQueue.h"
#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

void FD3D12DeferredObject::ProcessItems(const TArray<FD3D12DeferredObject>& Items)
{
    for (const FD3D12DeferredObject& Item : Items)
    {
        switch(Item.Type)
        {
            case FD3D12DeferredObject::EType::RHIResource:
            {
                CHECK(Item.RHIResource != nullptr);
                delete Item.RHIResource;
                break;
            }
            
            case FD3D12DeferredObject::EType::Resource:
            {
                CHECK(Item.Resource != nullptr);
                if (FD3D12Device* Device = Item.Resource->GetDevice())
                {
                    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
                    {
                        ResidencyManager->UnregisterPageable(Item.Resource->GetD3D12Resource());
                    }
                }

                Item.Resource->Release();
                break;
            }

            case FD3D12DeferredObject::EType::D3DResource:
            {
                CHECK(Item.D3DResource != nullptr);
                Item.D3DResource->Release();
                break;
            }

            case FD3D12DeferredObject::EType::OnlineDescriptorBlock:
            {
                CHECK(Item.OnlineDescriptorBlock.Heap != nullptr);

                FD3D12OnlineDescriptorHeap* Heap = Item.OnlineDescriptorBlock.Heap;
                Heap->RecycleBlock(Item.OnlineDescriptorBlock.Block);
                break;
            }

            case FD3D12DeferredObject::EType::Heap:
            {
                CHECK(Item.D3D12Heap != nullptr);

                if (FD3D12Device* Device = Item.D3D12Heap->GetDevice())
                {
                    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
                    {
                        const FD3D12ResidencyHandle& ResidencyHandle = Item.D3D12Heap->GetResidencyHandle();
                        if (ResidencyHandle.IsValid())
                        {
                            ResidencyManager->UnregisterPageable(ResidencyHandle);
                        }
                        else
                        {
                            ResidencyManager->UnregisterPageable(Item.D3D12Heap->GetD3D12Heap());
                        }
                    }
                }

                Item.D3D12Heap->Release();
                break;
            }

            case FD3D12DeferredObject::EType::AllocatorBlock:
            {
                CHECK(Item.AllocatorBlock.Allocator != nullptr);

                switch (Item.AllocatorBlock.AllocatorType)
                {
                case ED3D12DeferredAllocatorType::Pool:
                    static_cast<FD3D12PoolAllocator*>(Item.AllocatorBlock.Allocator)->RecycleAllocation(Item.AllocatorBlock.PoolAllocationData);
                    break;
                case ED3D12DeferredAllocatorType::Buddy:
                    static_cast<FD3D12BuddyAllocator*>(Item.AllocatorBlock.Allocator)->RecycleAllocation(Item.AllocatorBlock.BuddyAllocationData);
                    break;
                case ED3D12DeferredAllocatorType::MultiBuddy:
                    static_cast<FD3D12MultiBuddyAllocator*>(Item.AllocatorBlock.Allocator)->RecycleAllocation(Item.AllocatorBlock.MultiBuddyAllocationData);
                    break;
                case ED3D12DeferredAllocatorType::Bucket:
                    static_cast<FD3D12BucketAllocator*>(Item.AllocatorBlock.Allocator)->RecycleAllocation(Item.AllocatorBlock.BucketAllocationData);
                    break;
                }

                break;
            }
        }
    }
}
