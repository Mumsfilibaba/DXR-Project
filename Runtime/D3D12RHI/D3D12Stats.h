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
