#pragma once

struct FGenericPlatformTypes
{
    typedef unsigned char uint8;
    typedef unsigned short uint16;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;

    typedef signed char int8;
    typedef signed short int16;
    typedef signed int int32;
    typedef signed long long int64;

    typedef char ANSICHAR;
    typedef wchar_t WIDECHAR;
    typedef ANSICHAR CHAR;
    typedef ANSICHAR CHAR_T;

    typedef int64 PTR_INT;
    typedef uint64 UPTR_INT;

    typedef uint64 SIZE_T;
    typedef int64 SSIZE_T;

    typedef void VOID_TYPE;
    typedef decltype(nullptr) NULLPTR_TYPE;
};