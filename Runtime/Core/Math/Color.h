#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for floating-point color-data 

struct SColorF
{
    FORCEINLINE SColorF()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    {
    }

    FORCEINLINE SColorF(float InR, float InG, float InB, float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    {
    }

    FORCEINLINE void Set(float InR, float InG, float InB, float InA)
    {
        r = InR;
        g = InG;
        b = InB;
        a = InA;
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