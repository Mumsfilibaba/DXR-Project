#pragma once
#include <new>

#if defined(DEBUG_BUILD) && defined(PLATFORM_WINDOWS)
#define dbg_new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define dbg_new	new
#endif

void* operator new  (size_t Size);
void* operator new[]( size_t Size );
void* operator new  (size_t Size, const std::nothrow_t&) noexcept;
void* operator new[]( size_t Size, const std::nothrow_t& ) noexcept;

void operator delete  (void* Ptr) noexcept;
void operator delete[]( void* Ptr ) noexcept;
void operator delete  (void* Ptr, size_t) noexcept;
void operator delete[]( void* Ptr, size_t ) noexcept;
