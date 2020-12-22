#pragma once
#include "Defines.h"
#include "Types.h"

#include <new>

#ifdef _DEBUG
	#define DBG_NEW	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
	#define DBG_NEW	new
#endif

/*
* New
*/

Void* operator new  (size_t sizeInBytes);
Void* operator new[](size_t sizeInBytes);
Void* operator new  (size_t sizeInBytes, const std::nothrow_t&) noexcept;
Void* operator new[](size_t sizeInBytes, const std::nothrow_t&) noexcept;

void operator delete  (Void* pPtr) noexcept;
void operator delete[](Void* pPtr) noexcept;
void operator delete  (Void* pPtr, size_t) noexcept;
void operator delete[](Void* pPtr, size_t) noexcept;
