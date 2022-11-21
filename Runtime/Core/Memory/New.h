#pragma once
#include <new>

void* operator new  (size_t Size);
void* operator new[](size_t Size);
void* operator new  (size_t Size, const std::nothrow_t&) noexcept;
void* operator new[](size_t Size, const std::nothrow_t&) noexcept;

void operator delete  (void* Ptr) noexcept;
void operator delete[](void* Ptr) noexcept;
void operator delete  (void* Ptr, size_t) noexcept;
void operator delete[](void* Ptr, size_t) noexcept;
