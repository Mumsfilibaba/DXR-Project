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

class CORE_API CMemory
{
public:

    /**
     * @brief: Allocate memory
     * 
     * @param Size: The number of bytes to allocate
     * @return: Returns the newly allocated memory
     */
    static void* Malloc(uint64 Size) noexcept;

    /**
     * @brief: Reallocate memory
     *
     * @param Ptr: Ptr to the memory to reallocate
     * @param Size: The number of bytes to reallocate
     * @return: Returns the newly allocated memory
     */
    static void* Realloc(void* Ptr, uint64 Size) noexcept;

    /**
     * @brief: Free memory
     *
     * @param Ptr: A pointer to the memory to deallocate
     */
    static void Free(void* Ptr) noexcept;

    /**
     * @brief: Move memory from one location to another. Can be overlapping memory-ranges.
     * 
     * @param Dst: Destination of the moved memory
     * @param Src: Src of the memory
     * @param Size: Size of the memory to move
     * @return: Returns the destination pointer
     */
    static void* Memmove(void* Dst, const void* Src, uint64 Size) noexcept;

    /**
     * @brief: Copy memory from one memory range to another
     *
     * @param Dst: Destination of the memory
     * @param Src: Src of the memory
     * @param Size: Size of the memory to copy
     * @return: Returns the destination pointer
     */
    static void* Memcpy(void* restrict_ptr Dst, const void* restrict_ptr Src, uint64 Size) noexcept;

    /**
     * @brief: Set memory to a byte value
     *
     * @param Dst: Destination of the memory to set
     * @param Value: Value to set each byte to 
     * @param Size: Size of the memory to set
     * @return: Returns the destination pointer
     */
    static void* Memset(void* Dst, uint8 Value, uint64 Size) noexcept;

    /**
     * @brief: Set memory to zero
     *
     * @param Dst: Destination of the memory to set
     * @param Size: Size of the memory to set
     * @return: Returns the destination pointer
     */
    static void* Memzero(void* Dst, uint64 Size) noexcept;

    // TODO: Check if we need all the information from memcmp and refactor in that case
    
    /**
     * @brief: Compare two memory ranges
     *
     * @param LHS: Memory range 1
     * @param RHS: Memory range 2
     * @param Size: Size of the memory ranges
     * @return: Returns true if the memory ranges are equal
     */
    static bool Memcmp(const void* LHS, const void* RHS, uint64 Size) noexcept;

    /**
     * @brief: Swaps the contents of two memory ranges
     *
     * @param LHS: Memory range 1
     * @param RHS: Memory range 2
     * @param Size: Size of the memory ranges
     */
    static void Memswap(void* restrict_ptr LHS, void* restrict_ptr RHS, uint64 Size) noexcept;

    /**
     * @brief: Copy memory range from one memory range to another and then set the source to zero
     * 
     * @param Dst: Destination memory range
     * @param Src: Src memory range
     * @param Size: Size of the memory range
     */
    static FORCEINLINE void Memexchange(void* restrict_ptr Dst, void* restrict_ptr Src, uint64 Size) noexcept
    {
        if (Dst != Src)
        {
            Memcpy(Dst, Src, Size);
            Memzero(Src, Size);
        }
    }

    /**
     * @brief: Free memory of const pointers
     *
     * @param Ptr: A pointer to the memory to deallocate
     */
    static FORCEINLINE void Free(const void* Ptr) noexcept
    {
        Free((void*)Ptr);
    }

    /**
     * @brief: Allocate memory typed
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
     * @brief: Reallocate memory typed
     *
     * @param Ptr: Ptr to the memory to reallocate
     * @param Count: Number of elements to allocate
     * @return: Returns the pointer
     */
    template<typename T>
    static FORCEINLINE T* Realloc(T* Ptr, uint64 Count) noexcept
    {
        const uint64 NumBytes = Count * sizeof(T);
        return reinterpret_cast<T*>(Realloc(reinterpret_cast<void*>(Ptr), NumBytes));
    }

    /**
     * @brief: Set memory to zero
     * 
     * @param Dst: Destination of memory to zero
     * @param SizeInBytes: Size of the memory zero
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dst, uint64 SizeInBytes) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dst), SizeInBytes));
    }

    /**
     * @brief: Set memory to zero
     *
     * @param Dst: Destination of memory to zero
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dst) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dst), sizeof(T)));
    }

    /**
     * @brief: Set memory to zero
     *
     * @param Dst: Destination of memory to zero
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memzero(T* Dst, uint32 NumElements) noexcept
    {
        return reinterpret_cast<T*>(Memzero(reinterpret_cast<void*>(Dst), sizeof(T) * NumElements));
    }

    /**
     * @brief: Copy memory typed
     *
     * @param Dst: Destination of memory to copy
     * @param Src: Src of the memory to copy
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* Memcpy(T* restrict_ptr Dst, const T* restrict_ptr Src) noexcept
    {
        return reinterpret_cast<T*>(Memcpy(reinterpret_cast<void*>(Dst), reinterpret_cast<const void*>(Src), sizeof(T)));
    }

    /**
     * @brief: Copy memory typed
     *
     * @param Dst: Destination of memory to copy
     * @param Src: Src of the memory to copy
     * @param NumElements: Number of elements to copy
     * @return: Returns the destination pointer
     */
    template<typename T>
    static FORCEINLINE T* MemcpyTyped(T* restrict_ptr Dst, const T* restrict_ptr Src, uint32 NumElements) noexcept
    {
        return reinterpret_cast<T*>(Memcpy(reinterpret_cast<void*>(Dst), reinterpret_cast<const void*>(Src), sizeof(T) * NumElements));
    }

    /**
     * @brief: Compare memory typed
     *
     * @param LHS: Destination of memory to copy
     * @param RHS: Src of the memory to copy
     * @param Count: Number of elements to compare
     * @return: Returns true if the memory is equal to each other
     */
    template<typename T>
    static FORCEINLINE bool Memcmp(const T* LHS, const T* RHS, uint64 Count) noexcept
    {
        return Memcmp(reinterpret_cast<const void*>(LHS), reinterpret_cast<const void*>(RHS), sizeof(T) * Count);
    }

    /**
     * @brief: Copy typed memory range from one memory range to another and then set the source to zero
     *
     * @param Dst: Destination memory range
     * @param Src: Src memory range
     */
    template<typename T>
    static FORCEINLINE T* Memexchange(void* restrict_ptr Dst, void* restrict_ptr Src) noexcept
    {
        return Memexchange(reinterpret_cast<void*>(Dst), reinterpret_cast<void*>(Src), sizeof(T));
    }
};
