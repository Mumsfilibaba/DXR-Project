#ifndef RAY_TRACING_HELPERS
#define RAY_TRACING_HELPERS
#include "Structs.hlsli"

struct RayPayload
{
    float3 Color;
    float  T;
};

struct RandomData
{
    int FrameIndex;
    int Seed;
    int Padding1;
    int Padding2;
};

struct TriangleHit
{
    uint InstanceID;
    uint HitGroupIndex;
    uint PrimitiveIndex;
    float2 Barycentrics;
};

struct VertexData
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
    float2 TexCoord;
    float3 BarycentricCoords;
};

float3 WorldHitPosition()
{
    return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

uint CalculateBaseIndex(in TriangleHit HitData)
{
    const uint IndexSizeInBytes    = 4;
    const uint IndicesPerTriangle  = 3;
    const uint TriangleIndexStride = IndicesPerTriangle * IndexSizeInBytes;
    return HitData.PrimitiveIndex * TriangleIndexStride;
}

void LoadVertexData(in TriangleHit HitData, in Vertex Vertices[3], out VertexData Vertex)
{
    float3 TrianglePosition[3] =
    {
       Vertices[0].Position,
       Vertices[1].Position,
       Vertices[2].Position
    };
    
    float3 TriangleNormals[3] =
    {
        Vertices[0].Normal,
        Vertices[1].Normal,
        Vertices[2].Normal
    };
    
    float3 TriangleTangents[3] =
    {
        Vertices[0].Tangent,
        Vertices[1].Tangent,
        Vertices[2].Tangent
    };
    
    float2 TriangleTexCoords[3] =
    {
        Vertices[0].TexCoord,
        Vertices[1].TexCoord,
        Vertices[2].TexCoord
    };
    
    float3 BarycentricCoords = float3(1.0f - HitData.Barycentrics.x - HitData.Barycentrics.y, HitData.Barycentrics.x, HitData.Barycentrics.y);
    
    float3 Position = (TrianglePosition[0]  * BarycentricCoords.x) + (TrianglePosition[1]  * BarycentricCoords.y) + (TrianglePosition[2]  * BarycentricCoords.z);
    float3 Normal   = (TriangleNormals[0]   * BarycentricCoords.x) + (TriangleNormals[1]   * BarycentricCoords.y) + (TriangleNormals[2]   * BarycentricCoords.z);
    float3 Tangent  = (TriangleTangents[0]  * BarycentricCoords.x) + (TriangleTangents[1]  * BarycentricCoords.y) + (TriangleTangents[2]  * BarycentricCoords.z);
    float2 TexCoord = (TriangleTexCoords[0] * BarycentricCoords.x) + (TriangleTexCoords[1] * BarycentricCoords.y) + (TriangleTexCoords[2] * BarycentricCoords.z);
    TexCoord.y = -TexCoord.y;
    
    Vertex.Position  = Position;
    Vertex.Normal    = normalize(Normal);
    Vertex.Tangent   = normalize(Tangent);
    Vertex.Bitangent = normalize(cross(Vertex.Tangent, Vertex.Normal));
    Vertex.TexCoord  = TexCoord;
    Vertex.BarycentricCoords = BarycentricCoords;
}

#endif