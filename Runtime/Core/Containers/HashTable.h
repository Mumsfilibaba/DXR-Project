#pragma once
#include "Core/Containers/Allocators.h"
#include "Core/Containers/Pair.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/TypeHash.h"
#include "Core/Templates/Utility.h"

/** @brief Control byte marking a slot that holds no element */
static constexpr uint8 HashControlEmpty = 0;

/** @brief Control byte marking an occupied slot that is the last link of its chain */
static constexpr uint8 HashControlEnd = 255;

/** @brief Largest jump code that may be stored in a control byte, see HashJumpDistance */
static constexpr uint8 HashControlJumpMax = 254;

/** @brief Slot index returned by the lookup functions when a key is not present */
static constexpr int32 HashInvalidSlot = -1;

/** @brief Smallest number of slots the table ever allocates, always a power of two */
static constexpr int32 HashMinSlotCount = 8;

/** @brief Numerator of the maximum load factor (15/16, roughly 94% occupancy before growing) */
static constexpr uint32 HashLoadNumerator = 15;

/** @brief Denominator of the maximum load factor, see HashLoadNumerator */
static constexpr uint32 HashLoadDenominator = 16;

/**
 * @brief Applies Fibonacci mixing to a hash
 * @param Hash Hash to mix
 * @return Returns the mixed hash
 */
FORCEINLINE constexpr uint32 MixHash(uint64 Hash)
{
    return static_cast<uint32>(Hash * 0x9E3779B97F4A7C15ull);
}

/**
 * @brief Decodes a control byte into the signed slot distance to the next link of the chain
 * Codes 1-127 step forwards and codes 128-254 step backwards, which lets a chain reach slots on
 * both sides of its home slot while still only costing a single byte per slot.
 * 
 * @param JumpCode Jump code stored in a control byte
 * @return Returns the distance to the next slot in the chain
 */
FORCEINLINE constexpr int32 HashJumpDistance(uint8 JumpCode)
{
    return JumpCode <= 127 ? static_cast<int32>(JumpCode) : -static_cast<int32>(JumpCode - 127);
}

/**
 * @brief Calculates the number of slots needed to store a number of elements
 * @param ElementCount Number of elements that should fit without exceeding the maximum load factor
 * @return Returns a power-of-two slot count of at least HashMinSlotCount, or zero when ElementCount is zero or negative
 */
FORCEINLINE constexpr int32 HashSlotsForElements(int32 ElementCount)
{
    if (ElementCount <= 0)
    {
        return 0;
    }

    const uint64 Need = (static_cast<uint64>(ElementCount) * HashLoadDenominator + (HashLoadNumerator - 1)) / HashLoadNumerator;
    int32 Power = 1;
    while (static_cast<uint64>(Power) < Need)
    {
        Power <<= 1;
    }

    if (Power < HashMinSlotCount)
    {
        Power = HashMinSlotCount;
    }

    return Power;
}

template<typename InElementType>
struct TDefaultKeyFuncs
{
    typedef InElementType KeyType;

    /**
     * @brief Retrieve the key of an element
     * @param Element Element to retrieve the key from
     * @return Returns a reference to the key
     */
    static FORCEINLINE const KeyType& GetKey(const InElementType& Element)
    {
        return Element;
    }

    /**
     * @brief Compare two keys for equality
     * @param A First key to compare
     * @param B Second key to compare
     * @return Returns true if the keys are equal
     */
    static FORCEINLINE bool Matches(const KeyType& A, const KeyType& B)
    {
        return A == B;
    }

    /**
     * @brief Calculate the mixed hash of a key
     * @param Key Key to hash
     * @return Returns the hash to index the table with
     */
    static FORCEINLINE uint32 GetKeyHash(const KeyType& Key)
    {
        return MixHash(THash<KeyType>::GetHash(Key));
    }
};

template<typename InKeyType, typename InValueType>
struct TPairKeyFuncs
{
    typedef InKeyType                     KeyType;
    typedef TPair<InKeyType, InValueType> ElementType;

    /**
     * @brief Retrieve the key of a pair
     * @param Element Pair to retrieve the key from
     * @return Returns a reference to the key of the pair
     */
    static FORCEINLINE const KeyType& GetKey(const ElementType& Element)
    {
        return Element.First;
    }

    /**
     * @brief Compare two keys for equality
     * @param A First key to compare
     * @param B Second key to compare
     * @return Returns true if the keys are equal
     */
    static FORCEINLINE bool Matches(const KeyType& A, const KeyType& B)
    {
        return A == B;
    }

    /**
     * @brief Calculate the mixed hash of a key
     * @param Key Key to hash
     * @return Returns the hash to index the table with
     */
    static FORCEINLINE uint32 GetKeyHash(const KeyType& Key)
    {
        return MixHash(THash<KeyType>::GetHash(Key));
    }
};

template<typename InElementType, typename KeyFuncs, bool bAllowDuplicateKeys, typename AllocatorType = TDefaultHashTableAllocator<InElementType>>
class THashTable
{
    template<typename MapType, typename KeyType, typename ValueType>
    friend class TMapIterator;

    template<typename SetType, typename ElementType>
    friend class TSetIterator;

public:
    typedef InElementType              ElementType;
    typedef typename KeyFuncs::KeyType KeyType;
    typedef int32                      SizeType;

    /** @brief Default constructor, creates an empty table without allocating any slots */
    THashTable() = default;

    /**
     * @brief Copy-constructor
     * @param Other Table to copy the elements from
     */
    THashTable(const THashTable& Other)
    {
        CopyFrom(Other);
    }

    /**
     * @brief Move-constructor
     * @param Other Table to move the elements from
     */
    THashTable(THashTable&& Other) noexcept
    {
        MoveFrom(Move(Other));
    }

    /** @brief Destructor, destroys all elements and releases the storage */
    ~THashTable()
    {
        DestroySlots();
        FreeStorage();
    }

    /**
     * @brief Copy-assignment
     * @param Other Table to copy the elements from
     * @return Returns a reference to this table
     */
    THashTable& operator=(const THashTable& Other)
    {
        if (this != &Other)
        {
            Reset();
            CopyFrom(Other);
        }

        return *this;
    }

    /**
     * @brief Move-assignment
     * @param Other Table to move the elements from
     * @return Returns a reference to this table
     */
    THashTable& operator=(THashTable&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset();
            MoveFrom(Move(Other));
        }

        return *this;
    }

    /**
     * @brief Retrieve the number of elements in the table
     * @return Returns the number of elements
     */
    NODISCARD SizeType Size() const
    {
        return NumElements;
    }

    /**
     * @brief Check if the table is empty
     * @return Returns true if the table contains no elements
     */
    NODISCARD bool IsEmpty() const
    {
        return NumElements == 0;
    }

    /**
     * @brief Retrieve the number of slots in the table
     * @return Returns the number of slots
     */
    NODISCARD SizeType BucketCount() const
    {
        return SlotCount;
    }

    /**
     * @brief Retrieve the number of slots in the table
     * @return Returns the number of slots
     */
    NODISCARD SizeType Capacity() const
    {
        return SlotCount;
    }

    /**
     * @brief Retrieve the current ratio of elements to slots
     * @return Returns the load factor, or zero if no slots are allocated
     */
    NODISCARD float LoadFactor() const
    {
        return SlotCount > 0 ? static_cast<float>(NumElements) / static_cast<float>(SlotCount) : 0.0f;
    }

    /**
     * @brief Retrieve the load factor at which the table grows
     * @return Returns the maximum load factor
     */
    NODISCARD float MaxLoadFactor() const
    {
        return static_cast<float>(HashLoadNumerator) / static_cast<float>(HashLoadDenominator);
    }

    /**
     * @brief Retrieve the number of bytes allocated on the heap by the table
     * @return Returns the allocated size in bytes, or zero while the storage is inline
     */
    NODISCARD uint64 GetAllocatedSize() const
    {
        return Allocator.IsHeapAllocated() ? static_cast<uint64>(SlotCount) * (sizeof(ElementType) + sizeof(uint8)) : 0;
    }

    /**
     * @brief Check if the storage currently lives on the heap
     * @return Returns true if the allocator has allocated heap storage, false while the storage is inline
     */
    NODISCARD bool IsHeapAllocated() const
    {
        return Allocator.IsHeapAllocated();
    }

    /**
     * @brief Retrieve the largest number of elements the table can address
     * @return Returns the maximum number of elements
     */
    NODISCARD SizeType MaxSize() const
    {
        return TNumericLimits<SizeType>::Max();
    }

    /**
     * @brief Find the first element with the specified key
     * @param Key Key to look for
     * @return Returns a pointer to the element, or nullptr if the key is not in the table
     */
    NODISCARD ElementType* Find(const KeyType& Key)
    {
        const SizeType Slot = FindSlot(Key);
        return Slot != HashInvalidSlot ? &Slots[Slot] : nullptr;
    }

    /**
     * @brief Find the first element with the specified key
     * @param Key Key to look for
     * @return Returns a pointer to the element, or nullptr if the key is not in the table
     */
    NODISCARD const ElementType* Find(const KeyType& Key) const
    {
        const SizeType Slot = FindSlot(Key);
        return Slot != HashInvalidSlot ? &Slots[Slot] : nullptr;
    }

    /**
     * @brief Find the slot of the first element with the specified key
     * @param Key Key to look for
     * @return Returns the slot index, or HashInvalidSlot if the key is not in the table
     */
    NODISCARD SizeType FindSlot(const KeyType& Key) const
    {
        if (SlotCount == 0)
        {
            return HashInvalidSlot;
        }

        SizeType Index = HomeIndex(Key);
        if (Control[Index] == HashControlEmpty)
        {
            return HashInvalidSlot;
        }

        for (SizeType Probe = 0; Probe < SlotCount; ++Probe)
        {
            if (KeyFuncs::Matches(KeyFuncs::GetKey(Slots[Index]), Key))
            {
                return Index;
            }

            if (Control[Index] == HashControlEnd)
            {
                return HashInvalidSlot;
            }

            const SizeType Next = Advance(Index, Control[Index]);
            if (Next == Index)
            {
                return HashInvalidSlot;
            }

            Index = Next;
        }

        return HashInvalidSlot;
    }

    /**
     * @brief Find the slot of the next element with the specified key, used to walk duplicates
     * @param Key Key to look for
     * @param AfterSlot Slot to continue the search from, or HashInvalidSlot to start from the beginning
     * @return Returns the slot index of the next match, or HashInvalidSlot if there are no more matches
     */
    NODISCARD SizeType FindSlotAfter(const KeyType& Key, SizeType AfterSlot) const
    {
        if (SlotCount == 0 || AfterSlot == HashInvalidSlot)
        {
            return FindSlot(Key);
        }

        if (Control[AfterSlot] == HashControlEnd)
        {
            return HashInvalidSlot;
        }

        SizeType Index = Advance(AfterSlot, Control[AfterSlot]);
        for (SizeType Probe = 0; Probe < SlotCount; ++Probe)
        {
            if (KeyFuncs::Matches(KeyFuncs::GetKey(Slots[Index]), Key))
            {
                return Index;
            }

            if (Control[Index] == HashControlEnd)
            {
                return HashInvalidSlot;
            }

            const SizeType Next = Advance(Index, Control[Index]);
            if (Next == Index)
            {
                return HashInvalidSlot;
            }

            Index = Next;
        }

        return HashInvalidSlot;
    }

    /**
     * @brief Insert an element, or return the existing element when the key is already present
     *
     * When bAllowDuplicateKeys is true this always inserts, since duplicates are allowed.
     *
     * @param Pair Element to insert
     * @return Returns a reference to the inserted element, or to the element that already had the key
     */
    template<typename PairLike>
    ElementType& InsertUniqueOrGet(PairLike&& Pair)
    {
        const KeyType& Key = KeyFuncs::GetKey(Pair);
        if constexpr (!bAllowDuplicateKeys)
        {
            if (ElementType* Existing = Find(Key))
            {
                return *Existing;
            }
        }

        return InsertAlways(Forward<PairLike>(Pair));
    }

    /**
     * @brief Insert an element without checking for an existing key
     *
     * Grows and rehashes the table when the insertion would exceed the maximum load factor, or when
     * no reachable slot is free for the chain of the key.
     *
     * @param Pair Element to insert
     * @return Returns a reference to the inserted element
     */
    template<typename PairLike>
    ElementType& InsertAlways(PairLike&& Pair)
    {
        EnsureCapacity(NumElements + 1);

        for (uint32 Attempt = 0; Attempt < 8; ++Attempt)
        {
            const SizeType Slot = TryClaimSlot(KeyFuncs::GetKey(Pair));
            if (Slot != HashInvalidSlot)
            {
                new (static_cast<void*>(&Slots[Slot])) ElementType(Forward<PairLike>(Pair));
                ++NumElements;
                return Slots[Slot];
            }

            RehashTo(SlotCount == 0 ? HashMinSlotCount : SlotCount * 2);
        }

        CHECK(false);
        return *Slots;
    }

    /**
     * @brief Remove the element in the specified slot and repair the chain it belonged to
     * @param Slot Slot to remove the element from
     * @return Returns true if an element was removed, false if the slot was empty or out of range
     */
    bool RemoveSlot(SizeType Slot)
    {
        if (Slot == HashInvalidSlot || Slot >= SlotCount || Control[Slot] == HashControlEmpty)
        {
            return false;
        }

        if (Control[Slot] != HashControlEnd)
        {
            const SizeType Next = Advance(Slot, Control[Slot]);
            uint8 NewControl = HashControlEnd;
            if (Control[Next] != HashControlEnd)
            {
                const SizeType NextNext = Advance(Next, Control[Next]);
                if (!FindJumpCode(Slot, NextNext, NewControl))
                {
                    return RemoveSlotByRebuild(Slot);
                }
            }

            DestroyObject(&Slots[Slot]);
            RelocateObjects(&Slots[Slot], &Slots[Next], SizeType(1));
            Control[Slot] = NewControl;
            Control[Next] = HashControlEmpty;
        }
        else
        {
            const SizeType Home = HomeIndex(KeyFuncs::GetKey(Slots[Slot]));
            DestroyObject(&Slots[Slot]);
            Control[Slot] = HashControlEmpty;

            if (Home != Slot && Home >= 0 && Home < SlotCount && Control[Home] != HashControlEmpty)
            {
                SizeType Prev  = Home;
                SizeType Index = Home;
                bool     bFound = false;
                for (SizeType Probe = 0; Probe < SlotCount; ++Probe)
                {
                    if (Index == Slot)
                    {
                        bFound = true;
                        break;
                    }

                    if (Control[Index] == HashControlEnd || Control[Index] == HashControlEmpty)
                    {
                        break;
                    }

                    Prev              = Index;
                    const SizeType Next = Advance(Index, Control[Index]);
                    if (Next == Index)
                    {
                        break;
                    }

                    Index = Next;
                }

                if (bFound)
                {
                    Control[Prev] = HashControlEnd;
                }
            }
        }

        --NumElements;
        return true;
    }

    /**
     * @brief Remove a single element with the specified key
     * @param Key Key of the element to remove
     * @return Returns true if an element was removed
     */
    bool RemoveOne(const KeyType& Key)
    {
        return RemoveSlot(FindSlot(Key));
    }

    /**
     * @brief Remove every element with the specified key
     * @param Key Key of the elements to remove
     * @return Returns the number of removed elements
     */
    SizeType RemoveAll(const KeyType& Key)
    {
        SizeType Removed = 0;
        while (RemoveOne(Key))
        {
            ++Removed;
            if constexpr (!bAllowDuplicateKeys)
            {
                break;
            }
        }

        return Removed;
    }

    /** @brief Destroy all elements, but keep the allocated slots for reuse */
    void Clear()
    {
        DestroySlots();
        if (Control)
        {
            Memory::Memzero(Control, static_cast<uint64>(SlotCount));
        }

        NumElements = 0;
    }

    /** @brief Destroy all elements and release the storage, returning the table to its default state */
    void Reset()
    {
        DestroySlots();
        FreeStorage();
        Slots       = nullptr;
        Control     = nullptr;
        SlotCount   = 0;
        HashMask    = 0;
        NumElements = 0;
    }

    /**
     * @brief Grow the table so that the specified number of elements fits without rehashing
     * @param ElementCount Number of elements to make room for
     */
    void Reserve(SizeType ElementCount)
    {
        const SizeType RequiredSlots = SlotsForElements(ElementCount);
        if (RequiredSlots > SlotCount)
        {
            RehashTo(RequiredSlots);
        }
    }

    /**
     * @brief Rehash the table into at least the specified number of slots
     * @param MinSlotCount Minimum number of slots, rounded up to a power of two of at least HashMinSlotCount
     */
    void Rehash(SizeType MinSlotCount)
    {
        SizeType NewCount = SlotCount == 0 ? HashMinSlotCount : SlotCount;
        while (NewCount < MinSlotCount)
        {
            NewCount *= 2;
        }

        if (NewCount < HashMinSlotCount)
        {
            NewCount = HashMinSlotCount;
        }

        RehashTo(NewCount);
    }

    /**
     * @brief Check if the specified slot holds an element
     * @param Index Slot to check
     * @return Returns true if the slot is occupied
     */
    NODISCARD bool IsOccupied(SizeType Index) const
    {
        return Index >= 0 && Index < SlotCount && Control[Index] != HashControlEmpty;
    }

    /**
     * @brief Retrieve the element in the specified slot, which must be occupied
     * @param Index Slot to retrieve the element from
     * @return Returns a reference to the element
     */
    NODISCARD ElementType& GetElement(SizeType Index)
    {
        return Slots[Index];
    }

    /**
     * @brief Retrieve the element in the specified slot, which must be occupied
     * @param Index Slot to retrieve the element from
     * @return Returns a reference to the element
     */
    NODISCARD const ElementType& GetElement(SizeType Index) const
    {
        return Slots[Index];
    }

    /**
     * @brief Retrieve the first occupied slot, which is where iteration starts
     * @return Returns the slot index, or BucketCount if the table is empty
     */
    NODISCARD SizeType FirstOccupied() const
    {
        return NextOccupied(HashInvalidSlot);
    }

    /**
     * @brief Retrieve the first occupied slot after the specified slot
     * @param Index Slot to search from, exclusive
     * @return Returns the slot index, or BucketCount if there are no more occupied slots
     */
    NODISCARD SizeType NextOccupied(SizeType Index) const
    {
        const SizeType Start = Index + 1;
        for (SizeType Next = Start; Next < SlotCount; ++Next)
        {
            if (Control[Next] != HashControlEmpty)
            {
                return Next;
            }
        }

        return SlotCount;
    }

    /**
     * @brief Retrieve the last occupied slot before the specified slot
     * @param Index Slot to search from, exclusive
     * @return Returns the slot index, or HashInvalidSlot if there are no occupied slots before it
     */
    NODISCARD SizeType PrevOccupied(SizeType Index) const
    {
        SizeType Prev = (Index > SlotCount ? SlotCount : Index) - 1;
        for (; Prev >= 0; --Prev)
        {
            if (Control[Prev] != HashControlEmpty)
            {
                return Prev;
            }
        }

        return HashInvalidSlot;
    }

    /**
     * @brief Count the elements that compare equal to the specified element
     * @param Element Element to compare against
     * @return Returns the number of equal elements
     */
    NODISCARD SizeType CountEqual(const ElementType& Element) const
    {
        SizeType Count     = 0;
        const KeyType& Key = KeyFuncs::GetKey(Element);
        SizeType Index     = FindSlot(Key);

        while (Index != HashInvalidSlot)
        {
            if (Slots[Index] == Element)
            {
                ++Count;
            }

            Index = FindSlotAfter(Key, Index);
        }

        return Count;
    }

    /**
     * @brief Compare the contents of two tables, ignoring the order the elements are stored in
     * @param Other Table to compare against
     * @return Returns true if both tables contain the same elements the same number of times
     */
    NODISCARD bool EqualTo(const THashTable& Other) const
    {
        if (NumElements != Other.NumElements)
        {
            return false;
        }

        for (SizeType Index = 0; Index < SlotCount; ++Index)
        {
            if (Control[Index] == HashControlEmpty)
            {
                continue;
            }

            if (CountEqual(Slots[Index]) != Other.CountEqual(Slots[Index]))
            {
                return false;
            }
        }

        return true;
    }

private:
    NODISCARD static SizeType SlotsForElements(SizeType ElementCount)
    {
        return HashSlotsForElements(ElementCount);
    }

    NODISCARD SizeType HomeIndex(const KeyType& Key) const
    {
        return static_cast<SizeType>(KeyFuncs::GetKeyHash(Key) & HashMask);
    }

    NODISCARD SizeType Advance(SizeType Index, uint8 JumpCode) const
    {
        return static_cast<SizeType>((static_cast<uint32>(Index) + static_cast<uint32>(HashJumpDistance(JumpCode))) & HashMask);
    }

    void EnsureCapacity(SizeType RequiredElements)
    {
        if (SlotCount == 0 || static_cast<uint64>(RequiredElements) * HashLoadDenominator > static_cast<uint64>(SlotCount) * HashLoadNumerator)
        {
            RehashTo(SlotsForElements(RequiredElements));
        }
    }

    NODISCARD bool FindJumpCode(SizeType FromIndex, SizeType ToIndex, uint8& OutJumpCode) const
    {
        const uint32 Forward = (static_cast<uint32>(ToIndex) - static_cast<uint32>(FromIndex)) & HashMask;
        if (Forward >= 1 && Forward <= 127)
        {
            OutJumpCode = static_cast<uint8>(Forward);
            return true;
        }

        const uint32 Backward = (static_cast<uint32>(FromIndex) - static_cast<uint32>(ToIndex)) & HashMask;
        if (Backward >= 1 && Backward <= 127)
        {
            OutJumpCode = static_cast<uint8>(127 + Backward);
            return true;
        }

        return false;
    }

    bool DisplaceOccupant(SizeType Slot)
    {
        const SizeType ForeignHome = HomeIndex(KeyFuncs::GetKey(Slots[Slot]));
        CHECK(ForeignHome != Slot);

        SizeType Prev   = ForeignHome;
        SizeType Index  = ForeignHome;
        bool     bFound = false;

        for (SizeType Probe = 0; Probe < SlotCount; ++Probe)
        {
            if (Index == Slot)
            {
                bFound = true;
                break;
            }

            if (Control[Index] == HashControlEnd || Control[Index] == HashControlEmpty)
            {
                break;
            }

            Prev = Index;
            const SizeType Next = Advance(Index, Control[Index]);
            if (Next == Index)
            {
                break;
            }

            Index = Next;
        }

        if (!bFound)
        {
            return false;
        }

        const uint8    OldControl = Control[Slot];
        const SizeType OldNext    = OldControl != HashControlEnd ? Advance(Slot, OldControl) : HashInvalidSlot;

        uint8    PrevJump = 0;
        uint8    NextJump = HashControlEnd;
        SizeType NewSlot  = HashInvalidSlot;

        for (uint8 CandidateJump = 1; CandidateJump <= HashControlJumpMax; ++CandidateJump)
        {
            const SizeType Candidate = Advance(Prev, CandidateJump);
            if (Candidate == Slot || Control[Candidate] != HashControlEmpty)
            {
                continue;
            }

            uint8 CandidateNextJump = HashControlEnd;
            if (OldNext != HashInvalidSlot && !FindJumpCode(Candidate, OldNext, CandidateNextJump))
            {
                continue;
            }

            PrevJump = CandidateJump;
            NextJump = CandidateNextJump;
            NewSlot  = Candidate;
            break;
        }

        if (NewSlot == HashInvalidSlot)
        {
            return false;
        }

        RelocateObjects(&Slots[NewSlot], &Slots[Slot], SizeType(1));

        Control[NewSlot] = NextJump;
        Control[Prev]    = PrevJump;
        Control[Slot]    = HashControlEmpty;
        return true;
    }

    SizeType TryInsertAfterHome(SizeType Home, const uint8* ReservedHomes)
    {
        const uint8    OldControl = Control[Home];
        const SizeType OldNext    = OldControl != HashControlEnd ? Advance(Home, OldControl) : HashInvalidSlot;

        for (uint8 HomeJump = 1; HomeJump <= HashControlJumpMax; ++HomeJump)
        {
            const SizeType Candidate = Advance(Home, HomeJump);
            if (Candidate == Home || Control[Candidate] != HashControlEmpty)
            {
                continue;
            }

            if (ReservedHomes && ReservedHomes[Candidate] != 0)
            {
                continue;
            }

            uint8 NextJump = HashControlEnd;
            if (OldNext != HashInvalidSlot && !FindJumpCode(Candidate, OldNext, NextJump))
            {
                continue;
            }

            Control[Home]      = HomeJump;
            Control[Candidate] = NextJump;
            return Candidate;
        }

        return HashInvalidSlot;
    }

    SizeType TryAppendToChain(SizeType Home, const uint8* ReservedHomes)
    {
        SizeType Tail = Home;
        for (SizeType Probe = 0; Probe < SlotCount; ++Probe)
        {
            if (Control[Tail] == HashControlEnd)
            {
                break;
            }

            const SizeType Next = Advance(Tail, Control[Tail]);
            if (Next == Tail)
            {
                return HashInvalidSlot;
            }

            Tail = Next;
        }

        for (uint8 JumpCode = 1; JumpCode <= HashControlJumpMax; ++JumpCode)
        {
            const SizeType Candidate = Advance(Tail, JumpCode);
            if (Candidate == Tail || Control[Candidate] != HashControlEmpty)
            {
                continue;
            }

            if (ReservedHomes && ReservedHomes[Candidate] != 0)
            {
                continue;
            }

            Control[Tail]      = JumpCode;
            Control[Candidate] = HashControlEnd;
            return Candidate;
        }

        return HashInvalidSlot;
    }

    SizeType TryClaimSlot(const KeyType& Key)
    {
        SizeType Home = HomeIndex(Key);
        if (Control[Home] == HashControlEmpty)
        {
            Control[Home] = HashControlEnd;
            return Home;
        }

        if (HomeIndex(KeyFuncs::GetKey(Slots[Home])) != Home)
        {
            if (!DisplaceOccupant(Home))
            {
                return HashInvalidSlot;
            }

            Control[Home] = HashControlEnd;
            return Home;
        }

        const SizeType Inserted = TryInsertAfterHome(Home, nullptr);
        return Inserted != HashInvalidSlot ? Inserted : TryAppendToChain(Home, nullptr);
    }

    SizeType TryClaimHome(SizeType Home, const uint8* ReservedHomes)
    {
        if (Control[Home] == HashControlEmpty)
        {
            Control[Home] = HashControlEnd;
            return Home;
        }

        const SizeType Inserted = TryInsertAfterHome(Home, ReservedHomes);
        return Inserted != HashInvalidSlot ? Inserted : TryAppendToChain(Home, ReservedHomes);
    }

    void RehashTo(SizeType NewSlotCount)
    {
        if (NewSlotCount < HashMinSlotCount)
        {
            NewSlotCount = HashMinSlotCount;
        }

        if (NumElements == 0)
        {
            DestroySlots();
            FreeStorage();
            AllocateStorage(NewSlotCount);
            NumElements = 0;
            return;
        }

        if (NewSlotCount <= SlotCount)
        {
            return;
        }

        ElementType* const OldSlots   = Slots;
        uint8* const       OldControl = Control;
        const SizeType     OldCount   = SlotCount;
        const SizeType     OldNum     = NumElements;
        const uint32       OldMask    = HashMask;

        for (;;)
        {
            AllocatorType NewAllocator;
            NewAllocator.Allocate(NewSlotCount);
            BindStorage(NewAllocator, NewSlotCount, true);
            NumElements = 0;

            uint8* const    ReservedHomes = static_cast<uint8*>(Memory::Malloc(static_cast<uint64>(NewSlotCount)));
            SizeType* const DestSlots     = static_cast<SizeType*>(Memory::Malloc(static_cast<uint64>(OldCount) * sizeof(SizeType)));
            Memory::Memzero(ReservedHomes, static_cast<uint64>(NewSlotCount));

            for (SizeType Index = 0; Index < OldCount; ++Index)
            {
                DestSlots[Index] = HashInvalidSlot;
                if (OldControl[Index] != HashControlEmpty)
                {
                    const SizeType Home = static_cast<SizeType>(KeyFuncs::GetKeyHash(KeyFuncs::GetKey(OldSlots[Index])) & HashMask);
                    ReservedHomes[Home] = 1;
                }
            }

            bool bClaimedAll = true;
            for (SizeType Index = 0; Index < OldCount; ++Index)
            {
                if (OldControl[Index] == HashControlEmpty)
                {
                    continue;
                }

                const SizeType Home = static_cast<SizeType>(KeyFuncs::GetKeyHash(KeyFuncs::GetKey(OldSlots[Index])) & HashMask);
                const SizeType Slot = TryClaimHome(Home, ReservedHomes);
                if (Slot == HashInvalidSlot)
                {
                    bClaimedAll = false;
                    break;
                }

                DestSlots[Index] = Slot;
            }

            Memory::Free(ReservedHomes);

            if (!bClaimedAll)
            {
                Memory::Free(DestSlots);
                NewAllocator.Free();

                Slots         = OldSlots;
                Control       = OldControl;
                SlotCount     = OldCount;
                NumElements   = OldNum;
                HashMask      = OldMask;
                NewSlotCount *= 2;
                continue;
            }

            for (SizeType Index = 0; Index < OldCount; ++Index)
            {
                if (DestSlots[Index] != HashInvalidSlot)
                {
                    RelocateObjects(&Slots[DestSlots[Index]], &OldSlots[Index], SizeType(1));
                    ++NumElements;
                }
            }

            Memory::Free(DestSlots);
            Allocator.Free();
            Allocator.MoveFrom(Move(NewAllocator), NewSlotCount);
            BindStorage(Allocator, NewSlotCount, false);
            return;
        }
    }

    bool RemoveSlotByRebuild(SizeType RemovedSlot)
    {
        ElementType* const OldSlots   = Slots;
        uint8* const       OldControl = Control;
        const SizeType     OldCount   = SlotCount;

        AllocatorType NewAllocator;
        NewAllocator.Allocate(OldCount);
        BindStorage(NewAllocator, OldCount, true);
        NumElements = 0;

        for (SizeType Index = 0; Index < OldCount; ++Index)
        {
            if (OldControl[Index] == HashControlEmpty)
            {
                continue;
            }

            if (Index == RemovedSlot)
            {
                DestroyObject(&OldSlots[Index]);
                continue;
            }

            const SizeType Slot = TryClaimSlot(KeyFuncs::GetKey(OldSlots[Index]));
            CHECK(Slot != HashInvalidSlot);
            RelocateObjects(&Slots[Slot], &OldSlots[Index], SizeType(1));
            ++NumElements;
        }

        Allocator.Free();
        Allocator.MoveFrom(Move(NewAllocator), OldCount);
        BindStorage(Allocator, OldCount, false);
        return true;
    }

    void BindStorage(AllocatorType& InAllocator, SizeType NewSlotCount, bool bClearControl)
    {
        Slots     = InAllocator.GetSlots(NewSlotCount);
        Control   = InAllocator.GetControl(NewSlotCount);
        SlotCount = NewSlotCount;
        HashMask  = static_cast<uint32>(NewSlotCount - 1);

        CHECK(Slots != nullptr);
        CHECK(Control != nullptr);

        if (bClearControl)
        {
            Memory::Memzero(Control, static_cast<uint64>(NewSlotCount));
        }
    }

    void AllocateStorage(SizeType NewSlotCount)
    {
        CHECK(NewSlotCount > 0);
        Allocator.Allocate(NewSlotCount);
        BindStorage(Allocator, NewSlotCount, true);
    }

    void FreeStorage()
    {
        Allocator.Free();
        Slots   = nullptr;
        Control = nullptr;
    }

    void DestroySlots()
    {
        for (SizeType Index = 0; Index < SlotCount; ++Index)
        {
            if (Control[Index] != HashControlEmpty)
            {
                DestroyObject(&Slots[Index]);
            }
        }
    }

    void CopyFrom(const THashTable& Other)
    {
        if (Other.NumElements == 0)
        {
            return;
        }

        Reserve(Other.NumElements);
        for (SizeType Index = 0; Index < Other.SlotCount; ++Index)
        {
            if (Other.Control[Index] != HashControlEmpty)
            {
                InsertAlways(Other.Slots[Index]);
            }
        }
    }

    void MoveFrom(THashTable&& Other)
    {
        Allocator.MoveFrom(Move(Other.Allocator), Other.SlotCount);
        Slots       = Allocator.GetSlots(Other.SlotCount);
        Control     = Allocator.GetControl(Other.SlotCount);
        SlotCount   = Other.SlotCount;
        HashMask    = Other.HashMask;
        NumElements = Other.NumElements;

        Other.Slots       = nullptr;
        Other.Control     = nullptr;
        Other.SlotCount   = 0;
        Other.HashMask    = 0;
        Other.NumElements = 0;
    }

    AllocatorType Allocator;
    ElementType*  Slots       = nullptr;
    uint8*        Control     = nullptr;
    uint32        HashMask    = 0;
    SizeType      SlotCount   = 0;
    SizeType      NumElements = 0;
};
