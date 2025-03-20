#ifndef FAST_MATH_HLSLI
#define FAST_MATH_HLSLI
#include "Constants.hlsli"

// Fast approximate square root function using bit-level manipulation.
// This technique approximates sqrt(X) by operating directly on the bit representation.
float FastSqrt(float X)
{
    // Reinterpret the bits of the float as an integer.
    int I = asint(X);
    
    // Shift the bits right by 1 (roughly halving the exponent)
    // and add a magic constant to correct the bias for the square root approximation.
    I = 0x1fbd1df5 + (I >> 1);
    
    // Convert the manipulated integer bits back to a float.
    // The result is a fast approximation of the square root of X.
    return asfloat(I);
}

// Fast approximate vector length (magnitude) calculation for a 3D vector.
float FastLength(float3 V)
{
    float LengthSquared = dot(V, V);
    return FastSqrt(LengthSquared);
}

// Fast approximate sine function using a polynomial approximation.
// This method trades some precision for a significant performance gain.
float FastSin(float X)
{
    // Constants for the polynomial approximation.
    // B and C are coefficients that shape the linear and quadratic components,
    // while P is a correction factor that reduces the overall error.
    const float B =  4.0 / PI;     // Linear coefficient: scales the input.
    const float C = -4.0 / PI_2;   // Quadratic coefficient: adds curvature based on |X|.
    const float P =  0.225;        // Correction factor: refines the approximation.
    
    // Initial polynomial approximation combining a linear term and a quadratic term.
    // B * X produces a linear ramp, and C * X * abs(X) introduces a parabolic shape.
    float Y = B * X + C * X * abs(X);
    
    // Apply a correction term that further reduces the error of the approximation.
    Y = P * (Y * abs(Y) - Y) + Y;
    return Y;
}

// Fast approximate cosine function.
// Uses arithmetic and modulo operations to mimic the cosine waveform.
float FastCos(float X)
{
   // Breakdown of the expression:
   // 1. abs(X) / PI_2: Normalizes the angle using PI/2.
   // 2. % 4.0: Wraps the value into a 4-unit periodic range to mimic cosine's periodicity.
   // 3. abs(... - 2.0) - 1.0: Shapes the wrapped linear function into an approximate cosine curve.
   return abs(abs(X) / PI_2 % 4.0 - 2.0) - 1.0;
}


#endif