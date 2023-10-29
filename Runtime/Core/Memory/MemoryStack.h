#pragma once
#include "Memory.h"
#include "Core/Core.h"
#include "Core/Math/Math.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

// Currently each page is 64Kb
#define MEMORY_STACK_PAGE_SIZE   int32(64 * 1024)
#define MEMORY_STACK_ZERO_MEMORY (1)

class CORE_API FMemoryStack : FNonCopyable
{
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
        CHECK(Size      > 0);
        CHECK(Alignment > 0);

        const int32 AlignedSize = FMath::AlignUp(Size, Alignment);
        
        uint8* AlignedAddress = reinterpret_cast<uint8*>(FMath::AlignUp<uintptr>(reinterpret_cast<uintptr>(StackStart), Alignment));
        uint8* NewStart       = AlignedAddress + AlignedSize;
        if (NewStart >= StackEnd)
        {
            // In case the new chunk needs to be aligned, pass the alignment as well as the size
            AllocateNewChunk(AlignedSize + Alignment);
            AlignedAddress = reinterpret_cast<uint8*>(FMath::AlignUp<uintptr>(reinterpret_cast<uintptr>(StackStart), Alignment));
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
        return TopPage != nullptr;
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
        const int32 PageSize  = FMath::Max(MEMORY_STACK_PAGE_SIZE, MinSize);
        const int32 AllocSize = PageSize + sizeof(FMemoryHeader);
        
        FMemoryHeader* NewPage = reinterpret_cast<FMemoryHeader*>(FMemory::Malloc(AllocSize));
        NewPage->Size = PageSize;
        
        if (TopPage)
        {
            NewPage->Next = TopPage;
        }
        else
        {
            NewPage->Next = nullptr;
        }

        StackStart = reinterpret_cast<uint8*>(NewPage->Data());
        StackEnd   = StackStart + PageSize;
        TopPage    = NewPage;
        return StackStart;
    }

    void FreeChunks(FMemoryHeader* LastPage)
    {
        FMemoryHeader* CurrentChunk = TopPage;
        while (CurrentChunk != LastPage)
        {
            FMemoryHeader* PreviousChunk = CurrentChunk;
            CurrentChunk = CurrentChunk->Next;
            FMemory::Free(PreviousChunk);
        }

        TopPage = LastPage;

        // Reset the stack pointers since the current top-page is assumed to be in use (In most cases however, it is nullptr)
        StackStart = nullptr;
        StackEnd   = StackStart;
    }

    FMemoryHeader* TopPage{nullptr};
    uint8*         StackStart{nullptr};
    uint8*         StackEnd{nullptr};
};