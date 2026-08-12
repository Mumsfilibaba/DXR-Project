#pragma once
#include "Core/Core.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"

class CORE_API FMemoryPagePool : public FNonCopyAndNonMovable
{
public:

    /** @brief Total size of a pooled block, FMemoryStack's page header included */
    static constexpr int32 BlockSize = 64 * 1024;

    /** @brief Number of frames spanned by one low-water-mark window */
    static constexpr int32 PruneIntervalFrames = 60;

    /** @brief Upper bound on the pages retained between two prunes, 64 pages being 4 MB */
    static constexpr int32 MaxRetainedPages = 64;

    static FMemoryPagePool& Get();

public:
    void* AcquirePage(int32 InBlockSize);
    void  ReleasePage(void* Page, int32 InBlockSize);

    void Tick();
    void Flush();

    int32 GetNumFreePages()   const;
    int32 GetNumPooledBytes() const;

private:
    struct FFreePage
    {
        FFreePage* Next;
    };

    static void FreePageList(FFreePage* Pages);

    FMemoryPagePool();
    ~FMemoryPagePool();

    FFreePage* DetachPages(int32 NumPages);

    FFreePage*               FreeList;
    int32                    NumFreePages;
    int32                    LowWaterMark;
    int32                    FramesSincePrune;
    bool                     bPoolingEnabled;
    mutable FCriticalSection PoolCS;
};
