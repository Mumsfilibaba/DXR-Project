#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// D3D12 Allocator Pool Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferPoolAllocated);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferPoolUsed);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferPoolFragmented);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TexturePoolAllocated);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TexturePoolUsed);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TexturePoolFragmented);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_UploadHeapAllocated);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_UploadHeapUsed);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_UploadHeapFragmented);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommittedResourceMemory);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommittedResourceCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommittedDefaultMemory);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommittedUploadMemory);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommittedReadbackMemory);

// -------------------------------------------------------------------------------------------
// D3D12 Defrag Activity Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TextureDefragPending);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TextureDefragMovesCompleted);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_TextureDefragBytesMoved);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferDefragPending);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferDefragMovesCompleted);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_BufferDefragBytesMoved);

// -------------------------------------------------------------------------------------------
// D3D12 Residency Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyEvictionCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyEvictedBytes);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyMakeResidentCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyMakeResidentBytes);

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyTrackedCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyResidentBytes);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_ResidencyBudget);

// -------------------------------------------------------------------------------------------
// D3D12 PSO Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_PSOCreateCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_PSOCacheSize);

// -------------------------------------------------------------------------------------------
// D3D12 Command Primitive Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommandListCount);
STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_CommandAllocatorCount);

// -------------------------------------------------------------------------------------------
// D3D12 Query Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D12RHI_API, STAT_D3D12_QueryHeapCount);
