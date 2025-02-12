#pragma once
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanRefCounted.h"

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
        , RHIResource(InResource)
    {
        CHECK(InResource != nullptr);
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
        FRHIResource*      RHIResource;
        FVulkanRefCounted* VulkanResource;
    };
};