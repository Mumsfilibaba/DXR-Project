#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for floating-point color-data 

struct SColorF
{
    /**
     * Default constructor (Initialize components to zero)
     */
    FORCEINLINE SColorF()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    { }

    /**
     * Initialize color with all channels
     * 
     * @param InR: Red channel
     * @param InG: Green channel
     * @param InB: Blue channel
     * @param InA: Alpha channel
     */
    FORCEINLINE SColorF(float InR, float InG, float InB, float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    { }

    /**
     * Set all components
     *
     * @param InR: Red channel
     * @param InG: Green channel
     * @param InB: Blue channel
     * @param InA: Alpha channel
     */
    FORCEINLINE void SetElements(float InR, float InG, float InB, float InA)
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
            /* Red channel */
            float r;
            /* Green channel */
            float g;
            /* Blue channel */
            float b;
            /* Alpha channel */
            float a;
        };
    };
};