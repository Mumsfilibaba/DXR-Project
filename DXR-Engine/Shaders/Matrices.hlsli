#ifndef MATRICES_HLSLI
#define MATRICES_HLSLI

// Left handed
float4x4 CreateOrtographicProjection(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    float Width  = 1.0f / (Right - Left);
    float Height = 1.0f / (Top - Bottom);
    float Range = 1.0f / (Far - Near);
    
    return float4x4(
        float4(Width + Width, 0.0f, 0.0f, 0.0f),
        float4(0.0f, Height + Height, 0.0f, 0.0f),
        float4(0.0f, 0.0f, Range, 0.0f),
        float4(-(Left + Right) * Width, -(Top + Bottom) * Height, -Range * Near, 1.0f));
}

// TODO: Projection Matrix

float4x4 CreateTranslationMatrix(float x, float y, float z)
{
    return float4x4(
        float4(1.0f, 0.0f, 0.0f, 0.0f),
        float4(0.0f, 1.0f, 0.0f, 0.0f),
        float4(0.0f, 0.0f, 1.0f, 0.0f),
        float4(x, y, z, 1.0f));
}

float4x4 CreateScaleMatrix(float x, float y, float z, float w = 1.0f)
{
    return float4x4(
        float4(x,    0.0f, 0.0f, 0.0f),
        float4(0.0f, y,    0.0f, 0.0f),
        float4(0.0f, 0.0f, z,    0.0f),
        float4(0.0f, 0.0f, 0.0f, w));
}

float4x4 CreatePitchYawRollMatrix(float Pitch, float Yaw, float Roll)
{
    float SinP = sin(Pitch);
    float SinY = sin(Yaw);
    float SinR = sin(Roll);
    float CosP = cos(Pitch);
    float CosY = cos(Yaw);
    float CosR = cos(Roll);
    
    // TODO: Optimize
    return float4x4(
        float4(CosR*CosY - SinR*SinP*SinY, -SinR*CosP, CosR*SinY - SinR*SinP*CosY, 0.0f),
        float4(SinR*CosY + CosR*SinP*SinY,  CosR*CosP, SinR*SinY + CosR*SinP*CosY, 0.0f),
        float4(-CosP*SinY,                  SinP,      CosP*SinY,                  0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f));
}

// Left Handed
float4x4 CreateLookToMatrix(float3 Eye, float3 Direction, float3 Up)
{
    float3 e2 = normalize(Direction);
    float3 e0 = normalize(cross(Up, e2));
    float3 e1 = normalize(cross(e2, e0));
    
    float3 NegEye = -Eye;
    
    float m30 = dot(NegEye, e0);
    float m31 = dot(NegEye, e1);
    float m32 = dot(NegEye, e2);
    
    return transpose(float4x4(
        float4(e0, m30),
        float4(e1, m31),
        float4(e2, m32),
        float4(0.0f, 0.0f, 0.0f, 1.0f)));
}

// Left Handed
float4x4 CreateLookAtMatrix(float3 Eye, float3 At, float3 Up)
{
    float3 Direction = At - Eye;
    return CreateLookToMatrix(Eye, Direction, Up);
}

#endif