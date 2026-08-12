#pragma once
#include "Core/Core.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryPagePool.h"
#include "Core/Memory/MemoryStats.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

class CORE_API FMemoryStack : FNonCopyable
{
private:
    struct FMemoryHeader
    {
        void* Data()
        {
            return reinterpret_cast<uint8*>(this) + sizeof(FMemoryHeader);
        }

        FMemoryHeader* Next;
        int32          Size;
    };

public:

    /** @brief Usable bytes in an ordinary page, sized so that page plus header is one pool block */
    static constexpr int32 PageSize = FMemoryPagePool::BlockSize - int32(sizeof(FMemoryHeader));

    FMemoryStack() = default;

    explicit FMemoryStack(int32 Size) noexcept
        : TopPage(nullptr)
        , StackStart(nullptr)
        , StackEnd(nullptr)
    {
        AllocateNewChunk(Size);
    }
    
    FMemoryStack(FMemoryStack&& Other) noexcept
    {
        *this = Move(Other);
    }

    ~FMemoryStack() noexcept
    {
        FreeChunks(nullptr);
    }

    void* Allocate(int32 Size, int32 Alignment = STANDARD_ALIGNMENT) noexcept
    {
        CHECK(Size > 0);
        CHECK(Alignment > 0);

        const int32 AlignedSize = Math::AlignUp(Size, Alignment);
        
        uint8* AlignedAddress = reinterpret_cast<uint8*>(Math::AlignUp<UPTR_INT>(reinterpret_cast<UPTR_INT>(StackStart), Alignment));
        uint8* NewStart       = AlignedAddress + AlignedSize;
        if (NewStart > StackEnd)
        {
            // In case the new chunk needs to be aligned, pass the alignment as well as the size
            AllocateNewChunk(AlignedSize + Alignment);
            AlignedAddress = reinterpret_cast<uint8*>(Math::AlignUp<UPTR_INT>(reinterpret_cast<UPTR_INT>(StackStart), Alignment));
            NewStart       = AlignedAddress + AlignedSize;
        }

        StackStart = NewStart;
        return AlignedAddress;
    }

    uint8* PushBytes(int32 Size, int32 Alignment = STANDARD_ALIGNMENT) noexcept
    {
        return reinterpret_cast<uint8*>(Allocate(Size, Alignment));
    }

    void Reset() noexcept
    {
        FreeChunks(nullptr);
    }

    int32 GetNumAllocatedBytes() const
    {
        int32 Total = 0;
        for (FMemoryHeader* CurrentChunk = TopPage; CurrentChunk != nullptr; CurrentChunk = CurrentChunk->Next)
        {
            Total += CurrentChunk->Size;
        }

        return Total;
    }

    bool IsEmpty() const noexcept
    {
        return TopPage == nullptr;
    }

    FMemoryStack& operator=(FMemoryStack&& RHS) noexcept
    {
        TopPage    = RHS.TopPage;
        StackStart = RHS.StackStart;
        StackEnd   = RHS.StackEnd;
        RHS.TopPage    = nullptr;
        RHS.StackStart = nullptr;
        RHS.StackEnd   = nullptr;
        return *this;
    }

private:
    void* AllocateNewChunk(int32 MinSize)
    {
        const int32 HeapSize  = Math::Max(PageSize, MinSize);
        const int32 AllocSize = HeapSize + static_cast<int32>(sizeof(FMemoryHeader));
        
        FMemoryHeader* NewPage = reinterpret_cast<FMemoryHeader*>(FMemoryPagePool::Get().AcquirePage(AllocSize));
        NewPage->Size = HeapSize;
        
        if (TopPage)
        {
            NewPage->Next = TopPage;
        }
        else
        {
            NewPage->Next = nullptr;
        }

        StackStart = reinterpret_cast<uint8*>(NewPage->Data());
        StackEnd   = StackStart + HeapSize;
        TopPage    = NewPage;

        STAT_ADD(STAT_Memory_StackBytes, AllocSize);
        STAT_ADD(STAT_Memory_StackPageCount, 1);
        return StackStart;
    }

    void FreeChunks(FMemoryHeader* LastPage)
    {
        FMemoryHeader* CurrentChunk = TopPage;
        while (CurrentChunk != LastPage)
        {
            FMemoryHeader* PreviousChunk = CurrentChunk;
            const int32 ChunkSize = PreviousChunk->Size + static_cast<int32>(sizeof(FMemoryHeader));

            STAT_SUBTRACT(STAT_Memory_StackBytes, ChunkSize);
            STAT_SUBTRACT(STAT_Memory_StackPageCount, 1);

            CurrentChunk = CurrentChunk->Next;
            FMemoryPagePool::Get().ReleasePage(PreviousChunk, ChunkSize);
        }

        TopPage    = LastPage;
        StackStart = nullptr; // Reset the stack pointers since the current top-page is assumed to be in use (In most cases however, it is nullptr)
        StackEnd   = StackStart;
    }

    FMemoryHeader* TopPage    = nullptr;
    uint8*         StackStart = nullptr;
    uint8*         StackEnd   = nullptr;
};
