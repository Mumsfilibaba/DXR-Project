#include "VulkanTimestampQuery.h"

FVulkanTimestampQuery::FVulkanTimestampQuery(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
{
}

bool FVulkanTimestampQuery::Initialize()
{
    return true;
}

void FVulkanTimestampQuery::GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const
{
    OutQuery = FRHITimestamp();
}

uint64 FVulkanTimestampQuery::GetFrequency() const
{
    return 1;
}
