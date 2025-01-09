#ifndef MATRICES_HLSLI
#define MATRICES_HLSLI

// Left handed
float4x4 OrtographicMatrix(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    float Width  = 1.0 / (Right - Left);
    float Height = 1.0 / (Top - Bottom);
    float Range  = 1.0 / (Far - Near);
    
    return float4x4(
        float4( Width + Width,           0.0,                      0.0,          0.0),
        float4( 0.0,                     Height + Height,          0.0,          0.0),
        float4( 0.0,                     0.0,                      Range,        0.0),
        float4(-(Left + Right) * Width, -(Top + Bottom) * Height, -Range * Near, 1.0));
}

float4x4 TranslationMatrix(float x, float y, float z)
{
    return float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(x,   y,   z,   1.0));
}

float4x4 ScaleMatrix(float x, float y, float z, float w = 1.0)
{
    return float4x4(
        float4(x,   0.0, 0.0, 0.0),
        float4(0.0, y,   0.0, 0.0),
        float4(0.0, 0.0, z,   0.0),
        float4(0.0, 0.0, 0.0, w));
}

float4x4 PitchYawRollMatrix(float Pitch, float Yaw, float Roll)
{
    float SinP = sin(Pitch);
    float SinY = sin(Yaw);
    float SinR = sin(Roll);
    float CosP = cos(Pitch);
    float CosY = cos(Yaw);
    float CosR = cos(Roll);
    
    // TODO: Optimize
    return float4x4(
        float4(CosR*CosY - SinR*SinP*SinY, -SinR*CosP, CosR*SinY - SinR*SinP*CosY, 0.0),
        float4(SinR*CosY + CosR*SinP*SinY,  CosR*CosP, SinR*SinY + CosR*SinP*CosY, 0.0),
        float4(-CosP*SinY,                  SinP,      CosP*SinY,                  0.0),
        float4(0.0,                         0.0,       0.0,                        1.0));
}

// Left Handed
float4x4 LookToMatrix(float3 Eye, float3 Direction, float3 Up)
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
        float4(0.0, 0.0, 0.0, 1.0)));
}

// Left Handed
float4x4 LookAtMatrix(float3 Eye, float3 At, float3 Up)
{
    float3 Direction = At - Eye;
    return LookToMatrix(Eye, Direction, Up);
}

// Inverse of scale and translation (Hint: Projection for example)
float4x4 InverseScaleTranslation(in float4x4 Matrix)
{
    float4x4 Inverse = float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0));

    Inverse[0][0] =  1.0 / Matrix[0][0];
    Inverse[1][1] =  1.0 / Matrix[1][1];
    Inverse[2][2] =  1.0 / Matrix[2][2];
    Inverse[3][0] = -Matrix[3][0] * Inverse[0][0];
    Inverse[3][1] = -Matrix[3][1] * Inverse[1][1];
    Inverse[3][2] = -Matrix[3][2] * Inverse[2][2];
    return Inverse;
}

float4x4 InverseRotationTranslation(in float3x3 Rotation, in float3 Translation)
{
    float4x4 Inverse = float4x4(
        float4(Rotation._11_21_31, 0.0),
        float4(Rotation._12_22_32, 0.0),
        float4(Rotation._13_23_33, 0.0),
        float4(0.0, 0.0, 0.0, 1.0));
    
    Inverse[3][0] = -dot(Translation, Rotation[0]);
    Inverse[3][1] = -dot(Translation, Rotation[1]);
    Inverse[3][2] = -dot(Translation, Rotation[2]);
    return Inverse;
}

#endif