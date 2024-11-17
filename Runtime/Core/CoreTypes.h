#pragma once

typedef unsigned char uint8;
static_assert(sizeof(uint8) == 1, "uint8 has to be equal to 1 byte in size");

typedef unsigned short uint16;
static_assert(sizeof(uint16) == 2, "uint8 has to be equal to 2 byte in size");

typedef unsigned int uint32;
static_assert(sizeof(uint32) == 4, "uint8 has to be equal to 4 byte in size");

typedef unsigned long long uint64;
static_assert(sizeof(uint64) == 8, "uint64 has to be equal to 8 byte in size");

typedef signed char int8;
static_assert(sizeof(int8) == 1, "int8 has to be equal to 1 byte in size");

typedef signed short int16;
static_assert(sizeof(int16) == 2, "int16 has to be equal to 2 byte in size");

typedef signed int int32;
static_assert(sizeof(int32) == 4, "int32 has to be equal to 4 byte in size");

typedef signed long long int64;
static_assert(sizeof(int64) == 8, "int64 has to be equal to 8 byte in size");

typedef int64 ptrint;
static_assert(sizeof(ptrint) == sizeof(void*), "The size of ptrint has to be equal to the size of int64");

typedef uint64 uptrint;
static_assert(sizeof(uptrint) == sizeof(void*), "The size of uptrint has to be equal to the size of uint64");

typedef wchar_t WIDECHAR;
typedef char    CHAR;
typedef CHAR    TCHAR;

typedef uint64 TSIZE;

typedef void              void_type;
typedef decltype(nullptr) nullptr_type;