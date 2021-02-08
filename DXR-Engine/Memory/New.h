#pragma once
#include <new>

#ifdef _DEBUG
    #define DBG_NEW	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
    #define DBG_NEW	new
#endif

void* operator new  (size_t Size);
void* operator new[](size_t Size);
void* operator new  (size_t Size, const std::nothrow_t&) noexcept;
void* operator new[](size_t Size, const std::nothrow_t&) noexcept;

void operator delete  (void* Ptr) noexcept;
void operator delete[](void* Ptr) noexcept;
void operator delete  (void* Ptr, size_t) noexcept;
void operator delete[](void* Ptr, size_t) noexcept;
