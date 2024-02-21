#pragma once
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN (1)
#endif

#include <Windows.h>
#include <windowsx.h>
#include <sdkddkver.h>
#include <shlwapi.h>
#include <Dbt.h>

// TODO: Wrap in define for Win 8.1 and above
#include <shellscalingapi.h>

// NOTE: Undefine these as we have functions with these names 
#undef CreateWindow
#undef CreateFile
#undef CreateThread
#undef CreateEvent

#undef InterlockedAdd
#undef InterlockedSub
#undef InterlockedAnd
#undef InterlockedOr
#undef InterlockedXor
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedCompareExchange
#undef InterlockedExchange

#undef MemoryBarrier
#undef Yield

#undef MessageBox

#undef OutputDebugString
#undef OutputDebugFormat

#undef GetClassName
#undef GetModuleHandle
#undef GetCurrentWorkingDirectory

#undef IsMinimized
#undef IsMaximized

// Windows version define
#ifdef NTDDI_VISTA
    #define PLATFORM_WINDOWS_VISTA (1)
#endif

#ifdef NTDDI_WIN10
    #define PLATFORM_WINDOWS_10 (1)
#endif

#ifdef NTDDI_WIN10_RS1
    #define PLATFORM_WINDOWS_10_ANNIVERSARY (1)
#endif
