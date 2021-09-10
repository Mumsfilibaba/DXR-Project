#pragma once
#include "MathCommon.h"

// TODO: Fill this out properly with functions etc
class CMatrix3x4
{
public:
    
	/* Default constructor (Initialize components to zero) */
	FORCEINLINE CMatrix3x4() noexcept;
	
    /**
     * Constructor initializing all values with corresponding value
     *
     * @param In00: Value to set on row 0 and column 0
     * @param In01: Value to set on row 0 and column 1
     * @param In02: Value to set on row 0 and column 2
     * @param In03: Value to set on row 0 and column 3
     * @param In10: Value to set on row 1 and column 0
     * @param In11: Value to set on row 1 and column 1
     * @param In12: Value to set on row 1 and column 2
     * @param In13: Value to set on row 1 and column 3
     * @param In20: Value to set on row 2 and column 0
     * @param In21: Value to set on row 2 and column 1
     * @param In22: Value to set on row 2 and column 2
     * @param In23: Value to set on row 2 and column 3
     */
    FORCEINLINE explicit CMatrix3x4(
        float In00, float In01, float In02, float In03,
        float In10, float In11, float In12, float In13,
        float In20, float In21, float In22, float In23 ) noexcept;

public:
    union
    {
        /* Each element of the matrix */
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
        };

        /* 2-D array of the matrix */
        float Elements[3][4];
    };
};

FORCEINLINE CMatrix3x4::CMatrix3x4() noexcept
	: m00( 0.0f ), m01( 0.0f ), m02( 0.0f ), m03( 0.0f )
	, m10( 0.0f ), m11( 0.0f ), m12( 0.0f ), m13( 0.0f )
	, m20( 0.0f ), m21( 0.0f ), m22( 0.0f ), m23( 0.0f )
{
}

FORCEINLINE CMatrix3x4::CMatrix3x4(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23 ) noexcept
    : m00( m00 ), m01( m01 ), m02( m02 ), m03( m03 )
    , m10( m10 ), m11( m11 ), m12( m12 ), m13( m13 )
    , m20( m20 ), m21( m21 ), m22( m22 ), m23( m23 )
{
}
