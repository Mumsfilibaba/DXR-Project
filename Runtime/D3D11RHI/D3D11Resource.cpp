#include "D3D11RHI/D3D11Resource.h"
#include "D3D11RHI/D3D11Stats.h"

FD3D11Resource::~FD3D11Resource()
{
#if D3D11_ENABLE_STATS
    if (AllocationSize > 0)
    {
        STAT_SUBTRACT(STAT_D3D11_ResourceMemory, AllocationSize);
        STAT_SUBTRACT(STAT_D3D11_ResourceCount, 1);

        switch (Usage)
        {
        case D3D11_USAGE_DEFAULT:
        case D3D11_USAGE_IMMUTABLE: STAT_SUBTRACT(STAT_D3D11_DefaultMemory, AllocationSize); break;
        case D3D11_USAGE_DYNAMIC:   STAT_SUBTRACT(STAT_D3D11_DynamicMemory, AllocationSize); break;
        case D3D11_USAGE_STAGING:   STAT_SUBTRACT(STAT_D3D11_StagingMemory, AllocationSize); break;
        }
    }
#endif
}

void FD3D11Resource::SetAllocation(D3D11_USAGE InUsage, uint64 InAllocationSize)
{
    CHECK(AllocationSize == 0);

    Usage          = InUsage;
    AllocationSize = InAllocationSize;

#if D3D11_ENABLE_STATS
    STAT_ADD(STAT_D3D11_ResourceMemory, AllocationSize);
    STAT_ADD(STAT_D3D11_ResourceCount, 1);

    switch (Usage)
    {
    case D3D11_USAGE_DEFAULT:
    case D3D11_USAGE_IMMUTABLE: STAT_ADD(STAT_D3D11_DefaultMemory, AllocationSize); break;
    case D3D11_USAGE_DYNAMIC:   STAT_ADD(STAT_D3D11_DynamicMemory, AllocationSize); break;
    case D3D11_USAGE_STAGING:   STAT_ADD(STAT_D3D11_StagingMemory, AllocationSize); break;
    }
#endif
}
