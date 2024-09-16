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

// Windows version define
#ifdef NTDDI_VISTA
#define PLATFORM_WINDOWS_VISTA (1)
#endif

#ifdef NTDDI_WIN7
#define PLATFORM_WINDOWS_7 (1)
#endif

#ifdef NTDDI_WIN8
#define PLATFORM_WINDOWS_8 (1)
#endif

#ifdef NTDDI_WINBLUE
#define PLATFORM_WINDOWS_8_1 (1)
#endif

#ifdef NTDDI_WIN10
#define PLATFORM_WINDOWS_10 (1)
#endif

#ifdef NTDDI_WIN10_RS1
#define PLATFORM_WINDOWS_10_ANNIVERSARY (1)
#endif

#ifdef NTDDI_WIN10_FE
#define PLATFORM_WINDOWS_11 (1)
#endif

// TODO: Wrap in define for Win 8.1 and above
#if PLATFORM_WINDOWS_8_1
    #include <shellscalingapi.h>
#endif

// NOTE: Undefine these as we have functions with these names 
#undef CreateWindow
#undef CreateFile
#undef CreateThread
#undef CreateEvent
#undef CreateSemaphore

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
