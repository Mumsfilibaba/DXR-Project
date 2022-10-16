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

#ifdef CreateWindow
    #undef CreateWindow
#endif

#ifdef CreateFile
    #undef CreateFile
#endif

#ifdef CreateThread
    #undef CreateThread
#endif

#ifdef CreateEvent
    #undef CreateEvent
#endif

#ifdef InterlockedAdd
    #undef InterlockedAdd
#endif

#ifdef InterlockedSub
    #undef InterlockedSub
#endif

#ifdef InterlockedAnd
    #undef InterlockedAnd
#endif

#ifdef InterlockedOr
    #undef InterlockedOr
#endif

#ifdef InterlockedXor
    #undef InterlockedXor
#endif

#ifdef InterlockedIncrement
    #undef InterlockedIncrement
#endif

#ifdef InterlockedDecrement
    #undef InterlockedDecrement
#endif

#ifdef InterlockedCompareExchange
    #undef InterlockedCompareExchange
#endif

#ifdef InterlockedExchange
    #undef InterlockedExchange
#endif

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef OutputDebugString
     #undef OutputDebugString
#endif

#ifdef OutputDebugFormat
     #undef OutputDebugFormat
#endif

#ifdef MemoryBarrier
    #undef MemoryBarrier
#endif

#ifdef GetClassName
    #undef GetClassName
#endif

#ifdef GetModuleHandle
    #undef GetModuleHandle
#endif


#ifdef NTDDI_VISTA
    #define PLATFORM_WINDOWS_VISTA (1)
#endif

#ifdef NTDDI_WIN10_RS1
    #define PLATFORM_WINDOWS_10_ANNIVERSARY (1)
#endif
