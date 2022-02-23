#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Standard alignment of memory-allocations

#define STANDARD_ALIGNMENT (__STDCPP_DEFAULT_NEW_ALIGNMENT__)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Conversion from mega-bytes to bytes

namespace NMemoryUtils
{
    template<typename T>
    inline constexpr T BytesToMegaBytes(T Bytes)
    {
        return Bytes / T(1024 * 1024);
    }

    template<typename T>
    inline constexpr T MegaBytesToBytes(T MegaBytes)
    {
        return MegaBytes * T(1024 * 1024);
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class handling memory

class CORE_API Memory
{
public:

    /**
     * Allocate memory
     * 
     * @param Size: The number of bytes to allocate
     * @return: Returns the newly allocated memory
     */
    static void* Malloc(uint64 Size) noexcept;

    /**
     * Reallocate memory
     *
     * @param Pointer: Pointer to the memory to reallocate
     * @param Size: The number of bytes to reallocate
     * @return: Returns the newly allocated memory
     */
    static void* Realloc(void* Pointer, uint64 Size) noexcept;

    /**
     * Free memory
     *
     * @param Pointer: A pointer to the memory to deallocate
     */
    static void Free(void* Pointer) noexcept;

    /**
     * Move memory from one location to another. Can be overlapping memory-ranges.
     * 
     * @param Dest: Destination of the moved memory
     * @param Source: Source of the memory
     * @param Size: Size of the memory to move
     * @return: Returns the destination pointer
     */
    static void* Memmove(void* Dest, const void* Source, uint64 Size) noexcept;

    /**
     * Copy memory from one memory range to another
     *
     * @param Dest: Destination of the memory
     * @param Source: Source of the memory
     * @param Size: Size of the memory to copy
     * @return: Returns the destination pointer
     */
    static void* Memcpy(void* restrict_ptr Dest, const void* restrict_ptr Source, uint64 Size) noexcept;

    /**
     * Set memory to a byte value
     *
     * @param Dest: Destination of the memory to set
     * @param Value: Value to set each byte to 
     * @param Size: Size of the memory to set
     * @return: Returns the destination pointer
     */
    static void* Memset(void* Dest, uint8 Value, uint64 Size) noexcept;

    /**
     * Set memory to zero
     *
     * @param Dest: Destination of the memory to set
     * @param Size: Size of the memory to set
     * @return: Returns the destination pointer
     */
    static void* Memzero(void* Dest, uint64 Size) noexcept;

    // TODO: Check if we need all the information from memcmp and refactor in that case
    
    /**
     * Compare two memory ranges
     *
     * @param Lhs: Memory range 1
     * @param Rhs: Memory range 2
     * @param Size: Size of the memory ranges
     * @return: Returns true if the memory ranges are equal
     */
    static bool Memcmp(const void* Lhs, const void* Rhs, uint64 Size) noexcept;

    /**
     * Swaps the contents of two memory ranges
     *
     * @param Lhs: Memory range 1
     * @param Rhs: Memory range 2
     * @param Size: Size of the memory ranges
     */
    static void Memswap(void* restrict_ptr Lhs, void* restrict_ptr Rhs, uint64 Size) noexcept;

    /**
     * Copy memory range from one memory range to another and then set the source to zero
     * 
     * @param Dest: Destination memory range
     * @param Source: Source memory range
     * @param Size: Size of the memory range
     */
    static FORCEINLINE void Memexchange(void* restrict_ptr Dest, void* restrict_ptr Source, uint64 Size) noexcept
    {
        if (Dest != Source)
        {
            Memcpy(Dest, Source, Size);
            Memzero(Source, Size);
        }
    }

    /**
     * Allocate memory typed
     * 
     * @param Count: Number of elements to allocate
     * @return: Returns of the newly allocated memory
     */
    template<typename T>
    static FORCEINLINE T* Malloc(uint32 Count) noexcept
    {
        return reinterpret_cast<T*>(Malloc(sizeof(T) * Count));
    }

    /**
     * Reallocate memory typed
     *
     * @param Pointer: Pointer to the memory to reallocate
     * @param Count: Number of elements to allocate
     * @return: Returns the pointer
     */
    template<typename T>
    static FORCEINLINE T* Realloc(T* Pointer, uint64 Count) noexcept
    {
        const uint64 NumBytes = Count * sizeof(T);
        return reinterpret_cast<T*>(Realloc(reinterpret_cast<void*>(Pointer), NumBytes));
    }

    /**
     * Set memory to zero
     * 
     * @param Dest: Destination of memory to zero
     * @param SizeInBytes: Size of the memory zero
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dest, uint64 SizeInBytes) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dest), SizeInBytes));
    }

    /**
     * Set memory to zero
     *
     * @param Dest: Destination of memory to zero
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dest) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dest), sizeof(T)));
    }

    /**
     * Copy memory typed
     *
     * @param Dest: Destination of memory to copy
     * @param Source: Source of the memory to copy
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memcpy(T* restrict_ptr Dest, const T* restrict_ptr Source) noexcept
    {
        return reinterpret_cast<T*>(Memcpy(reinterpret_cast<void*>(Dest), reinterpret_cast<const void*>(Source), sizeof(T)));
    }

    /**
     * Compare memory typed
     *
     * @param Lhs: Destination of memory to copy
     * @param Rhs: Source of the memory to copy
     * @param Count: Number of elements to compare
     * @return: Returns true if the memory is equal to each other
     */
    template<typename T>
    static FORCEINLINE bool Memcmp(const T* Lhs, const T* Rhs, uint64 Count) noexcept
    {
        return Memcmp(reinterpret_cast<const void*>(Lhs), reinterpret_cast<const void*>(Rhs), sizeof(T) * Count);
    }

    /**
     * Copy typed memory range from one memory range to another and then set the source to zero
     *
     * @param Dest: Destination memory range
     * @param Source: Source memory range
     */
    template<typename T>
    static FORCEINLINE T* Memexchange(void* restrict_ptr Dest, void* restrict_ptr Source) noexcept
    {
        return Memexchange(reinterpret_cast<void*>(Dest), reinterpret_cast<void*>(Source), sizeof(T));
    }
};
