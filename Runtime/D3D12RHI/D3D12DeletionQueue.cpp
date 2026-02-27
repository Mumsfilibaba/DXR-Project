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
                Item.Resource->Release();
                break;
            }

            case FD3D12DeferredObject::EType::D3DResource:
            {
                CHECK(Item.D3DResource != nullptr);
                Item.D3DResource->Release();
                break;
            }

            case FD3D12DeferredObject::EType::D3DHeap:
            {
                CHECK(Item.D3DHeap != nullptr);
                Item.D3DHeap->Release();
                break;
            }

            case FD3D12DeferredObject::EType::OnlineDescriptorBlock:
            {
                CHECK(Item.OnlineDescriptorBlock.Heap != nullptr);
                Item.OnlineDescriptorBlock.Heap->RecycleBlock(Item.OnlineDescriptorBlock.Block);
                break;
            }

            case FD3D12DeferredObject::EType::Heap:
            {
                CHECK(Item.D3D12Heap != nullptr);
                Item.D3D12Heap->Release();
                break;
            }

            case FD3D12DeferredObject::EType::BuddyAllocatorBlock:
            {
                CHECK(Item.BuddyAllocatorBlock.Allocator != nullptr);
                Item.BuddyAllocatorBlock.Allocator->RecycleAllocation(Item.BuddyAllocatorBlock.AllocationData);
                break;
            }

            case FD3D12DeferredObject::EType::PoolAllocatorBlock:
            {
                CHECK(Item.PoolAllocatorBlock.Allocator != nullptr);
                Item.PoolAllocatorBlock.Allocator->RecycleAllocation(Item.PoolAllocatorBlock.AllocationData);
                break;
            }

            case FD3D12DeferredObject::EType::BucketAllocatorBlock:
            {
                CHECK(Item.BucketAllocatorBlock.Allocator != nullptr);
                Item.BucketAllocatorBlock.Allocator->RecycleAllocation(Item.BucketAllocatorBlock.AllocationData);
                break;
            }

            case FD3D12DeferredObject::EType::LinearAllocatorPage:
            {
                CHECK(Item.LinearAllocatorPage.Allocator != nullptr);
                CHECK(Item.LinearAllocatorPage.Page != nullptr);
                Item.LinearAllocatorPage.Allocator->ReturnPage(Item.LinearAllocatorPage.Page);
                break;
            }
        }
    }
}
