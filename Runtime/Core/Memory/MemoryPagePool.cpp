#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryPagePool.h"
#include "Core/Memory/MemoryStats.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"

static TAutoConsoleVariable<bool> CVarEnableStackPagePooling(
    "Memory.EnableStackPagePooling",
    "Recycle FMemoryStack pages through the page pool instead of returning them to the allocator",
    true);

FMemoryPagePool::FMemoryPagePool()
    : FreeList(nullptr)
    , NumFreePages(0)
    , LowWaterMark(0)
    , FramesSincePrune(0)
    , bPoolingEnabled(true)
{
}

FMemoryPagePool::~FMemoryPagePool() = default;

FMemoryPagePool& FMemoryPagePool::Get()
{
    static FMemoryPagePool* MemoryPagePool = new FMemoryPagePool();
    return *MemoryPagePool;
}

void* FMemoryPagePool::AcquirePage(int32 InBlockSize)
{
    if (InBlockSize == BlockSize)
    {
        TScopedLock Lock(PoolCS);

        if (bPoolingEnabled)
        {
            if (FreeList)
            {
                FFreePage* Recycled = FreeList;
                FreeList = Recycled->Next;
                NumFreePages--;

                LowWaterMark = Math::Min(LowWaterMark, NumFreePages);

                STAT_SUBTRACT(STAT_Memory_StackPooledBytes, BlockSize);
                STAT_SUBTRACT(STAT_Memory_StackPooledPageCount, 1);
                return Recycled;
            }

            // Demand outran the pool, so nothing retained during this window is surplus
            LowWaterMark = 0;
        }
    }

    return Memory::Malloc(InBlockSize);
}

void FMemoryPagePool::ReleasePage(void* Page, int32 InBlockSize)
{
    CHECK(Page != nullptr);

    if (InBlockSize == BlockSize)
    {
        TScopedLock Lock(PoolCS);

        if (bPoolingEnabled && NumFreePages < MaxRetainedPages)
        {
            FFreePage* FreePage = reinterpret_cast<FFreePage*>(Page);
            FreePage->Next = FreeList;
            FreeList       = FreePage;
            NumFreePages++;

            STAT_ADD(STAT_Memory_StackPooledBytes, BlockSize);
            STAT_ADD(STAT_Memory_StackPooledPageCount, 1);
            return;
        }
    }

    Memory::Free(Page);
}

void FMemoryPagePool::Tick()
{
    FFreePage* Pruned = nullptr;
    {
        TScopedLock Lock(PoolCS);

        bPoolingEnabled = CVarEnableStackPagePooling.GetValue();
        FramesSincePrune++;

        if (bPoolingEnabled && FramesSincePrune < PruneIntervalFrames)
        {
            return;
        }

        FramesSincePrune = 0;

        Pruned = DetachPages(bPoolingEnabled ? LowWaterMark : NumFreePages);
    }

    FreePageList(Pruned);
}

void FMemoryPagePool::Flush()
{
    FFreePage* Pruned = nullptr;
    {
        TScopedLock Lock(PoolCS);
        Pruned = DetachPages(NumFreePages);
    }

    FreePageList(Pruned);
}

int32 FMemoryPagePool::GetNumFreePages() const
{
    TScopedLock Lock(PoolCS);
    return NumFreePages;
}

int32 FMemoryPagePool::GetNumPooledBytes() const
{
    TScopedLock Lock(PoolCS);
    return NumFreePages * BlockSize;
}

FMemoryPagePool::FFreePage* FMemoryPagePool::DetachPages(int32 NumPages)
{
    FFreePage* Detached = nullptr;
    for (int32 Index = 0; Index < NumPages && FreeList; Index++)
    {
        FFreePage* Page = FreeList;
        FreeList   = Page->Next;
        Page->Next = Detached;
        Detached   = Page;
        NumFreePages--;

        STAT_SUBTRACT(STAT_Memory_StackPooledBytes, BlockSize);
        STAT_SUBTRACT(STAT_Memory_StackPooledPageCount, 1);
    }

    LowWaterMark = NumFreePages;
    return Detached;
}

void FMemoryPagePool::FreePageList(FFreePage* Pages)
{
    // Freed with no lock held, since the allocator may block
    while (Pages)
    {
        FFreePage* Next = Pages->Next;
        Memory::Free(Pages);
        Pages = Next;
    }
}
