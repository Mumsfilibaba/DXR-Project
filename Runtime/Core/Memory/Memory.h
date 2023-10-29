#pragma once
#include "Core/Core.h"

struct CORE_API FMemory
{
    template<typename T>
    static constexpr T BytesToMegaBytes(T Bytes)
    {
        return Bytes / T(1024 * 1024);
    }

    template<typename T>
    static constexpr T MegaBytesToBytes(T MegaBytes)
    {
        return MegaBytes * T(1024 * 1024);
    }

    /**
     * @brief      - Allocate memory
     * @param Size - The number of bytes to allocate
     * @return     - Returns the newly allocated memory
     */
    static void* Malloc(uint64 Size) noexcept;

    /**
     * @brief       - Reallocate memory
     * @param Block - Block to the memory to reallocate
     * @param Size  - The number of bytes to reallocate
     * @return      - Returns the newly allocated memory
     */
    static void* Realloc(void* Block, uint64 Size) noexcept;

    /**
     * @brief       - Free memory
     * @param Block - A pointer to the memory to deallocate
     */
    static void Free(void* Block) noexcept;

    /**
	 * @brief       - Free memory of const pointers
	 * @param Block - A pointer to the memory to deallocate
	 */
    static FORCEINLINE void Free(const void* Block) noexcept
    {
        Free((void*)Block);
    }

    /**
     * @brief      - Allocate memory and zero it
     * @param Size - The number of bytes to allocate
     * @return     - Returns the newly allocated memory
     */
    static FORCEINLINE void* MallocZeroed(uint64 Size) noexcept
    {
        void* NewMemory = Malloc(Size);
        Memzero(NewMemory, Size);
        return NewMemory;
    }

    /**
     * @brief      - Move memory from one location to another. Can be overlapping memory-ranges.
     * @param Dst  - Destination of the moved memory
     * @param Src  - Src of the memory
     * @param Size - Size of the memory to move
     * @return     - Returns the destination pointer
     */
    static void* Memmove(void* Dst, const void* Src, uint64 Size) noexcept;

    /**
     * @brief      - Copy memory from one memory range to another
     * @param Dst  - Destination of the memory
     * @param Src  - Src of the memory
     * @param Size - Size of the memory to copy
     * @return     - Returns the destination pointer
     */
    static void* Memcpy(void* RESTRICT Dst, const void* RESTRICT Src, uint64 Size) noexcept;

    /**
     * @brief       - Set memory to a byte value
     * @param Dst   - Destination of the memory to set
     * @param Value - Value to set each byte to
     * @param Size  - Size of the memory to set
     * @return      - Returns the destination pointer
     */
    static void* Memset(void* Dst, uint8 Value, uint64 Size) noexcept;

    /**
     * @brief      - Set memory to zero
     * @param Dst  - Destination of the memory to set
     * @param Size - Size of the memory to set
     * @return     - Returns the destination pointer
     */
    static void* Memzero(void* Dst, uint64 Size) noexcept;

    /**
     * @brief      - Compare two memory ranges
     * @param LHS  - Memory range 1
     * @param RHS  - Memory range 2
     * @param Size - Size of the memory ranges
     * @return     - Returns the position that was not equal
     */
    static int32 Memcmp(const void* LHS, const void* RHS, uint64 Size) noexcept;

    /**
     * @brief      - Swaps the contents of two memory ranges
     * @param LHS  - First of the memory ranges to swap
     * @param RHS  - Second of the memory ranges to swap
     * @param Size - Size of the memory ranges
     */
    static void Memswap(void* RESTRICT LHS, void* RESTRICT RHS, uint64 Size) noexcept;

    /**
     * @brief      - Copy memory range from one memory range to another and then set the source to zero
     * @param Dst  - Destination memory range
     * @param Src  - Src memory range
     * @param Size - Size of the memory range
     */
    static FORCEINLINE void Memexchange(void* RESTRICT Dst, void* RESTRICT Src, uint64 Size) noexcept
    {
        if (Dst != Src)
        {
            Memcpy(Dst, Src, Size);
            Memzero(Src, Size);
        }
    }

    /**
     * @brief     - Set memory to zero
     * @param Dst - Destination of memory to zero
     * @return    - Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dst) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dst), sizeof(T)));
    }
};
