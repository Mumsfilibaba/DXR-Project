#include "MetalRHI/MetalDeletionQueue.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalRHI.h"

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

            case FMetalDeferredObject::EType::ObjCObject:
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

            case FMetalDeferredObject::EType::StandaloneResource:
            {
                CHECK(Item.Resource != nil);

                if (FMetalDevice* Device = FMetalDeviceRHI::Get()->GetMetalDevice())
                {
                    Device->GetResidencySet().Remove(Item.Resource);
                }

                [Item.Resource release];
                break;
            }
        }
    }
}
