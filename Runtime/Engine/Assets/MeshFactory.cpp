#include "Engine/Assets/MeshFactory.h"

FMeshData MeshFactory::CreateCube(float Width, float Height, float Depth) noexcept
{
    const float HalfWidth  = Width  * 0.5f;
    const float HalfHeight = Height * 0.5f;
    const float HalfDepth  = Depth  * 0.5f;

    FMeshData CubeData;
    CubeData.Vertices =
    {
        // FRONT FACE
        { Vector3(-HalfWidth,  HalfHeight, -HalfDepth), Vector3(0.0f,  0.0f, -1.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 0.0f) },
        { Vector3( HalfWidth,  HalfHeight, -HalfDepth), Vector3(0.0f,  0.0f, -1.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-HalfWidth, -HalfHeight, -HalfDepth), Vector3(0.0f,  0.0f, -1.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3( HalfWidth, -HalfHeight, -HalfDepth), Vector3(0.0f,  0.0f, -1.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 1.0f) },

        // BACK FACE
        { Vector3( HalfWidth,  HalfHeight,  HalfDepth), Vector3(0.0f,  0.0f,  1.0f), Vector3(-1.0f,  0.0f, 0.0f), Vector2(0.0f, 0.0f) },
        { Vector3(-HalfWidth,  HalfHeight,  HalfDepth), Vector3(0.0f,  0.0f,  1.0f), Vector3(-1.0f,  0.0f, 0.0f), Vector2(1.0f, 0.0f) },
        { Vector3( HalfWidth, -HalfHeight,  HalfDepth), Vector3(0.0f,  0.0f,  1.0f), Vector3(-1.0f,  0.0f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3(-HalfWidth, -HalfHeight,  HalfDepth), Vector3(0.0f,  0.0f,  1.0f), Vector3(-1.0f,  0.0f, 0.0f), Vector2(1.0f, 1.0f) },

        // RIGHT FACE
        { Vector3(HalfWidth,  HalfHeight, -HalfDepth), Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(0.0f, 0.0f) },
        { Vector3(HalfWidth,  HalfHeight,  HalfDepth), Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(1.0f, 0.0f) },
        { Vector3(HalfWidth, -HalfHeight, -HalfDepth), Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(0.0f, 1.0f) },
        { Vector3(HalfWidth, -HalfHeight,  HalfDepth), Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(1.0f, 1.0f) },

        // LEFT FACE
        { Vector3(-HalfWidth,  HalfHeight, -HalfDepth), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(0.0f, 0.0f) },
        { Vector3(-HalfWidth,  HalfHeight,  HalfDepth), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-HalfWidth, -HalfHeight, -HalfDepth), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(0.0f, 1.0f) },
        { Vector3(-HalfWidth, -HalfHeight,  HalfDepth), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  0.0f, 1.0f), Vector2(1.0f, 1.0f) },

        // TOP FACE
        { Vector3(-HalfWidth,  HalfHeight,  HalfDepth), Vector3(0.0f,  1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 0.0f) },
        { Vector3( HalfWidth,  HalfHeight,  HalfDepth), Vector3(0.0f,  1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-HalfWidth,  HalfHeight, -HalfDepth), Vector3(0.0f,  1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3( HalfWidth,  HalfHeight, -HalfDepth), Vector3(0.0f,  1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 1.0f) },

        // BOTTOM FACE
        { Vector3(-HalfWidth, -HalfHeight, -HalfDepth), Vector3(0.0f, -1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 0.0f) },
        { Vector3( HalfWidth, -HalfHeight, -HalfDepth), Vector3(0.0f, -1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-HalfWidth, -HalfHeight,  HalfDepth), Vector3(0.0f, -1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3( HalfWidth, -HalfHeight,  HalfDepth), Vector3(0.0f, -1.0f,  0.0f), Vector3(1.0f,  0.0f, 0.0f), Vector2(1.0f, 1.0f) },
    };

    CubeData.Indices =
    {
        // Front Face
        0, 1, 2,
        1, 3, 2,

        // Back Face
        4, 5, 6,
        5, 7, 6,

        // Right Face
        8, 9, 10,
        9, 11, 10,

        // Left Face
        14, 13, 12,
        14, 15, 13,

        // Top Face
        16, 17, 18,
        17, 19, 18,

        // Bottom Face
        20, 21, 22,
        21, 23, 22
    };

    CubeData.CalculateTangents();
    return CubeData;
}

FMeshData MeshFactory::CreatePlane(uint32 Width, uint32 Height) noexcept
{
    FMeshData PlaneData;
    if (Width < 1)
    {
        Width = 1;
    }
    if (Height < 1)
    {
        Height = 1;
    }

    PlaneData.Vertices.Resize((Width + 1) * (Height + 1));
    PlaneData.Indices.Resize((Width * Height) * 6);

    // Size of each quad, size of the plane will always be between -0.5 and 0.5
    Vector2 QuadSize   = Vector2(1.0f / float(Width), 1.0f / float(Height));
    Vector2 UvQuadSize = Vector2(1.0f / float(Width), 1.0f / float(Height));

    for (uint32 x = 0; x <= Width; x++)
    {
        for (uint32 y = 0; y <= Height; y++)
        {
            int32 v = ((1 + Height) * x) + y;
            PlaneData.Vertices[v].Position = Vector3(0.5f - (QuadSize.X * x), 0.5f - (QuadSize.Y * y), 0.0f);
            // TODO: Fix vertices so normal is positive
            PlaneData.Vertices[v].Normal   = Vector3(0.0f, 0.0f, -1.0f);
            PlaneData.Vertices[v].Tangent  = Vector3(1.0f, 0.0f, 0.0f);
            PlaneData.Vertices[v].TexCoord = Vector2(0.0f + (UvQuadSize.X * x), 0.0f + (UvQuadSize.Y * y));
        }
    }

    for (uint8 x = 0; x < Width; x++)
    {
        for (uint8 y = 0; y < Height; y++)
        {
            int32 quad = (Height * x) + y;
            PlaneData.Indices[(quad * 6) + 0] = (x * (1 + Height)) + y + 1;
            PlaneData.Indices[(quad * 6) + 1] = (PlaneData.Indices[quad * 6] + 2 + (Height - 1));
            PlaneData.Indices[(quad * 6) + 2] = PlaneData.Indices[(quad * 6) + 0] - 1;
            PlaneData.Indices[(quad * 6) + 3] = PlaneData.Indices[(quad * 6) + 1];
            PlaneData.Indices[(quad * 6) + 4] = PlaneData.Indices[(quad * 6) + 1] - 1;
            PlaneData.Indices[(quad * 6) + 5] = PlaneData.Indices[(quad * 6) + 2];
        }
    }

    PlaneData.Vertices.Shrink();
    PlaneData.Indices.Shrink();

    PlaneData.CalculateTangents();
    return PlaneData;
}

FMeshData MeshFactory::CreateSphere(uint32 Subdivisions, float Radius) noexcept
{
    FMeshData SphereData;
    SphereData.Vertices.Resize(12);

    const float t = (1.0f + Math::Sqrt(5.0f)) / 2.0f;
    SphereData.Vertices[0].Position  = Vector3(-1.0f,  t   ,  0.0f);
    SphereData.Vertices[1].Position  = Vector3( 1.0f,  t   ,  0.0f);
    SphereData.Vertices[2].Position  = Vector3(-1.0f, -t   ,  0.0f);
    SphereData.Vertices[3].Position  = Vector3( 1.0f, -t   ,  0.0f);
    SphereData.Vertices[4].Position  = Vector3( 0.0f, -1.0f,  t);
    SphereData.Vertices[5].Position  = Vector3( 0.0f,  1.0f,  t);
    SphereData.Vertices[6].Position  = Vector3( 0.0f, -1.0f, -t);
    SphereData.Vertices[7].Position  = Vector3( 0.0f,  1.0f, -t);
    SphereData.Vertices[8].Position  = Vector3( t   ,  0.0f, -1.0f);
    SphereData.Vertices[9].Position  = Vector3( t   ,  0.0f,  1.0f);
    SphereData.Vertices[10].Position = Vector3(-t   ,  0.0f, -1.0f);
    SphereData.Vertices[11].Position = Vector3(-t   ,  0.0f,  1.0f);

    SphereData.Indices =
    {
        0, 11, 5,
        0, 5,  1,
        0, 1,  7,
        0, 7,  10,
        0, 10, 11,

        1,  5,  9,
        5,  11, 4,
        11, 10, 2,
        10, 7,  6,
        7,  1,  8,

        3, 9, 4,
        3, 4, 2,
        3, 2, 6,
        3, 6, 8,
        3, 8, 9,

        4, 9, 5,
        2, 4, 11,
        6, 2, 10,
        8, 6, 7,
        9, 8, 1,
    };

    if (Subdivisions > 0)
    {
        SphereData.Subdivide(Subdivisions);
    }

    for (uint32 i = 0; i < static_cast<uint32>(SphereData.Vertices.Size()); i++)
    {
        // Calculate the new position, normal and tangent
        Vector3 Position = SphereData.Vertices[i].Position;
        Position.Normalize();

        SphereData.Vertices[i].Normal   = Position;
        SphereData.Vertices[i].Position = Position * Radius;

        // Calculate UVs
        SphereData.Vertices[i].TexCoord.Y = (Math::Asin(SphereData.Vertices[i].Position.Y) / Math::Constants::PI) + 0.5f;
        SphereData.Vertices[i].TexCoord.X = (Math::Atan2(SphereData.Vertices[i].Position.Z, SphereData.Vertices[i].Position.X) + Math::Constants::PI) / (2.0f * Math::Constants::PI);
    }

    SphereData.Indices.Shrink();
    SphereData.Vertices.Shrink();

    SphereData.CalculateTangents();
    return SphereData;
}

FMeshData MeshFactory::CreateCone(uint32 Sides, float Radius, float Height) noexcept
{
    if (Sides < 3)
    {
        // A cone must have at least 3 sides
        return FMeshData();
    }

    FMeshData MeshData;

    // Number of vertices: (Sides + 1) for the base (including center) + Sides for the side vertices
    const uint32 NumVertices = Sides + 1 + Sides + 1;
    MeshData.Vertices.Resize(NumVertices);

    // Number of indices: (Sides * 3) for the base cap + (Sides * 3) for the sides
    const uint32 NumIndices = Sides * 3 + Sides * 3;
    MeshData.Indices.Resize(NumIndices);

    // Angle between each side segment
    const float Angle = (2.0f * Math::Constants::PI) / static_cast<float>(Sides);
    
    // Create the center vertex for the base cap
    MeshData.Vertices[0].Position = Vector3(0.0f, 0.0f, 0.0f);
    MeshData.Vertices[0].Normal   = Vector3(0.0f, -1.0f, 0.0f);
    MeshData.Vertices[0].TexCoord = Vector2(0.5f, 0.5f); // Center UV coordinates

    // Create vertices for the base cap and sides
    const uint32 Offset = Sides + 1;
    for (uint32 i = 0; i < Sides; ++i)
    {
        // Calculate the position of the current vertex on the base circle
        const float x = Radius * Math::Cos(Angle * i);
        const float z = Radius * Math::Sin(Angle * i);
        const Vector3 BasePosition(x, 0.0f, z);

        // Base vertex
        MeshData.Vertices[i + 1].Position = BasePosition;
        MeshData.Vertices[i + 1].Normal   = Vector3(0.0f, -1.0f, 0.0f); // Pointing downwards
        MeshData.Vertices[i + 1].TexCoord = Vector2((x / Radius + 1.0f) * 0.5f, (z / Radius + 1.0f) * 0.5f);

        // Side vertex
        MeshData.Vertices[Offset + i].Position = BasePosition;
        
        Vector3 Normal(x, Radius / Height, z);
        if (Normal.GetLengthSquared() > 0.0f) // Ensure normalization is safe
        {
            Normal.Normalize();
        }
        
        MeshData.Vertices[Offset + i].Normal   = Normal;
        MeshData.Vertices[Offset + i].TexCoord = Vector2(static_cast<float>(i) / static_cast<float>(Sides), 1.0f);
    }

    // Apex vertex
    MeshData.Vertices[NumVertices - 1].Position = Vector3(0.0f, Height, 0.0f);
    MeshData.Vertices[NumVertices - 1].Normal   = Vector3(0.0f, 1.0f, 0.0f); // Pointing upwards
    MeshData.Vertices[NumVertices - 1].TexCoord = Vector2(0.5f, 0.0f);

    // Create indices for the base cap
    uint32 Index = 0;
    for (uint32 i = 0; i < Sides; ++i)
    {
        MeshData.Indices[Index++] = 0;
        MeshData.Indices[Index++] = i + 1;
        MeshData.Indices[Index++] = ((i + 1) % Sides) + 1;
    }

    // Create indices for the sides
    for (uint32 i = 0; i < Sides; ++i)
    {
        MeshData.Indices[Index++] = Offset + i;
        MeshData.Indices[Index++] = NumVertices - 1;
        MeshData.Indices[Index++] = Offset + ((i + 1) % Sides);
    }

    // Calculate tangents for proper lighting and normal mapping
    MeshData.CalculateTangents();
    return MeshData;
}

FMeshData MeshFactory::CreateTorus(float RingRadius, float TubeRadius, uint32 RingSegments, uint32 TubeSegments) noexcept
{
    if (RingSegments < 3 || TubeSegments < 3)
    {
        // A torus must have at least 3 segments for both the ring and the tube
        return FMeshData();
    }

    // Number of vertices and indices
    const uint32 NumVertices = RingSegments * TubeSegments;
    const uint32 NumIndices  = RingSegments * TubeSegments * 6;

    FMeshData MeshData;
    MeshData.Vertices.Resize(NumVertices);
    MeshData.Indices.Resize(NumIndices);

    // Step angles for each segment
    const float RingStep = 2.0f * Math::Constants::PI / static_cast<float>(RingSegments);
    const float TubeStep = 2.0f * Math::Constants::PI / static_cast<float>(TubeSegments);

    // Create vertices
    uint32 VertexIndex = 0;
    for (uint32 i = 0; i < RingSegments; ++i)
    {
        const float RingAngle = i * RingStep;
        const Vector3 RingCenter = Vector3(RingRadius * Math::Cos(RingAngle), 0.0f, RingRadius * Math::Sin(RingAngle));
        for (uint32 j = 0; j < TubeSegments; ++j)
        {
            const float TubeAngle = j * TubeStep;
            const float CosTube   = Math::Cos(TubeAngle);
            const float SinTube   = Math::Sin(TubeAngle);

            // Position of the vertex
            Vector3 Position = RingCenter + Vector3(TubeRadius * CosTube * Math::Cos(RingAngle), TubeRadius * SinTube, TubeRadius * CosTube * Math::Sin(RingAngle));
            MeshData.Vertices[VertexIndex].Position = Position;

            // Normal vector
            Vector3 Normal = Vector3(CosTube * Math::Cos(RingAngle), SinTube, CosTube * Math::Sin(RingAngle));
            Normal.Normalize();
            
            MeshData.Vertices[VertexIndex].Normal = Normal;

            // Texture coordinates
            const float u = static_cast<float>(i) / static_cast<float>(RingSegments);
            const float v = static_cast<float>(j) / static_cast<float>(TubeSegments);
            MeshData.Vertices[VertexIndex].TexCoord = Vector2(u, v);

            ++VertexIndex;
        }
    }

    // Create indices with inverted winding order
    uint32 Index = 0;
    for (uint32 i = 0; i < RingSegments; ++i)
    {
        for (uint32 j = 0; j < TubeSegments; ++j)
        {
            // Calculate the indices for the four corners of this quad
            uint32 Current  = i * TubeSegments + j;
            uint32 NextRing = ((i + 1) % RingSegments) * TubeSegments + j;
            uint32 NextTube = i * TubeSegments + (j + 1) % TubeSegments;
            uint32 Diagonal = ((i + 1) % RingSegments) * TubeSegments + (j + 1) % TubeSegments;

            // First triangle
            MeshData.Indices[Index++] = Current;
            MeshData.Indices[Index++] = NextTube;
            MeshData.Indices[Index++] = NextRing;

            // Second triangle
            MeshData.Indices[Index++] = NextTube;
            MeshData.Indices[Index++] = Diagonal;
            MeshData.Indices[Index++] = NextRing;
        }
    }

    // Calculate tangents for proper lighting and normal mapping
    MeshData.CalculateTangents();
    return MeshData;
}

FMeshData MeshFactory::CreateTeapot(uint32 Tessellation) noexcept
{
    static constexpr int32 NumPatches  = 32;
    static constexpr int32 NumVertices = 306;

    static constexpr float TeapotControlPoints[NumVertices][3] =
    {
        { 1.4000,  0.0000,  2.4000}, { 1.4000, -0.7840,  2.4000}, { 0.7840, -1.4000,  2.4000}, { 0.0000, -1.4000,  2.4000},
        { 1.3375,  0.0000,  2.5312}, { 1.3375, -0.7490,  2.5312}, { 0.7490, -1.3375,  2.5312}, { 0.0000, -1.3375,  2.5312},
        { 1.4375,  0.0000,  2.5312}, { 1.4375, -0.8050,  2.5312}, { 0.8050, -1.4375,  2.5312}, { 0.0000, -1.4375,  2.5312},
        { 1.5000,  0.0000,  2.4000}, { 1.5000, -0.8400,  2.4000}, { 0.8400, -1.5000,  2.4000}, { 0.0000, -1.5000,  2.4000},
        {-0.7840, -1.4000,  2.4000}, {-1.4000, -0.7840,  2.4000}, {-1.4000,  0.0000,  2.4000}, {-0.7490, -1.3375,  2.5312},
        {-1.3375, -0.7490,  2.5312}, {-1.3375,  0.0000,  2.5312}, {-0.8050, -1.4375,  2.5312}, {-1.4375, -0.8050,  2.5312},
        {-1.4375,  0.0000,  2.5312}, {-0.8400, -1.5000,  2.4000}, {-1.5000, -0.8400,  2.4000}, {-1.5000,  0.0000,  2.4000},
        {-1.4000,  0.7840,  2.4000}, {-0.7840,  1.4000,  2.4000}, { 0.0000,  1.4000,  2.4000}, {-1.3375,  0.7490,  2.5312},
        {-0.7490,  1.3375,  2.5312}, { 0.0000,  1.3375,  2.5312}, {-1.4375,  0.8050,  2.5312}, {-0.8050,  1.4375,  2.5312},
        { 0.0000,  1.4375,  2.5312}, {-1.5000,  0.8400,  2.4000}, {-0.8400,  1.5000,  2.4000}, { 0.0000,  1.5000,  2.4000},
        { 0.7840,  1.4000,  2.4000}, { 1.4000,  0.7840,  2.4000}, { 0.7490,  1.3375,  2.5312}, { 1.3375,  0.7490,  2.5312},
        { 0.8050,  1.4375,  2.5312}, { 1.4375,  0.8050,  2.5312}, { 0.8400,  1.5000,  2.4000}, { 1.5000,  0.8400,  2.4000},
        { 1.7500,  0.0000,  1.8750}, { 1.7500, -0.9800,  1.8750}, { 0.9800, -1.7500,  1.8750}, { 0.0000, -1.7500,  1.8750},
        { 2.0000,  0.0000,  1.3500}, { 2.0000, -1.1200,  1.3500}, { 1.1200, -2.0000,  1.3500}, { 0.0000, -2.0000,  1.3500},
        { 2.0000,  0.0000,  0.9000}, { 2.0000, -1.1200,  0.9000}, { 1.1200, -2.0000,  0.9000}, { 0.0000, -2.0000,  0.9000},
        {-0.9800, -1.7500,  1.8750}, {-1.7500, -0.9800,  1.8750}, {-1.7500,  0.0000,  1.8750}, {-1.1200, -2.0000,  1.3500},
        {-2.0000, -1.1200,  1.3500}, {-2.0000,  0.0000,  1.3500}, {-1.1200, -2.0000,  0.9000}, {-2.0000, -1.1200,  0.9000},
        {-2.0000,  0.0000,  0.9000}, {-1.7500,  0.9800,  1.8750}, {-0.9800,  1.7500,  1.8750}, { 0.0000,  1.7500,  1.8750},
        {-2.0000,  1.1200,  1.3500}, {-1.1200,  2.0000,  1.3500}, { 0.0000,  2.0000,  1.3500}, {-2.0000,  1.1200,  0.9000},
        {-1.1200,  2.0000,  0.9000}, { 0.0000,  2.0000,  0.9000}, { 0.9800,  1.7500,  1.8750}, { 1.7500,  0.9800,  1.8750},
        { 1.1200,  2.0000,  1.3500}, { 2.0000,  1.1200,  1.3500}, { 1.1200,  2.0000,  0.9000}, { 2.0000,  1.1200,  0.9000},
        { 2.0000,  0.0000,  0.4500}, { 2.0000, -1.1200,  0.4500}, { 1.1200, -2.0000,  0.4500}, { 0.0000, -2.0000,  0.4500},
        { 1.5000,  0.0000,  0.2250}, { 1.5000, -0.8400,  0.2250}, { 0.8400, -1.5000,  0.2250}, { 0.0000, -1.5000,  0.2250},
        { 1.5000,  0.0000,  0.1500}, { 1.5000, -0.8400,  0.1500}, { 0.8400, -1.5000,  0.1500}, { 0.0000, -1.5000,  0.1500},
        {-1.1200, -2.0000,  0.4500}, {-2.0000, -1.1200,  0.4500}, {-2.0000,  0.0000,  0.4500}, {-0.8400, -1.5000,  0.2250},
        {-1.5000, -0.8400,  0.2250}, {-1.5000,  0.0000,  0.2250}, {-0.8400, -1.5000,  0.1500}, {-1.5000, -0.8400,  0.1500},
        {-1.5000,  0.0000,  0.1500}, {-2.0000,  1.1200,  0.4500}, {-1.1200,  2.0000,  0.4500}, { 0.0000,  2.0000,  0.4500},
        {-1.5000,  0.8400,  0.2250}, {-0.8400,  1.5000,  0.2250}, { 0.0000,  1.5000,  0.2250}, {-1.5000,  0.8400,  0.1500},
        {-0.8400,  1.5000,  0.1500}, { 0.0000,  1.5000,  0.1500}, { 1.1200,  2.0000,  0.4500}, { 2.0000,  1.1200,  0.4500},
        { 0.8400,  1.5000,  0.2250}, { 1.5000,  0.8400,  0.2250}, { 0.8400,  1.5000,  0.1500}, { 1.5000,  0.8400,  0.1500},
        {-1.6000,  0.0000,  2.0250}, {-1.6000, -0.3000,  2.0250}, {-1.5000, -0.3000,  2.2500}, {-1.5000,  0.0000,  2.2500},
        {-2.3000,  0.0000,  2.0250}, {-2.3000, -0.3000,  2.0250}, {-2.5000, -0.3000,  2.2500}, {-2.5000,  0.0000,  2.2500},
        {-2.7000,  0.0000,  2.0250}, {-2.7000, -0.3000,  2.0250}, {-3.0000, -0.3000,  2.2500}, {-3.0000,  0.0000,  2.2500},
        {-2.7000,  0.0000,  1.8000}, {-2.7000, -0.3000,  1.8000}, {-3.0000, -0.3000,  1.8000}, {-3.0000,  0.0000,  1.8000},
        {-1.5000,  0.3000,  2.2500}, {-1.6000,  0.3000,  2.0250}, {-2.5000,  0.3000,  2.2500}, {-2.3000,  0.3000,  2.0250},
        {-3.0000,  0.3000,  2.2500}, {-2.7000,  0.3000,  2.0250}, {-3.0000,  0.3000,  1.8000}, {-2.7000,  0.3000,  1.8000},
        {-2.7000,  0.0000,  1.5750}, {-2.7000, -0.3000,  1.5750}, {-3.0000, -0.3000,  1.3500}, {-3.0000,  0.0000,  1.3500},
        {-2.5000,  0.0000,  1.1250}, {-2.5000, -0.3000,  1.1250}, {-2.6500, -0.3000,  0.9375}, {-2.6500,  0.0000,  0.9375},
        {-2.0000, -0.3000,  0.9000}, {-1.9000, -0.3000,  0.6000}, {-1.9000,  0.0000,  0.6000}, {-3.0000,  0.3000,  1.3500},
        {-2.7000,  0.3000,  1.5750}, {-2.6500,  0.3000,  0.9375}, {-2.5000,  0.3000,  1.1250}, {-1.9000,  0.3000,  0.6000},
        {-2.0000,  0.3000,  0.9000}, { 1.7000,  0.0000,  1.4250}, { 1.7000, -0.6600,  1.4250}, { 1.7000, -0.6600,  0.6000},
        { 1.7000,  0.0000,  0.6000}, { 2.6000,  0.0000,  1.4250}, { 2.6000, -0.6600,  1.4250}, { 3.1000, -0.6600,  0.8250},
        { 3.1000,  0.0000,  0.8250}, { 2.3000,  0.0000,  2.1000}, { 2.3000, -0.2500,  2.1000}, { 2.4000, -0.2500,  2.0250},
        { 2.4000,  0.0000,  2.0250}, { 2.7000,  0.0000,  2.4000}, { 2.7000, -0.2500,  2.4000}, { 3.3000, -0.2500,  2.4000},
        { 3.3000,  0.0000,  2.4000}, { 1.7000,  0.6600,  0.6000}, { 1.7000,  0.6600,  1.4250}, { 3.1000,  0.6600,  0.8250},
        { 2.6000,  0.6600,  1.4250}, { 2.4000,  0.2500,  2.0250}, { 2.3000,  0.2500,  2.1000}, { 3.3000,  0.2500,  2.4000},
        { 2.7000,  0.2500,  2.4000}, { 2.8000,  0.0000,  2.4750}, { 2.8000, -0.2500,  2.4750}, { 3.5250, -0.2500,  2.4938},
        { 3.5250,  0.0000,  2.4938}, { 2.9000,  0.0000,  2.4750}, { 2.9000, -0.1500,  2.4750}, { 3.4500, -0.1500,  2.5125},
        { 3.4500,  0.0000,  2.5125}, { 2.8000,  0.0000,  2.4000}, { 2.8000, -0.1500,  2.4000}, { 3.2000, -0.1500,  2.4000},
        { 3.2000,  0.0000,  2.4000}, { 3.5250,  0.2500,  2.4938}, { 2.8000,  0.2500,  2.4750}, { 3.4500,  0.1500,  2.5125},
        { 2.9000,  0.1500,  2.4750}, { 3.2000,  0.1500,  2.4000}, { 2.8000,  0.1500,  2.4000}, { 0.0000,  0.0000,  3.1500},
        { 0.0000, -0.0020,  3.1500}, { 0.0020,  0.0000,  3.1500}, { 0.8000,  0.0000,  3.1500}, { 0.8000, -0.4500,  3.1500},
        { 0.4500, -0.8000,  3.1500}, { 0.0000, -0.8000,  3.1500}, { 0.0000,  0.0000,  2.8500}, { 0.2000,  0.0000,  2.7000},
        { 0.2000, -0.1120,  2.7000}, { 0.1120, -0.2000,  2.7000}, { 0.0000, -0.2000,  2.7000}, {-0.0020,  0.0000,  3.1500},
        {-0.4500, -0.8000,  3.1500}, {-0.8000, -0.4500,  3.1500}, {-0.8000,  0.0000,  3.1500}, {-0.1120, -0.2000,  2.7000},
        {-0.2000, -0.1120,  2.7000}, {-0.2000,  0.0000,  2.7000}, { 0.0000,  0.0020,  3.1500}, {-0.8000,  0.4500,  3.1500},
        {-0.4500,  0.8000,  3.1500}, { 0.0000,  0.8000,  3.1500}, {-0.2000,  0.1120,  2.7000}, {-0.1120,  0.2000,  2.7000},
        { 0.0000,  0.2000,  2.7000}, { 0.4500,  0.8000,  3.1500}, { 0.8000,  0.4500,  3.1500}, { 0.1120,  0.2000,  2.7000},
        { 0.2000,  0.1120,  2.7000}, { 0.4000,  0.0000,  2.5500}, { 0.4000, -0.2240,  2.5500}, { 0.2240, -0.4000,  2.5500},
        { 0.0000, -0.4000,  2.5500}, { 1.3000,  0.0000,  2.5500}, { 1.3000, -0.7280,  2.5500}, { 0.7280, -1.3000,  2.5500},
        { 0.0000, -1.3000,  2.5500}, { 1.3000,  0.0000,  2.4000}, { 1.3000, -0.7280,  2.4000}, { 0.7280, -1.3000,  2.4000},
        { 0.0000, -1.3000,  2.4000}, {-0.2240, -0.4000,  2.5500}, {-0.4000, -0.2240,  2.5500}, {-0.4000,  0.0000,  2.5500},
        {-0.7280, -1.3000,  2.5500}, {-1.3000, -0.7280,  2.5500}, {-1.3000,  0.0000,  2.5500}, {-0.7280, -1.3000,  2.4000},
        {-1.3000, -0.7280,  2.4000}, {-1.3000,  0.0000,  2.4000}, {-0.4000,  0.2240,  2.5500}, {-0.2240,  0.4000,  2.5500},
        { 0.0000,  0.4000,  2.5500}, {-1.3000,  0.7280,  2.5500}, {-0.7280,  1.3000,  2.5500}, { 0.0000,  1.3000,  2.5500},
        {-1.3000,  0.7280,  2.4000}, {-0.7280,  1.3000,  2.4000}, { 0.0000,  1.3000,  2.4000}, { 0.2240,  0.4000,  2.5500},
        { 0.4000,  0.2240,  2.5500}, { 0.7280,  1.3000,  2.5500}, { 1.3000,  0.7280,  2.5500}, { 0.7280,  1.3000,  2.4000},
        { 1.3000,  0.7280,  2.4000}, { 0.0000,  0.0000,  0.0000}, { 1.5000,  0.0000,  0.1500}, { 1.5000,  0.8400,  0.1500},
        { 0.8400,  1.5000,  0.1500}, { 0.0000,  1.5000,  0.1500}, { 1.5000,  0.0000,  0.0750}, { 1.5000,  0.8400,  0.0750},
        { 0.8400,  1.5000,  0.0750}, { 0.0000,  1.5000,  0.0750}, { 1.4250,  0.0000,  0.0000}, { 1.4250,  0.7980,  0.0000},
        { 0.7980,  1.4250,  0.0000}, { 0.0000,  1.4250,  0.0000}, {-0.8400,  1.5000,  0.1500}, {-1.5000,  0.8400,  0.1500},
        {-1.5000,  0.0000,  0.1500}, {-0.8400,  1.5000,  0.0750}, {-1.5000,  0.8400,  0.0750}, {-1.5000,  0.0000,  0.0750},
        {-0.7980,  1.4250,  0.0000}, {-1.4250,  0.7980,  0.0000}, {-1.4250,  0.0000,  0.0000}, {-1.5000, -0.8400,  0.1500},
        {-0.8400, -1.5000,  0.1500}, { 0.0000, -1.5000,  0.1500}, {-1.5000, -0.8400,  0.0750}, {-0.8400, -1.5000,  0.0750},
        { 0.0000, -1.5000,  0.0750}, {-1.4250, -0.7980,  0.0000}, {-0.7980, -1.4250,  0.0000}, { 0.0000, -1.4250,  0.0000},
        { 0.8400, -1.5000,  0.1500}, { 1.5000, -0.8400,  0.1500}, { 0.8400, -1.5000,  0.0750}, { 1.5000, -0.8400,  0.0750},
        { 0.7980, -1.4250,  0.0000}, { 1.4250, -0.7980,  0.0000}
    };

    constexpr int32 TeapotPatches[NumPatches][16] =
    {
        {  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16},
        {  4,  17,  18,  19,   8,  20,  21,  22,  12,  23,  24,  25,  16,  26,  27,  28},
        { 19,  29,  30,  31,  22,  32,  33,  34,  25,  35,  36,  37,  28,  38,  39,  40},
        { 31,  41,  42,   1,  34,  43,  44,   5,  37,  45,  46,   9,  40,  47,  48,  13},
        { 13,  14,  15,  16,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60},
        { 16,  26,  27,  28,  52,  61,  62,  63,  56,  64,  65,  66,  60,  67,  68,  69},
        { 28,  38,  39,  40,  63,  70,  71,  72,  66,  73,  74,  75,  69,  76,  77,  78},
        { 40,  47,  48,  13,  72,  79,  80,  49,  75,  81,  82,  53,  78,  83,  84,  57},
        { 57,  58,  59,  60,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96},
        { 60,  67,  68,  69,  88,  97,  98,  99,  92, 100, 101, 102,  96, 103, 104, 105},
        { 69,  76,  77,  78,  99, 106, 107, 108, 102, 109, 110, 111, 105, 112, 113, 114},
        { 78,  83,  84,  57, 108, 115, 116,  85, 111, 117, 118,  89, 114, 119, 120,  93},
        {121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136},
        {124, 137, 138, 121, 128, 139, 140, 125, 132, 141, 142, 129, 136, 143, 144, 133},
        {133, 134, 135, 136, 145, 146, 147, 148, 149, 150, 151, 152,  69, 153, 154, 155},
        {136, 143, 144, 133, 148, 156, 157, 145, 152, 158, 159, 149, 155, 160, 161,  69},
        {162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177},
        {165, 178, 179, 162, 169, 180, 181, 166, 173, 182, 183, 170, 177, 184, 185, 174},
        {174, 175, 176, 177, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197},
        {177, 184, 185, 174, 189, 198, 199, 186, 193, 200, 201, 190, 197, 202, 203, 194},
        {204, 204, 204, 204, 207, 208, 209, 210, 211, 211, 211, 211, 212, 213, 214, 215},
        {204, 204, 204, 204, 210, 217, 218, 219, 211, 211, 211, 211, 215, 220, 221, 222},
        {204, 204, 204, 204, 219, 224, 225, 226, 211, 211, 211, 211, 222, 227, 228, 229},
        {204, 204, 204, 204, 226, 230, 231, 207, 211, 211, 211, 211, 229, 232, 233, 212},
        {212, 213, 214, 215, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245},
        {215, 220, 221, 222, 237, 246, 247, 248, 241, 249, 250, 251, 245, 252, 253, 254},
        {222, 227, 228, 229, 248, 255, 256, 257, 251, 258, 259, 260, 254, 261, 262, 263},
        {229, 232, 233, 212, 257, 264, 265, 234, 260, 266, 267, 238, 263, 268, 269, 242},
        {270, 270, 270, 270, 279, 280, 281, 282, 275, 276, 277, 278, 271, 272, 273, 274},
        {270, 270, 270, 270, 282, 289, 290, 291, 278, 286, 287, 288, 274, 283, 284, 285},
        {270, 270, 270, 270, 291, 298, 299, 300, 288, 295, 296, 297, 285, 292, 293, 294},
        {270, 270, 270, 270, 300, 305, 306, 279, 297, 303, 304, 275, 294, 301, 302, 271}
    };

    // Bernstein polynomials and their derivatives
    const auto Bernstein = [](float t, float* B, float* dB)
    {
        const float it = 1.0f - t;
        B[0] = it * it * it;
        B[1] = 3 * t * it * it;
        B[2] = 3 * t * t * it;
        B[3] = t * t * t;

        dB[0] = -3 * it * it;
        dB[1] = 3 * it * it - 6 * t * it;
        dB[2] = 6 * t * it - 3 * t * t;
        dB[3] = 3 * t * t;
    };

    // Function to evaluate a point and normal on a Bezier patch
    const auto EvaluateBezierPatch = [&](const float ControlPoints[16][3], float u, float v, FSourceVertex& Vertex)
    {
        float Bu[4];
        float Bv[4];
        float dBu[4];
        float dBv[4];
        Bernstein(u, Bu, dBu);
        Bernstein(v, Bv, dBv);

        float Position[3] = { 0.0f, 0.0f, 0.0f };
        float TangentU[3] = { 0.0f, 0.0f, 0.0f };
        float TangentV[3] = { 0.0f, 0.0f, 0.0f };
        for (int32 i = 0; i < 4; ++i)
        {
            for (int32 j = 0; j < 4; ++j)
            {
                float Basis   = Bu[i] * Bv[j];
                float dBasisU = dBu[i] * Bv[j];
                float dBasisV = Bu[i] * dBv[j];

                const float* Point = ControlPoints[i * 4 + j];
                for (int32 Axis = 0; Axis < 3; ++Axis)
                {
                    Position[Axis] += Basis * Point[Axis];
                    TangentU[Axis] += dBasisU * Point[Axis];
                    TangentV[Axis] += dBasisV * Point[Axis];
                }
            }
        }

        // Invert Y-axis for position
        Vertex.Position[0] = Position[0];
        Vertex.Position[1] = Position[1];
        Vertex.Position[2] = Position[2];

        Vector3 Normal(
            TangentU[1] * TangentV[2] - TangentU[2] * TangentV[1],
            TangentU[2] * TangentV[0] - TangentU[0] * TangentV[2],
            TangentU[0] * TangentV[1] - TangentU[1] * TangentV[0]);
        
        Normal.Normalize();

        // Invert normal
        Vertex.Normal = -Normal;

        // Set texture coordinates
        Vertex.TexCoord[0] = u;
        Vertex.TexCoord[1] = v;
    };

    FMeshData MeshData;
    if (Tessellation < 1)
    {
        Tessellation = 1;
    }

    for (uint32 PatchIndex = 0; PatchIndex < NumPatches; ++PatchIndex)
    {
        float PatchControlPoints[16][3];
        for (uint32 i = 0; i < 16; ++i)
        {
            const uint32 ControlPointIndex = TeapotPatches[PatchIndex][i] - 1;
            PatchControlPoints[i][0] = TeapotControlPoints[ControlPointIndex][0];
            PatchControlPoints[i][1] = TeapotControlPoints[ControlPointIndex][1];
            PatchControlPoints[i][2] = TeapotControlPoints[ControlPointIndex][2];
        }

        const uint32 StartVertex = MeshData.Vertices.Size();
        for (uint32 u = 0; u <= Tessellation; ++u)
        {
            const float t = static_cast<float>(u) / static_cast<float>(Tessellation);
            for (uint32 v = 0; v <= Tessellation; ++v)
            {
                const float s = static_cast<float>(v) / static_cast<float>(Tessellation);

                FSourceVertex Vertex;
                EvaluateBezierPatch(PatchControlPoints, t, s, Vertex);
                MeshData.Vertices.Add(Vertex);
            }
        }

        for (uint32 u = 0; u < Tessellation; ++u)
        {
            for (uint32 v = 0; v < Tessellation; ++v)
            {
                const uint32 Index0 = StartVertex + (u * (Tessellation + 1)) + v;
                const uint32 Index1 = StartVertex + ((u + 1) * (Tessellation + 1)) + v;
                const uint32 Index2 = StartVertex + (u * (Tessellation + 1)) + v + 1;
                const uint32 Index3 = StartVertex + ((u + 1) * (Tessellation + 1)) + v + 1;

                MeshData.Indices.Add(Index0);
                MeshData.Indices.Add(Index2);
                MeshData.Indices.Add(Index1);

                MeshData.Indices.Add(Index1);
                MeshData.Indices.Add(Index2);
                MeshData.Indices.Add(Index3);
            }
        }
    }

    // Calculate tangents for proper lighting and normal mapping
    MeshData.CalculateTangents();
    return MeshData;
}

FMeshData MeshFactory::CreatePyramid(float Width, float Depth, float Height) noexcept
{
    FMeshData MeshData;

    float HalfWidth = Width / 2.0f;
    float HalfDepth = Depth / 2.0f;

    // Bottom vertices
    FSourceVertex v0;
    v0.Position = Vector3(-HalfWidth, 0.0f, -HalfDepth); // Front-left
    FSourceVertex v1;
    v1.Position = Vector3( HalfWidth, 0.0f, -HalfDepth); // Front-right
    FSourceVertex v2;
    v2.Position = Vector3( HalfWidth, 0.0f,  HalfDepth); // Back-right
    FSourceVertex v3;
    v3.Position = Vector3(-HalfWidth, 0.0f,  HalfDepth); // Back-left

    // Apex vertex
    FSourceVertex v4;
    v4.Position = Vector3(0.0f, Height, 0.0f); // Top center

    // Base normal
    Vector3 BaseNormal = Vector3(0.0f, -1.0f, 0.0f);

    // Side normals (calculated for each face)
    Vector3 Normal0 = (v4.Position - v0.Position).CrossProduct(v1.Position - v0.Position).Normalize();
    Vector3 Normal1 = (v4.Position - v1.Position).CrossProduct(v2.Position - v1.Position).Normalize();
    Vector3 Normal2 = (v4.Position - v2.Position).CrossProduct(v3.Position - v2.Position).Normalize();
    Vector3 Normal3 = (v4.Position - v3.Position).CrossProduct(v0.Position - v3.Position).Normalize();

    // Assign normals to the base vertices
    v0.Normal = BaseNormal;
    v1.Normal = BaseNormal;
    v2.Normal = BaseNormal;
    v3.Normal = BaseNormal;

    // Assign texture coordinates for the base
    v0.TexCoord = Vector2(0.0f, 0.0f);
    v1.TexCoord = Vector2(1.0f, 0.0f);
    v2.TexCoord = Vector2(1.0f, 1.0f);
    v3.TexCoord = Vector2(0.0f, 1.0f);

    // Add base vertices to the mesh
    uint32 BaseIndex = static_cast<uint32>(MeshData.Vertices.Size());
    MeshData.Vertices.Add(v0); // Index 0
    MeshData.Vertices.Add(v1); // Index 1
    MeshData.Vertices.Add(v2); // Index 2
    MeshData.Vertices.Add(v3); // Index 3

    // Add indices for the base (two triangles)
    MeshData.Indices.Add(BaseIndex + 0);
    MeshData.Indices.Add(BaseIndex + 1);
    MeshData.Indices.Add(BaseIndex + 2);

    MeshData.Indices.Add(BaseIndex + 0);
    MeshData.Indices.Add(BaseIndex + 2);
    MeshData.Indices.Add(BaseIndex + 3);

    // Side 1 (v0, v1, v4)
    FSourceVertex s0v0 = v0;
    FSourceVertex s0v1 = v1;
    FSourceVertex s0v4 = v4;
    s0v0.Normal   = Normal0;
    s0v1.Normal   = Normal0;
    s0v4.Normal   = Normal0;
    s0v0.TexCoord = Vector2(0.0f, 0.0f);
    s0v1.TexCoord = Vector2(1.0f, 0.0f);
    s0v4.TexCoord = Vector2(0.5f, 1.0f);

    const uint32 Side0Index = static_cast<uint32>(MeshData.Vertices.Size());
    MeshData.Vertices.Add(s0v0);
    MeshData.Vertices.Add(s0v1);
    MeshData.Vertices.Add(s0v4);

    MeshData.Indices.Add(Side0Index + 0);
    MeshData.Indices.Add(Side0Index + 2);
    MeshData.Indices.Add(Side0Index + 1);

    // Side 2 (v1, v2, v4)
    FSourceVertex s1v0 = v1;
    FSourceVertex s1v1 = v2;
    FSourceVertex s1v4 = v4;
    s1v0.Normal   = Normal1;
    s1v1.Normal   = Normal1;
    s1v4.Normal   = Normal1;
    s1v0.TexCoord = Vector2(0.0f, 0.0f);
    s1v1.TexCoord = Vector2(1.0f, 0.0f);
    s1v4.TexCoord = Vector2(0.5f, 1.0f);

    const uint32 Side1Index = static_cast<uint32>(MeshData.Vertices.Size());
    MeshData.Vertices.Add(s1v0);
    MeshData.Vertices.Add(s1v1);
    MeshData.Vertices.Add(s1v4);

    MeshData.Indices.Add(Side1Index + 0);
    MeshData.Indices.Add(Side1Index + 2);
    MeshData.Indices.Add(Side1Index + 1);

    // Side 3 (v2, v3, v4)
    FSourceVertex s2v0 = v2;
    FSourceVertex s2v1 = v3;
    FSourceVertex s2v4 = v4;
    s2v0.Normal   = Normal2;
    s2v1.Normal   = Normal2;
    s2v4.Normal   = Normal2;
    s2v0.TexCoord = Vector2(0.0f, 0.0f);
    s2v1.TexCoord = Vector2(1.0f, 0.0f);
    s2v4.TexCoord = Vector2(0.5f, 1.0f);

    const uint32 Side2Index = static_cast<uint32>(MeshData.Vertices.Size());
    MeshData.Vertices.Add(s2v0);
    MeshData.Vertices.Add(s2v1);
    MeshData.Vertices.Add(s2v4);

    MeshData.Indices.Add(Side2Index + 0);
    MeshData.Indices.Add(Side2Index + 2);
    MeshData.Indices.Add(Side2Index + 1);

    // Side 4 (v3, v0, v4)
    FSourceVertex s3v0 = v3;
    FSourceVertex s3v1 = v0;
    FSourceVertex s3v4 = v4;
    s3v0.Normal   = Normal3;
    s3v1.Normal   = Normal3;
    s3v4.Normal   = Normal3;
    s3v0.TexCoord = Vector2(0.0f, 0.0f);
    s3v1.TexCoord = Vector2(1.0f, 0.0f);
    s3v4.TexCoord = Vector2(0.5f, 1.0f);

    const uint32 Side3Index = static_cast<uint32>(MeshData.Vertices.Size());
    MeshData.Vertices.Add(s3v0);
    MeshData.Vertices.Add(s3v1);
    MeshData.Vertices.Add(s3v4);

    MeshData.Indices.Add(Side3Index + 0);
    MeshData.Indices.Add(Side3Index + 2);
    MeshData.Indices.Add(Side3Index + 1);

    MeshData.CalculateTangents();
    return MeshData;
}

FMeshData MeshFactory::CreateCylinder(uint32 Sides, float Radius, float Height) noexcept
{
    FMeshData MeshData;

    // Validate the number of sides
    if (Sides < 3)
    {
        // A cylinder must have at least 3 sides
        Sides = 3;
    }

    // Half height for positioning the cylinder centered at Y = 0
    const float HalfHeight = Height / 2.0f;

    // Angle increment per side
    const float DeltaAngle = 2.0f * Math::Constants::PI / static_cast<float>(Sides);

    // Generate top cap vertices
    FSourceVertex TopCenterVertex;
    TopCenterVertex.Position = Vector3(0.0f, HalfHeight, 0.0f);
    TopCenterVertex.Normal   = Vector3(0.0f, 1.0f, 0.0f);
    TopCenterVertex.TexCoord = Vector2(0.5f, 0.5f); // Center of the texture
    MeshData.Vertices.Add(TopCenterVertex);

    for (uint32 i = 0; i < Sides; ++i)
    {
        const float Angle = i * DeltaAngle;
        const float X     = Radius * cosf(Angle);
        const float Z     = Radius * sinf(Angle);
        
        FSourceVertex Vertex;
        Vertex.Position = Vector3(X, HalfHeight, Z);
        Vertex.Normal   = Vector3(0.0f, 1.0f, 0.0f);
        Vertex.TexCoord = Vector2((cosf(Angle) + 1.0f) * 0.5f, (sinf(Angle) + 1.0f) * 0.5f); // Map to [0,1]
        MeshData.Vertices.Add(Vertex);
    }

    // Generate bottom cap vertices
    const uint32 BottomCenterIndex = static_cast<uint32>(MeshData.Vertices.Size());

    FSourceVertex BottomCenterVertex;
    BottomCenterVertex.Position = Vector3(0.0f, -HalfHeight, 0.0f);
    BottomCenterVertex.Normal   = Vector3(0.0f, -1.0f, 0.0f);
    BottomCenterVertex.TexCoord = Vector2(0.5f, 0.5f);
    MeshData.Vertices.Add(BottomCenterVertex);

    for (uint32 i = 0; i < Sides; ++i)
    {
        const float Angle = i * DeltaAngle;
        const float X     = Radius * cosf(Angle);
        const float Z     = Radius * sinf(Angle);

        FSourceVertex Vertex;
        Vertex.Position = Vector3(X, -HalfHeight, Z);
        Vertex.Normal   = Vector3(0.0f, -1.0f, 0.0f);
        Vertex.TexCoord = Vector2((cosf(Angle) + 1.0f) * 0.5f, (sinf(Angle) + 1.0f) * 0.5f);
        MeshData.Vertices.Add(Vertex);
    }

    // Generate side vertices
    const uint32 SideStartIndex = static_cast<uint32>(MeshData.Vertices.Size());
    for (uint32 i = 0; i <= Sides; ++i) // <= to wrap around
    {
        const float Angle = (i % Sides) * DeltaAngle;
        const float X     = Radius * cosf(Angle);
        const float Z     = Radius * sinf(Angle);
        const float U     = static_cast<float>(i) / static_cast<float>(Sides); // Texture coordinate U

        const Vector3 Normal = Vector3(X, 0.0f, Z).Normalize();

        // Top vertex
        FSourceVertex TopVertex;
        TopVertex.Position = Vector3(X, HalfHeight, Z);
        TopVertex.Normal   = Normal;
        TopVertex.TexCoord = Vector2(U, 0.0f); // V = 0 at the top
        MeshData.Vertices.Add(TopVertex);

        // Bottom vertex
        FSourceVertex BottomVertex;
        BottomVertex.Position = Vector3(X, -HalfHeight, Z);
        BottomVertex.Normal   = Normal;
        BottomVertex.TexCoord = Vector2(U, 1.0f); // V = 1 at the bottom
        MeshData.Vertices.Add(BottomVertex);
    }

    // Generate indices for the top cap
    for (uint32 i = 0; i < Sides; ++i)
    {
        const uint32 CenterIndex = 0;
        const uint32 CurrIndex   = i + 1;
        const uint32 NextIndex   = (i + 1) % Sides + 1;

        MeshData.Indices.Add(CenterIndex);
        MeshData.Indices.Add(NextIndex);
        MeshData.Indices.Add(CurrIndex);
    }

    // Generate indices for the bottom cap
    const uint32 BottomStartIndex        = BottomCenterIndex + 1;
    const uint32 BottomCenterVertexIndex = BottomCenterIndex;

    for (uint32 i = 0; i < Sides; ++i)
    {
        const uint32 CurrIndex = BottomStartIndex + i;
        const uint32 NextIndex = BottomStartIndex + ((i + 1) % Sides);

        MeshData.Indices.Add(BottomCenterVertexIndex);
        MeshData.Indices.Add(CurrIndex);
        MeshData.Indices.Add(NextIndex);
    }

    // Generate indices for the sides
    for (uint32 i = 0; i < Sides; ++i)
    {
        const uint32 TopCurr    = SideStartIndex + i * 2;
        const uint32 TopNext    = SideStartIndex + (i + 1) * 2;
        const uint32 BottomCurr = TopCurr + 1;
        const uint32 BottomNext = TopNext + 1;

        MeshData.Indices.Add(TopCurr);
        MeshData.Indices.Add(TopNext);
        MeshData.Indices.Add(BottomCurr);

        MeshData.Indices.Add(BottomCurr);
        MeshData.Indices.Add(TopNext);
        MeshData.Indices.Add(BottomNext);
    }

    MeshData.CalculateTangents();
    return MeshData;
}
