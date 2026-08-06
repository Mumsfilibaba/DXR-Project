#ifndef VERTEX_PACKING_HLSLI
#define VERTEX_PACKING_HLSLI

// Decodes four signed-normalized 16-bit channels packed into two uints,
// matching FRGBA16Snorm in Runtime/Core/Math/FormatStructs.h.
float4 UnpackSnorm16x4(uint2 Packed)
{
    const int X = int(Packed.x << 16) >> 16;
    const int Y = int(Packed.x) >> 16;
    const int Z = int(Packed.y << 16) >> 16;
    const int W = int(Packed.y) >> 16;

    return max(float4(X, Y, Z, W) / 32767.0f, -1.0f);
}

#endif
