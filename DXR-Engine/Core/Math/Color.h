#pragma once
#include "Core.h"

struct ColorF
{
    ColorF()
        : r( 0.0f )
        , g( 0.0f )
        , b( 0.0f )
        , a( 0.0f )
    {
    }

    ColorF( float InR, float InG, float InB, float InA )
        : r( InR )
        , g( InG )
        , b( InB )
        , a( InA )
    {
    }

    void Set( float InR, float InG, float InB, float InA )
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