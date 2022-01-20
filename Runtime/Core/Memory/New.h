#pragma once
#include <new>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Enables memory-leak checking

#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
#define dbg_new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define dbg_new	new
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Memory operators

void* operator new  (size_t Size);
void* operator new[](size_t Size);
void* operator new  (size_t Size, const std::nothrow_t&) noexcept;
void* operator new[](size_t Size, const std::nothrow_t&) noexcept;

void operator delete  (void* Ptr) noexcept;
void operator delete[](void* Ptr) noexcept;
void operator delete  (void* Ptr, size_t) noexcept;
void operator delete[](void* Ptr, size_t) noexcept;
