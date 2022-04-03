#pragma once
#include "Float.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CColor

class CColor
{
public:
    
    /**
     * @brief: Default constructor (Initialize components to zero)
     */
    FORCEINLINE CColor()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    { }

    /**
     * @brief: Initialize color with all channels
     *
     * @param InR: Red channel
     * @param InG: Green channel
     * @param InB: Blue channel
     * @param InA: Alpha channel
     */
    FORCEINLINE CColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    { }

    FORCEINLINE uint8* GetData()
    {
        return reinterpret_cast<uint8*>(this);
    }

    FORCEINLINE const uint8* GetData() const
    {
        return reinterpret_cast<const uint8*>(this);
    }

    bool operator==(const CColor& RHS) const
    {
        return (r == RHS.r) && (g == RHS.g) && (b == RHS.b) && (a == RHS.a);
    }

    bool operator!=(const CColor& RHS) const
    {
        return !(*this == RHS);
    }

    struct
    {
        /** Red channel */
        uint8 r;

        /** Green channel */
        uint8 g;

        /** Blue channel */
        uint8 b;

        /** Alpha channel */
        uint8 a;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CFloatColor16

class CFloatColor16
{
public:

    /**
     * @brief: Default constructor (Initialize components to zero)
     */
    FORCEINLINE CFloatColor16()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    { }

    /**
     * @brief: Initialize color with all channels
     *
     * @param InR: Red channel
     * @param InG: Green channel
     * @param InB: Blue channel
     * @param InA: Alpha channel
     */
    FORCEINLINE CFloatColor16(float InR, float InG, float InB, float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    { }

    FORCEINLINE uint16* GetData()
    { 
        return reinterpret_cast<uint16*>(this);
    }

    FORCEINLINE const uint16* GetData() const
    { 
        return reinterpret_cast<const uint16*>(this); 
    }

    bool operator==(const CFloatColor16& RHS) const
    {
        return (r == RHS.r) && (g == RHS.g) && (b == RHS.b) && (a == RHS.a);
    }

    bool operator!=(const CFloatColor16& RHS) const
    {
        return !(*this == RHS);
    }

    /** Red channel */
    CFloat16 r;

    /** Green channel */
    CFloat16 g;

    /** Blue channel */
    CFloat16 b;

    /** Alpha channel */
    CFloat16 a;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CFloatColor

class CFloatColor
{
public:

    /**
     * @brief: Default constructor (Initialize components to zero)
     */
    FORCEINLINE CFloatColor()
        : r(0.0f)
        , g(0.0f)
        , b(0.0f)
        , a(0.0f)
    { }

    /**
     * @brief: Initialize color with all channels
     * 
     * @param InR: Red channel
     * @param InG: Green channel
     * @param InB: Blue channel
     * @param InA: Alpha channel
     */
    FORCEINLINE CFloatColor(float InR, float InG, float InB, float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    { }

    FORCEINLINE float* GetData()
    {
        return reinterpret_cast<float*>(this);
    }

    FORCEINLINE const float* GetData() const
    {
        return reinterpret_cast<const float*>(this);
    }

    bool operator==(const CFloatColor& RHS) const
    {
        return (r == RHS.r) && (g == RHS.g) && (b == RHS.b) && (a == RHS.a);
    }

    bool operator!=(const CFloatColor& RHS) const
    {
        return !(*this == RHS);
    }

    /** Red channel */
    float r;

    /** Green channel */
    float g;
            
    /** Blue channel */
    float b;
            
    /** Alpha channel */
    float a;
};