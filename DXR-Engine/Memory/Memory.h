#pragma once
#include "Core.h"

class Memory
{
public:
    static void* Malloc(uint64 Size);
    static void  Free(void* Ptr);

    template<typename T>
    static T* Malloc(uint32 Count)
    {
        return reinterpret_cast<T*>(Malloc(sizeof(T) * Count));
    }

    static void* Memset(void* Destination, uint8 Value, uint64 Size);
    static void* Memzero(void* Destination, uint64 Size);
    
    template<typename T>
    static T* Memzero(T* Destination)
    {
        return reinterpret_cast<T*>(Memzero(Destination, sizeof(T)));
    }

    static void* Memcpy(void* Destination, const void* Source, uint64 Size);

    template<typename T>
    static T* Memcpy(T* Destination, const T* Source)
    {
        return reinterpret_cast<T*>(Memcpy(Destination, Source, sizeof(T)));
    }

    static void* Memmove(void* Destination, const void* Source, uint64 Size);
    
    static char* Strcpy(char* Destination, const char* Source);
};