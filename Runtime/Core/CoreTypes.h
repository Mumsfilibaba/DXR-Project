#pragma once
#include "Core/Platform/PlatformTypes.h"

typedef FPlatformTypes::uint8 uint8;
static_assert(sizeof(uint8) == 1, "uint8 has to be equal to 1 byte in size");

typedef FPlatformTypes::uint16 uint16;
static_assert(sizeof(uint16) == 2, "uint8 has to be equal to 2 byte in size");

typedef FPlatformTypes::uint32 uint32;
static_assert(sizeof(uint32) == 4, "uint8 has to be equal to 4 byte in size");

typedef FPlatformTypes::uint64 uint64;
static_assert(sizeof(uint64) == 8, "uint64 has to be equal to 8 byte in size");

typedef FPlatformTypes::int8 int8;
static_assert(sizeof(int8) == 1, "int8 has to be equal to 1 byte in size");

typedef FPlatformTypes::int16 int16;
static_assert(sizeof(int16) == 2, "int16 has to be equal to 2 byte in size");

typedef FPlatformTypes::int32 int32;
static_assert(sizeof(int32) == 4, "int32 has to be equal to 4 byte in size");

typedef FPlatformTypes::int64 int64;
static_assert(sizeof(int64) == 8, "int64 has to be equal to 8 byte in size");

typedef FPlatformTypes::PTR_INT PTR_INT;
static_assert(sizeof(PTR_INT) == sizeof(void*), "The size of PTR_INT has to be equal to the size of int64");

typedef FPlatformTypes::UPTR_INT UPTR_INT;
static_assert(sizeof(UPTR_INT) == sizeof(void*), "The size of UPTR_INT has to be equal to the size of uint64");

typedef FPlatformTypes::WIDECHAR WIDECHAR;
typedef FPlatformTypes::CHAR CHAR;
typedef FPlatformTypes::CHAR_T CHAR_T;

typedef FPlatformTypes::SIZE_T SIZE_T;
typedef FPlatformTypes::SSIZE_T SSIZE_T;

typedef FPlatformTypes::VOID_TYPE VOID_TYPE;
typedef FPlatformTypes::NULLPTR_TYPE NULLPTR_TYPE;
