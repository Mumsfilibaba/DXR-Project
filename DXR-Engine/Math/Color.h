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

    ColorF(float InR, float InG, float InB, float InA)
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

    void Set(float InR, float InG, float InB, float InA)
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
        float Elements[4];
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
    };
};