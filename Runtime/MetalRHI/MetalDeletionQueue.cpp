#include "MetalRHI/MetalDeletionQueue.h"
#include "MetalRHI/MetalBindlessDescriptors.h"

void FMetalDeferredObject::ProcessItems(const TArray<FMetalDeferredObject>& Items)
{
    for (const FMetalDeferredObject& Item : Items)
    {
        switch (Item.Type)
        {
            case FMetalDeferredObject::EType::RHIResource:
            {
                CHECK(Item.RHIResource != nullptr);
                delete Item.RHIResource;
                break;
            }

            case FMetalDeferredObject::EType::MTLResource:
            {
                CHECK(Item.Resource != nil);
                [Item.Resource release];
                break;
            }

            case FMetalDeferredObject::EType::MTLHeap:
            {
                CHECK(Item.Heap != nil);
                [Item.Heap release];
                break;
            }

            case FMetalDeferredObject::EType::MTLObject:
            {
                CHECK(Item.Object != nil);
                [Item.Object release];
                break;
            }

            case FMetalDeferredObject::EType::BindlessSlot:
            {
                CHECK(Item.BindlessSlot.Manager != nullptr);
                Item.BindlessSlot.Manager->RecycleSlot(Item.BindlessSlot.Handle);
                break;
            }
        }
    }
}
