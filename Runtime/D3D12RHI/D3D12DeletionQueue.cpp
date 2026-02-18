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
                CHECK(Item.NativeHeap.Heap != nullptr);
                Item.NativeHeap.Heap->Release();
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
                case ED3D12DeferredAllocatorType::Bucket:
                    static_cast<FD3D12BucketAllocator*>(Item.AllocatorBlock.Allocator)->RecycleAllocation(Item.AllocatorBlock.BucketAllocationData);
                    break;
                }

                break;
            }
        }
    }
}
