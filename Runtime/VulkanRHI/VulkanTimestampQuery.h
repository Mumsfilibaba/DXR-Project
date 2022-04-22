#pragma once
#include "VulkanDeviceObject.h"

#include "RHI/RHIResources.h"

#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanTimestampQuery

class CVulkanTimestampQuery : public CRHITimestampQuery, public CVulkanDeviceObject
{
public:

    static TSharedRef<CVulkanTimestampQuery> CreateQuery(CVulkanDevice* InDevice);

    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const override final;
    virtual uint64 GetFrequency() const override final;

private:

    CVulkanTimestampQuery(CVulkanDevice* InDevice);
    ~CVulkanTimestampQuery() = default;

    bool Initialize();
};

