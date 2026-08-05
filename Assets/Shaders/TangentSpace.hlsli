#ifndef TANGENT_SPACE_HLSLI
#define TANGENT_SPACE_HLSLI 1

float3 CreateBitangent(float3 Normal, float3 Tangent, float Sign)
{
    return cross(Normal, Tangent) * Sign;
}

float3 DecodeTangentNormal(float3 TangentNormal, float3 Normal, float3 Tangent, float Sign)
{
    const float3 N = normalize(Normal);
    const float3 T = normalize(Tangent);
    const float3 B = CreateBitangent(N, T, Sign);
    return normalize(TangentNormal.x * T + TangentNormal.y * B + TangentNormal.z * N);
}

float3x3 CreateWorldToTangent(float3 Normal, float3 Tangent, float Sign)
{
    const float3 N = normalize(Normal);
    const float3 T = normalize(Tangent - dot(Tangent, N) * N);
    const float3 B = CreateBitangent(N, T, Sign);
    return float3x3(T, B, N);
}

#endif // TANGENT_SPACE_HLSLI
