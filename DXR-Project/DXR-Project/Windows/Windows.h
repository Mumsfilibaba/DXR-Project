#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>

#ifdef CreateWindow
	#undef CreateWindow
#endif