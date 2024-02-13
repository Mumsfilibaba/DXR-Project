#pragma once
#include "VulkanCore.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Platform/CriticalSection.h"

class FRHIResource;
class FVulkanDescriptorPool;

class FVulkanDeletionQueue
{
public:
    enum class EType
    {
        RHIResource    = 1,
        VulkanResource = 2,
        DescriptorPool = 3,
    };

    struct FDeferredResource
    {
        FDeferredResource(FRHIResource* InResource)
            : Type(EType::RHIResource)
            , Resource(InResource)
        {
            CHECK(InResource != nullptr);
            InResource->AddRef();
        }

        FDeferredResource(FVulkanRefCounted* InResource)
            : Type(EType::VulkanResource)
            , VulkanResource(InResource)
        {
            CHECK(InResource != nullptr);
            InResource->AddRef();
        }
        
        FDeferredResource(FVulkanDescriptorPool* InDescriptorPool)
            : Type(EType::DescriptorPool)
            , DescriptorPool(InDescriptorPool)
        {
            CHECK(DescriptorPool != nullptr);
        }
        
        void Release();

        EType Type;
        union
        {
            FRHIResource*          Resource;
            FVulkanRefCounted*     VulkanResource;
            FVulkanDescriptorPool* DescriptorPool;
        };
    };

    template<typename... ArgTypes>
    void Emplace(ArgTypes&&... Args)
    {
        TScopedLock Lock(ResourcesCS);
        Resources.Emplace(Forward<ArgTypes>(Args)...);
    }

    void Dequeue(TArray<FDeferredResource>& OutResources)
    {
        TScopedLock Lock(ResourcesCS);
        OutResources = Move(Resources);
    }
    
private:
    TArray<FDeferredResource> Resources;
    FCriticalSection          ResourcesCS;
};

using FDeletionQueueArray = TArray<FVulkanDeletionQueue::FDeferredResource>;
