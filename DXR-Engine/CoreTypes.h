#pragma once

/* Unsigned Integer types*/

typedef unsigned char uint8;
static_assert(sizeof(uint8) == 1, "uint8 has to be 1 byte in size");

typedef unsigned short uint16;
static_assert(sizeof(uint16) == 2, "uint8 has to be 2 byte in size");

typedef unsigned int uint32;
static_assert(sizeof(uint32) == 4, "uint8 has to be 4 byte in size");

typedef unsigned long long uint64;
static_assert(sizeof(uint64) == 8, "uint64 has to be 8 byte in size");

/* Signed Integer types*/

typedef char int8;
static_assert(sizeof(int8) == 1, "int8 has to be 1 byte in size");

typedef short int16;
static_assert(sizeof(int16) == 2, "int16 has to be 2 byte in size");

typedef int int32;
static_assert(sizeof(int32) == 4, "int32 has to be 4 byte in size");

typedef long long int64;
static_assert(sizeof(int64) == 8, "int64 has to be 8 byte in size");
