#ifndef MATRICES_HLSLI
#define MATRICES_HLSLI

struct Matrix
{
    // Left handed
    static float4x4 OrthographicProjection(float Left, float Right, float Bottom, float Top, float Near, float Far)
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

    static float4x4 Translation(float X, float Y, float Z)
    {
        return float4x4(
            float4(1.0, 0.0, 0.0, 0.0),
            float4(0.0, 1.0, 0.0, 0.0),
            float4(0.0, 0.0, 1.0, 0.0),
            float4(X,   Y,   Z,   1.0));
    }

    static float4x4 Scale(float X, float Y, float Z, float W = 1.0)
    {
        return float4x4(
            float4(X,   0.0, 0.0, 0.0),
            float4(0.0, Y,   0.0, 0.0),
            float4(0.0, 0.0, Z,   0.0),
            float4(0.0, 0.0, 0.0, W));
    }

    static float4x4 PitchYawRoll(float Pitch, float Yaw, float Roll)
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
    static float4x4 LookTo(float3 Eye, float3 Direction, float3 Up)
    {
        float3 AxisZ = normalize(Direction);
        float3 AxisX = normalize(cross(Up, AxisZ));
        float3 AxisY = normalize(cross(AxisZ, AxisX));
        
        float3 NegEye = -Eye;
        
        float M30 = dot(NegEye, AxisX);
        float M31 = dot(NegEye, AxisY);
        float M32 = dot(NegEye, AxisZ);
        
        return transpose(float4x4(
            float4(AxisX, M30),
            float4(AxisY, M31),
            float4(AxisZ, M32),
            float4(0.0, 0.0, 0.0, 1.0)));
    }

    // Left Handed
    static float4x4 LookAt(float3 Eye, float3 At, float3 Up)
    {
        float3 Direction = At - Eye;
        return LookTo(Eye, Direction, Up);
    }

    // Inverse of scale and translation
    static float4x4 InvScaleTranslation(in float4x4 ScaleTranslation)
    {
        float4x4 Inverse = float4x4(
            float4(1.0, 0.0, 0.0, 0.0),
            float4(0.0, 1.0, 0.0, 0.0),
            float4(0.0, 0.0, 1.0, 0.0),
            float4(0.0, 0.0, 0.0, 1.0));

        Inverse[0][0] =  1.0 / ScaleTranslation[0][0];
        Inverse[1][1] =  1.0 / ScaleTranslation[1][1];
        Inverse[2][2] =  1.0 / ScaleTranslation[2][2];
        Inverse[3][0] = -ScaleTranslation[3][0] * Inverse[0][0];
        Inverse[3][1] = -ScaleTranslation[3][1] * Inverse[1][1];
        Inverse[3][2] = -ScaleTranslation[3][2] * Inverse[2][2];
        return Inverse;
    }

    static float4x4 InvRotationTranslation(in float3x3 Rotation, in float3 Translation)
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
};

#endif