#pragma once
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanTimestampQuery> FVulkanTimestampQueryRef;

class FVulkanTimestampQuery : public FRHITimestampQuery, public FVulkanDeviceChild
{
public:
    FVulkanTimestampQuery(FVulkanDevice* InDevice);
    virtual ~FVulkanTimestampQuery() = default;

    bool Initialize();

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final;

    virtual uint64 GetFrequency() const override final;
};

