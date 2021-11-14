#pragma once
#ifdef PLATFORM_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN (1)
#endif

#include <Windows.h>
#include <windowsx.h>
#include <sdkddkver.h>

#ifdef CreateWindow
#undef CreateWindow
#endif

#ifdef OutputDebugString
#undef OutputDebugString
#endif

#ifdef NTDDI_VISTA
#define PLATFORM_WINDOWS_VISTA (1)
#endif

#ifdef NTDDI_WIN10_RS1
#define PLATFORM_WINDOWS_10_ANNIVERSARY (1)
#endif

#endif