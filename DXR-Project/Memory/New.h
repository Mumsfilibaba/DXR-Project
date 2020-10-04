#pragma once
#include "Defines.h"
#include "Types.h"

#include <new>

#ifdef _DEBUG
	#define DBG_NEW	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
	#define DBG_NEW	new
#endif

VoidPtr operator new  (size_t Size);
VoidPtr operator new[](size_t Size);
VoidPtr operator new  (size_t Size, const std::nothrow_t&) noexcept;
VoidPtr operator new[](size_t Size, const std::nothrow_t&) noexcept;

void operator delete  (VoidPtr Ptr) noexcept;
void operator delete[](VoidPtr Ptr) noexcept;
void operator delete  (VoidPtr Ptr, size_t) noexcept;
void operator delete[](VoidPtr Ptr, size_t) noexcept;