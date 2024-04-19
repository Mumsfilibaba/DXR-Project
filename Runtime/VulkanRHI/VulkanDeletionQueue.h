#pragma once
#include "VulkanCore.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Platform/CriticalSection.h"

class FRHIResource;
class FVulkanDescriptorPool;

struct FVulkanDeferredObject
{
    // Go through an array of deferred resources and delete them
    static void ProcessItems(const TArray<FVulkanDeferredObject>& Items);

    enum class EType
    {
        RHIResource    = 1,
        VulkanResource = 2,
    };

    FVulkanDeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
        , Resource(InResource)
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    FVulkanDeferredObject(FVulkanRefCounted* InResource)
        : Type(EType::VulkanResource)
        , VulkanResource(InResource)
    {
        CHECK(InResource != nullptr);
        InResource->AddRef();
    }

    EType Type;
    union
    {
        FRHIResource* Resource;
        FVulkanRefCounted* VulkanResource;
    };
};