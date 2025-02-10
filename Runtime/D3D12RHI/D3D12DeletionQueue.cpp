#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12DeletionQueue.h"

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
            case FD3D12DeferredObject::EType::OnlineDescriptorBlock:
            {
                CHECK(Item.OnlineDescriptorBlock.Heap != nullptr);
                FD3D12OnlineDescriptorHeap* Heap = Item.OnlineDescriptorBlock.Heap;
                Heap->RecycleBlock(Item.OnlineDescriptorBlock.Block);
                break;
            }
        }
    }
}
