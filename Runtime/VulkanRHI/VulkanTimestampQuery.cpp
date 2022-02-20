#include "VulkanTimestampQuery.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTimestampQuery

TSharedRef<CVulkanTimestampQuery> CVulkanTimestampQuery::CreateQuery(CVulkanDevice* InDevice)
{
    TSharedRef<CVulkanTimestampQuery> NewQuery = dbg_new CVulkanTimestampQuery(InDevice);
    if (NewQuery && NewQuery->Initialize())
    {
        return NewQuery;
    }

    return nullptr;
}

CVulkanTimestampQuery::CVulkanTimestampQuery(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
{
}

bool CVulkanTimestampQuery::Initialize()
{
    return true;
}

void CVulkanTimestampQuery::GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const
{
    OutQuery = SRHITimestamp();
}

uint64 CVulkanTimestampQuery::GetFrequency() const
{
    return 1;
}
