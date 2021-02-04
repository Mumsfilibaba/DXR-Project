#pragma once
#include "Core.h"

#include "Memory/Memory.h"

struct ColorF
{
    ColorF()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    {
    }

    ColorF(Float InR, Float InG, Float InB, Float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    {
    }

    ColorF(const ColorF& Other)
    {
        Memory::Memcpy(Elements, Other.Elements, sizeof(Elements));
    }

    void Set(Float InR, Float InG, Float InB, Float InA)
    {
        r = InR;
        g = InG;
        b = InB;
        a = InA;
    }

    ColorF& operator=(const ColorF& Other)
    {
        Memory::Memcpy(Elements, Other.Elements, sizeof(Elements));
        return *this;
    }

    union
    {
        Float Elements[4];
        struct
        {
            Float r;
            Float g;
            Float b;
            Float a;
        };
    };
};