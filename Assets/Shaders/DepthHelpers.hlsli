#ifndef DEPTHHELPERS_HLSLI
#define DEPTHHELPERS_HLSLI

float Depth_ProjToView(float Depth, float4x4 ProjectionInv)
{
    return 1.0f / (Depth * ProjectionInv._34 + ProjectionInv._44);
}

float3 Float3_ProjToView(float3 P, float4x4 ProjectionInv)
{
    float4 ViewP = mul(float4(P, 1.0f), ProjectionInv);
    return (ViewP / ViewP.w).xyz;
}

float3 PositionFromDepth(float Depth, float2 TexCoord, float4x4 ProjectionInv)
{
    float z = Depth;
    float x = TexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - TexCoord.y) * 2.0f - 1.0f;

    float4 ProjectedPos  = float4(x, y, z, 1.0f);
    
    float4 FinalPosition = mul(ProjectedPos, ProjectionInv);  
    return FinalPosition.xyz / FinalPosition.w;
}

float DepthClipToEye(float Near, float Far, float z)
{
    return Near + (Far - Near) * z;
}

float DepthToLinear(float NearPlane, float FarPlane, float Depth)
{
    float LinearDepth = (2.0 * NearPlane * FarPlane) / (FarPlane + NearPlane - Depth * (FarPlane - NearPlane));
    return LinearDepth;
}

#endif