#include "Engine/Engine.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/Assets/ModelCreateInfo.h"

TArray<uint16> FMeshCreateInfo::GetSmallIndices() const
{
    TArray<uint16> NewArray;
    NewArray.Reserve(Indices.Size());

    for (uint32 Index : Indices)
    {
        NewArray.Add(static_cast<uint16>(Index));
    }

    return NewArray;
}

void FMeshCreateInfo::Optimize(uint32 StartVertex)
{
    uint32 VertexCount = static_cast<uint32>(Vertices.Size());
    uint32 IndexCount  = static_cast<uint32>(Indices.Size());

    uint32 k = 0;
    uint32 j = 0;
    for (uint32 i = StartVertex; i < VertexCount; i++)
    {
        for (j = 0; j < VertexCount; j++)
        {
            if (Vertices[i] == Vertices[j])
            {
                if (i != j)
                {
                    Vertices.RemoveAt(i);
                    VertexCount--;
                    j--;

                    for (k = 0; k < IndexCount; k++)
                    {
                        if (Indices[k] == i)
                        {
                            Indices[k] = j;
                        }
                        else if (Indices[k] > i)
                        {
                            Indices[k]--;
                        }
                    }

                    i--;
                    break;
                }
            }
        }
    }
}

void FMeshCreateInfo::CalculateHardNormals()
{
    CHECK(Indices.Size() % 3 == 0);

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        FVertex& Vertex0 = Vertices[Indices[i + 0]];
        FVertex& Vertex1 = Vertices[Indices[i + 1]];
        FVertex& Vertex2 = Vertices[Indices[i + 2]];

        FVector3 Edge0  = Vertex2.Position - Vertex0.Position;
        FVector3 Edge1  = Vertex1.Position - Vertex0.Position;
        FVector3 Normal = Edge0.CrossProduct(Edge1);
        Normal.Normalize();

        Vertex0.Normal = Normal;
        Vertex1.Normal = Normal;
        Vertex2.Normal = Normal;
    }
}

void FMeshCreateInfo::CalculateSoftNormals()
{
    CHECK(Indices.Size() % 3 == 0);

    // TODO: Write better version. For now calculate the hard normals and then average all of them
    CalculateHardNormals();

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        FVertex& Vertex0 = Vertices[Indices[i + 0]];
        FVertex& Vertex1 = Vertices[Indices[i + 1]];
        FVertex& Vertex2 = Vertices[Indices[i + 2]];

        FVector3 Edge0  = Vertex2.Position - Vertex0.Position;
        FVector3 Edge1  = Vertex1.Position - Vertex0.Position;
        FVector3 Normal = Edge0.CrossProduct(Edge1);
        Normal.Normalize();

        // Average current and new normal
        Vertex0.Normal = (Vertex0.Normal + Normal) * 0.5f;
        Vertex0.Normal.Normalize();
        Vertex1.Normal = (Vertex1.Normal + Normal) * 0.5f;
        Vertex1.Normal.Normalize();
        Vertex2.Normal = (Vertex2.Normal + Normal) * 0.5f;
        Vertex2.Normal.Normalize();
    }
}

void FMeshCreateInfo::CalculateTangents()
{
    CHECK(Indices.Size() % 3 == 0);

    auto CalculateTangentFromVectors = [](FVertex& Vertex1, const FVertex& Vertex2, const FVertex& Vertex3)
    {
        FVector3 Edge1   = Vertex2.Position - Vertex1.Position;
        FVector3 Edge2   = Vertex3.Position - Vertex1.Position;
        FVector2 UVEdge1 = Vertex2.TexCoord - Vertex1.TexCoord;
        FVector2 UVEdge2 = Vertex3.TexCoord - Vertex1.TexCoord;

        const float RecipDenominator = 1.0f / (UVEdge1.x * UVEdge2.y - UVEdge2.x * UVEdge1.y);

        FVector3 Tangent = RecipDenominator * ((UVEdge2.y * Edge1) - (UVEdge1.y * Edge2));
        Tangent.Normalize();

        Vertex1.Tangent = Tangent;
    };

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        FVertex& Vertex1 = Vertices[Indices[i + 0]];
        FVertex& Vertex2 = Vertices[Indices[i + 1]];
        FVertex& Vertex3 = Vertices[Indices[i + 2]];

        CalculateTangentFromVectors(Vertex1, Vertex2, Vertex3);
        CalculateTangentFromVectors(Vertex2, Vertex3, Vertex1);
        CalculateTangentFromVectors(Vertex3, Vertex1, Vertex2);
    }
}

void FMeshCreateInfo::ReverseHandedness()
{
    CHECK(Indices.Size() % 3 == 0);

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        uint32 TempIndex = Indices[i + 1];
        Indices[i + 1] = Indices[i + 2];
        Indices[i + 2] = TempIndex;
    }

    for (int32 i = 0; i < Vertices.Size(); ++i)
    {
        Vertices[i].Position.z = Vertices[i].Position.z * -1.0f;
        Vertices[i].Normal.z   = Vertices[i].Normal.z   * -1.0f;
    }
}

void FMeshCreateInfo::Subdivide(uint32 Subdivisions)
{
    if (Subdivisions < 1)
    {
        return;
    }

    FVertex TempVertices[3];
    uint32 IndexCount     = 0;
    uint32 VertexCount    = 0;
    uint32 OldVertexCount = 0;

    Vertices.Reserve((Vertices.Size() * static_cast<uint32>(pow(2, Subdivisions))));
    Indices.Reserve((Indices.Size() * static_cast<uint32>(pow(4, Subdivisions))));

    for (uint32 i = 0; i < Subdivisions; i++)
    {
        OldVertexCount = uint32(Vertices.Size());
        IndexCount     = uint32(Indices.Size());

        CHECK(IndexCount % 3 == 0);

        for (uint32 j = 0; j < IndexCount; j += 3)
        {
            // Calculate Position
            FVector3 Position0 = Vertices[Indices[j]].Position;
            FVector3 Position1 = Vertices[Indices[j + 1]].Position;
            FVector3 Position2 = Vertices[Indices[j + 2]].Position;

            FVector3 Position = Position0 + Position1;
            TempVertices[0].Position = Position * 0.5f;

            Position = Position0 + Position2;
            TempVertices[1].Position = Position * 0.5f;

            Position = Position1 + Position2;
            TempVertices[2].Position = Position * 0.5f;

            // Calculate TexCoord
            FVector2 TexCoord0 = Vertices[Indices[j]].TexCoord;
            FVector2 TexCoord1 = Vertices[Indices[j + 1]].TexCoord;
            FVector2 TexCoord2 = Vertices[Indices[j + 2]].TexCoord;

            FVector2 TexCoord = TexCoord0 + TexCoord1;
            TempVertices[0].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord0 + TexCoord2;
            TempVertices[1].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord1 + TexCoord2;
            TempVertices[2].TexCoord = TexCoord * 0.5f;

            // Calculate Normal
            FVector3 Normal0 = Vertices[Indices[j]].Normal;
            FVector3 Normal1 = Vertices[Indices[j + 1]].Normal;
            FVector3 Normal2 = Vertices[Indices[j + 2]].Normal;

            FVector3 Normal = Normal0 + Normal1;
            Normal = Normal * 0.5f;
            TempVertices[0].Normal = Normal.GetNormalized();

            Normal = Normal0 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[1].Normal = Normal.GetNormalized();

            Normal = Normal1 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[2].Normal = Normal.GetNormalized();

            // Calculate Tangent
            FVector3 Tangent0 = Vertices[Indices[j]].Tangent;
            FVector3 Tangent1 = Vertices[Indices[j + 1]].Tangent;
            FVector3 Tangent2 = Vertices[Indices[j + 2]].Tangent;

            FVector3 Tangent = Tangent0 + Tangent1;
            Tangent = Tangent * 0.5f;
            TempVertices[0].Tangent = Tangent.GetNormalized();

            Tangent = Tangent0 + Tangent2;
            Tangent = Tangent * 0.5f;
            TempVertices[1].Tangent = Tangent.GetNormalized();

            Tangent = Tangent1 + Tangent2;
            Tangent = Tangent * 0.5f;
            TempVertices[2].Tangent = Tangent.GetNormalized();

            // Add the new Vertices
            Vertices.Emplace(TempVertices[0]);
            Vertices.Emplace(TempVertices[1]);
            Vertices.Emplace(TempVertices[2]);

            // Add index of the new triangles
            VertexCount = uint32(Vertices.Size());
            Indices.Emplace(VertexCount - 3);
            Indices.Emplace(VertexCount - 1);
            Indices.Emplace(VertexCount - 2);

            Indices.Emplace(VertexCount - 3);
            Indices.Emplace(Indices[j + 1]);
            Indices.Emplace(VertexCount - 1);

            Indices.Emplace(VertexCount - 2);
            Indices.Emplace(VertexCount - 1);
            Indices.Emplace(Indices[j + 2]);

            // Reassign the old indexes
            Indices[j + 1] = VertexCount - 3;
            Indices[j + 2] = VertexCount - 2;
        }

        Optimize(OldVertexCount);
    }

    Vertices.Shrink();
    Indices.Shrink();
}

FMeshCreateInfo FMeshFactory::CreateCube(float Width, float Height, float Depth) noexcept
{
    const float HalfWidth  = Width  * 0.5f;
    const float HalfHeight = Height * 0.5f;
    const float HalfDepth  = Depth  * 0.5f;

    FMeshCreateInfo CubeInfo;
    CubeInfo.Vertices =
    {
        // FRONT FACE
        { FVector3(-HalfWidth,  HalfHeight, -HalfDepth), FVector3(0.0f,  0.0f, -1.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 0.0f) },
        { FVector3( HalfWidth,  HalfHeight, -HalfDepth), FVector3(0.0f,  0.0f, -1.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 0.0f) },
        { FVector3(-HalfWidth, -HalfHeight, -HalfDepth), FVector3(0.0f,  0.0f, -1.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 1.0f) },
        { FVector3( HalfWidth, -HalfHeight, -HalfDepth), FVector3(0.0f,  0.0f, -1.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 1.0f) },

        // BACK FACE
        { FVector3( HalfWidth,  HalfHeight,  HalfDepth), FVector3(0.0f,  0.0f,  1.0f), FVector3(-1.0f,  0.0f, 0.0f), FVector2(0.0f, 0.0f) },
        { FVector3(-HalfWidth,  HalfHeight,  HalfDepth), FVector3(0.0f,  0.0f,  1.0f), FVector3(-1.0f,  0.0f, 0.0f), FVector2(1.0f, 0.0f) },
        { FVector3( HalfWidth, -HalfHeight,  HalfDepth), FVector3(0.0f,  0.0f,  1.0f), FVector3(-1.0f,  0.0f, 0.0f), FVector2(0.0f, 1.0f) },
        { FVector3(-HalfWidth, -HalfHeight,  HalfDepth), FVector3(0.0f,  0.0f,  1.0f), FVector3(-1.0f,  0.0f, 0.0f), FVector2(1.0f, 1.0f) },

        // RIGHT FACE
        { FVector3(HalfWidth,  HalfHeight, -HalfDepth), FVector3(1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(0.0f, 0.0f) },
        { FVector3(HalfWidth,  HalfHeight,  HalfDepth), FVector3(1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(1.0f, 0.0f) },
        { FVector3(HalfWidth, -HalfHeight, -HalfDepth), FVector3(1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(0.0f, 1.0f) },
        { FVector3(HalfWidth, -HalfHeight,  HalfDepth), FVector3(1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(1.0f, 1.0f) },

        // LEFT FACE
        { FVector3(-HalfWidth,  HalfHeight, -HalfDepth), FVector3(-1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(0.0f, 1.0f) },
        { FVector3(-HalfWidth,  HalfHeight,  HalfDepth), FVector3(-1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(1.0f, 1.0f) },
        { FVector3(-HalfWidth, -HalfHeight, -HalfDepth), FVector3(-1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(0.0f, 0.0f) },
        { FVector3(-HalfWidth, -HalfHeight,  HalfDepth), FVector3(-1.0f,  0.0f,  0.0f), FVector3(0.0f,  0.0f, 1.0f), FVector2(1.0f, 0.0f) },

        // TOP FACE
        { FVector3(-HalfWidth,  HalfHeight,  HalfDepth), FVector3(0.0f,  1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 0.0f) },
        { FVector3( HalfWidth,  HalfHeight,  HalfDepth), FVector3(0.0f,  1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 0.0f) },
        { FVector3(-HalfWidth,  HalfHeight, -HalfDepth), FVector3(0.0f,  1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 1.0f) },
        { FVector3( HalfWidth,  HalfHeight, -HalfDepth), FVector3(0.0f,  1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 1.0f) },

        // BOTTOM FACE
        { FVector3(-HalfWidth, -HalfHeight, -HalfDepth), FVector3(0.0f, -1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 0.0f) },
        { FVector3( HalfWidth, -HalfHeight, -HalfDepth), FVector3(0.0f, -1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 0.0f) },
        { FVector3(-HalfWidth, -HalfHeight,  HalfDepth), FVector3(0.0f, -1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(0.0f, 1.0f) },
        { FVector3( HalfWidth, -HalfHeight,  HalfDepth), FVector3(0.0f, -1.0f,  0.0f), FVector3(1.0f,  0.0f, 0.0f), FVector2(1.0f, 1.0f) },
    };

    CubeInfo.Indices =
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

    return CubeInfo;
}

FMeshCreateInfo FMeshFactory::CreatePlane(uint32 Width, uint32 Height) noexcept
{
    FMeshCreateInfo PlaneInfo;
    if (Width < 1)
    {
        Width = 1;
    }
    if (Height < 1)
    {
        Height = 1;
    }

    PlaneInfo.Vertices.Resize((Width + 1) * (Height + 1));
    PlaneInfo.Indices.Resize((Width * Height) * 6);

    // Size of each quad, size of the plane will always be between -0.5 and 0.5
    FVector2 QuadSize   = FVector2(1.0f / float(Width), 1.0f / float(Height));
    FVector2 UvQuadSize = FVector2(1.0f / float(Width), 1.0f / float(Height));

    for (uint32 x = 0; x <= Width; x++)
    {
        for (uint32 y = 0; y <= Height; y++)
        {
            int32 v = ((1 + Height) * x) + y;
            PlaneInfo.Vertices[v].Position = FVector3(0.5f - (QuadSize.x * x), 0.5f - (QuadSize.y * y), 0.0f);
            // TODO: Fix vertices so normal is positive
            PlaneInfo.Vertices[v].Normal   = FVector3(0.0f, 0.0f, -1.0f);
            PlaneInfo.Vertices[v].Tangent  = FVector3(1.0f, 0.0f, 0.0f);
            PlaneInfo.Vertices[v].TexCoord = FVector2(0.0f + (UvQuadSize.x * x), 0.0f + (UvQuadSize.y * y));
        }
    }

    for (uint8 x = 0; x < Width; x++)
    {
        for (uint8 y = 0; y < Height; y++)
        {
            int32 quad = (Height * x) + y;
            PlaneInfo.Indices[(quad * 6) + 0] = (x * (1 + Height)) + y + 1;
            PlaneInfo.Indices[(quad * 6) + 1] = (PlaneInfo.Indices[quad * 6] + 2 + (Height - 1));
            PlaneInfo.Indices[(quad * 6) + 2] = PlaneInfo.Indices[(quad * 6) + 0] - 1;
            PlaneInfo.Indices[(quad * 6) + 3] = PlaneInfo.Indices[(quad * 6) + 1];
            PlaneInfo.Indices[(quad * 6) + 4] = PlaneInfo.Indices[(quad * 6) + 1] - 1;
            PlaneInfo.Indices[(quad * 6) + 5] = PlaneInfo.Indices[(quad * 6) + 2];
        }
    }

    PlaneInfo.Vertices.Shrink();
    PlaneInfo.Indices.Shrink();

    return PlaneInfo;
}

FMeshCreateInfo FMeshFactory::CreateSphere(uint32 Subdivisions, float Radius) noexcept
{
    FMeshCreateInfo SphereInfo;
    SphereInfo.Vertices.Resize(12);

    const float t = (1.0f + FMath::Sqrt(5.0f)) / 2.0f;
    SphereInfo.Vertices[0].Position  = FVector3(-1.0f,  t   ,  0.0f);
    SphereInfo.Vertices[1].Position  = FVector3( 1.0f,  t   ,  0.0f);
    SphereInfo.Vertices[2].Position  = FVector3(-1.0f, -t   ,  0.0f);
    SphereInfo.Vertices[3].Position  = FVector3( 1.0f, -t   ,  0.0f);
    SphereInfo.Vertices[4].Position  = FVector3( 0.0f, -1.0f,  t);
    SphereInfo.Vertices[5].Position  = FVector3( 0.0f,  1.0f,  t);
    SphereInfo.Vertices[6].Position  = FVector3( 0.0f, -1.0f, -t);
    SphereInfo.Vertices[7].Position  = FVector3( 0.0f,  1.0f, -t);
    SphereInfo.Vertices[8].Position  = FVector3( t   ,  0.0f, -1.0f);
    SphereInfo.Vertices[9].Position  = FVector3( t   ,  0.0f,  1.0f);
    SphereInfo.Vertices[10].Position = FVector3(-t   ,  0.0f, -1.0f);
    SphereInfo.Vertices[11].Position = FVector3(-t   ,  0.0f,  1.0f);

    SphereInfo.Indices =
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
        SphereInfo.Subdivide(Subdivisions);
    }

    for (uint32 i = 0; i < static_cast<uint32>(SphereInfo.Vertices.Size()); i++)
    {
        // Calculate the new position, normal and tangent
        FVector3 Position = SphereInfo.Vertices[i].Position;
        Position.Normalize();

        SphereInfo.Vertices[i].Normal   = Position;
        SphereInfo.Vertices[i].Position = Position * Radius;

        // Calculate UVs
        SphereInfo.Vertices[i].TexCoord.y = (FMath::Asin(SphereInfo.Vertices[i].Position.y) / FMath::kPI_f) + 0.5f;
        SphereInfo.Vertices[i].TexCoord.x = (FMath::Atan2(SphereInfo.Vertices[i].Position.z, SphereInfo.Vertices[i].Position.x) + FMath::kPI_f) / (2.0f * FMath::kPI_f);
    }

    SphereInfo.Indices.Shrink();
    SphereInfo.Vertices.Shrink();

    SphereInfo.CalculateTangents();
    return SphereInfo;
}

FMeshCreateInfo FMeshFactory::CreateCone(uint32 Sides, float Radius, float Height) noexcept
{
    if (Sides < 3)
    {
        // A cone must have at least 3 sides
        return FMeshCreateInfo();
    }

    FMeshCreateInfo MeshCreateInfo;

    // Number of vertices: (Sides + 1) for the base (including center) + Sides for the side vertices
    const uint32 NumVertices = Sides + 1 + Sides + 1;
    MeshCreateInfo.Vertices.Resize(NumVertices);

    // Number of indices: (Sides * 3) for the base cap + (Sides * 3) for the sides
    const uint32 NumIndices = Sides * 3 + Sides * 3;
    MeshCreateInfo.Indices.Resize(NumIndices);

    // Angle between each side segment
    const float Angle = (2.0f * FMath::kPI) / static_cast<float>(Sides);
    
    // Create the center vertex for the base cap
    MeshCreateInfo.Vertices[0].Position = FVector3(0.0f, 0.0f, 0.0f);
    MeshCreateInfo.Vertices[0].Normal   = FVector3(0.0f, -1.0f, 0.0f);
    MeshCreateInfo.Vertices[0].TexCoord = FVector2(0.5f, 0.5f); // Center UV coordinates

    // Create vertices for the base cap and sides
    const uint32 Offset = Sides + 1;
    for (uint32 i = 0; i < Sides; ++i)
    {
        // Calculate the position of the current vertex on the base circle
        const float x = Radius * FMath::Cos<float>(Angle * i);
        const float z = Radius * FMath::Sin<float>(Angle * i);
        const FVector3 BasePosition(x, 0.0f, z);

        // Base vertex
        MeshCreateInfo.Vertices[i + 1].Position = BasePosition;
        MeshCreateInfo.Vertices[i + 1].Normal   = FVector3(0.0f, -1.0f, 0.0f); // Pointing downwards
        MeshCreateInfo.Vertices[i + 1].TexCoord = FVector2((x / Radius + 1.0f) * 0.5f, (z / Radius + 1.0f) * 0.5f);

        // Side vertex
        MeshCreateInfo.Vertices[Offset + i].Position = BasePosition;
        
        FVector3 Normal(x, Radius / Height, z);
        if (Normal.LengthSquared() > 0.0f) // Ensure normalization is safe
        {
            Normal.Normalize();
        }
        
        MeshCreateInfo.Vertices[Offset + i].Normal   = Normal;
        MeshCreateInfo.Vertices[Offset + i].TexCoord = FVector2(static_cast<float>(i) / static_cast<float>(Sides), 1.0f);
    }

    // Apex vertex
    MeshCreateInfo.Vertices[NumVertices - 1].Position = FVector3(0.0f, Height, 0.0f);
    MeshCreateInfo.Vertices[NumVertices - 1].Normal   = FVector3(0.0f, 1.0f, 0.0f); // Pointing upwards
    MeshCreateInfo.Vertices[NumVertices - 1].TexCoord = FVector2(0.5f, 0.0f);

    // Create indices for the base cap
    uint32 Index = 0;
    for (uint32 i = 0; i < Sides; ++i)
    {
        MeshCreateInfo.Indices[Index++] = 0;
        MeshCreateInfo.Indices[Index++] = i + 1;
        MeshCreateInfo.Indices[Index++] = ((i + 1) % Sides) + 1;
    }

    // Create indices for the sides
    for (uint32 i = 0; i < Sides; ++i)
    {
        MeshCreateInfo.Indices[Index++] = Offset + i;
        MeshCreateInfo.Indices[Index++] = NumVertices - 1;
        MeshCreateInfo.Indices[Index++] = Offset + ((i + 1) % Sides);
    }

    // Calculate tangents for proper lighting and normal mapping
    MeshCreateInfo.CalculateTangents();
    return MeshCreateInfo;
}

FMeshCreateInfo FMeshFactory::CreateTorus(float RingRadius, float TubeRadius, uint32 RingSegments, uint32 TubeSegments) noexcept
{
    if (RingSegments < 3 || TubeSegments < 3)
    {
        // A torus must have at least 3 segments for both the ring and the tube
        return FMeshCreateInfo();
    }

    // Number of vertices and indices
    const uint32 NumVertices = RingSegments * TubeSegments;
    const uint32 NumIndices  = RingSegments * TubeSegments * 6;

    FMeshCreateInfo MeshCreateInfo;
    MeshCreateInfo.Vertices.Resize(NumVertices);
    MeshCreateInfo.Indices.Resize(NumIndices);

    // Step angles for each segment
    const float RingStep = 2.0f * FMath::kPI_f / static_cast<float>(RingSegments);
    const float TubeStep = 2.0f * FMath::kPI_f / static_cast<float>(TubeSegments);

    // Create vertices
    uint32 VertexIndex = 0;
    for (uint32 i = 0; i < RingSegments; ++i)
    {
        const float RingAngle = i * RingStep;
        const FVector3 RingCenter = FVector3(RingRadius * FMath::Cos<float>(RingAngle), 0.0f, RingRadius * FMath::Sin<float>(RingAngle));
        for (uint32 j = 0; j < TubeSegments; ++j)
        {
            const float TubeAngle = j * TubeStep;
            const float CosTube   = FMath::Cos<float>(TubeAngle);
            const float SinTube   = FMath::Sin<float>(TubeAngle);

            // Position of the vertex
            FVector3 Position = RingCenter + FVector3(TubeRadius * CosTube * FMath::Cos<float>(RingAngle), TubeRadius * SinTube, TubeRadius * CosTube * FMath::Sin<float>(RingAngle));
            MeshCreateInfo.Vertices[VertexIndex].Position = Position;

            // Normal vector
            FVector3 Normal = FVector3(CosTube * FMath::Cos<float>(RingAngle), SinTube, CosTube * FMath::Sin<float>(RingAngle));
            Normal.Normalize();
            
            MeshCreateInfo.Vertices[VertexIndex].Normal = Normal;

            // Texture coordinates
            const float u = static_cast<float>(i) / static_cast<float>(RingSegments);
            const float v = static_cast<float>(j) / static_cast<float>(TubeSegments);
            MeshCreateInfo.Vertices[VertexIndex].TexCoord = FVector2(u, v);

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
            MeshCreateInfo.Indices[Index++] = Current;
            MeshCreateInfo.Indices[Index++] = NextTube;
            MeshCreateInfo.Indices[Index++] = NextRing;

            // Second triangle
            MeshCreateInfo.Indices[Index++] = NextTube;
            MeshCreateInfo.Indices[Index++] = Diagonal;
            MeshCreateInfo.Indices[Index++] = NextRing;
        }
    }

    // Calculate tangents for proper lighting and normal mapping
    MeshCreateInfo.CalculateTangents();
    return MeshCreateInfo;
}

FMeshCreateInfo FMeshFactory::CreateTeapot(uint32 Tessellation) noexcept
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
    const auto EvaluateBezierPatch = [&](const float ControlPoints[16][3], float u, float v, FVertex& Vertex)
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

        FVector3 Normal(
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

    FMeshCreateInfo MeshInfo;
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

        const uint32 StartVertex = MeshInfo.Vertices.Size();
        for (uint32 u = 0; u <= Tessellation; ++u)
        {
            const float t = static_cast<float>(u) / static_cast<float>(Tessellation);
            for (uint32 v = 0; v <= Tessellation; ++v)
            {
                const float s = static_cast<float>(v) / static_cast<float>(Tessellation);

                FVertex Vertex;
                EvaluateBezierPatch(PatchControlPoints, t, s, Vertex);
                MeshInfo.Vertices.Add(Vertex);
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

                MeshInfo.Indices.Add(Index0);
                MeshInfo.Indices.Add(Index2);
                MeshInfo.Indices.Add(Index1);

                MeshInfo.Indices.Add(Index1);
                MeshInfo.Indices.Add(Index2);
                MeshInfo.Indices.Add(Index3);
            }
        }
    }

    return MeshInfo;
}

FMeshCreateInfo FMeshFactory::CreatePyramid(float Width, float Depth, float Height) noexcept
{
    FMeshCreateInfo MeshInfo;

    float HalfWidth = Width / 2.0f;
    float HalfDepth = Depth / 2.0f;

    // Bottom vertices
    FVertex v0;
    v0.Position = FVector3(-HalfWidth, 0.0f, -HalfDepth); // Front-left
    FVertex v1;
    v1.Position = FVector3( HalfWidth, 0.0f, -HalfDepth); // Front-right
    FVertex v2;
    v2.Position = FVector3( HalfWidth, 0.0f,  HalfDepth); // Back-right
    FVertex v3;
    v3.Position = FVector3(-HalfWidth, 0.0f,  HalfDepth); // Back-left

    // Apex vertex
    FVertex v4;
    v4.Position = FVector3(0.0f, Height, 0.0f); // Top center

    // Base normal
    FVector3 BaseNormal = FVector3(0.0f, -1.0f, 0.0f);

    // Side normals (calculated for each face)
    FVector3 Normal0 = FVector3::CrossProduct(v4.Position - v0.Position, v1.Position - v0.Position).Normalize();
    FVector3 Normal1 = FVector3::CrossProduct(v4.Position - v1.Position, v2.Position - v1.Position).Normalize();
    FVector3 Normal2 = FVector3::CrossProduct(v4.Position - v2.Position, v3.Position - v2.Position).Normalize();
    FVector3 Normal3 = FVector3::CrossProduct(v4.Position - v3.Position, v0.Position - v3.Position).Normalize();

    // Assign normals to the base vertices
    v0.Normal = BaseNormal;
    v1.Normal = BaseNormal;
    v2.Normal = BaseNormal;
    v3.Normal = BaseNormal;

    // Assign texture coordinates for the base
    v0.TexCoord = FVector2(0.0f, 0.0f);
    v1.TexCoord = FVector2(1.0f, 0.0f);
    v2.TexCoord = FVector2(1.0f, 1.0f);
    v3.TexCoord = FVector2(0.0f, 1.0f);

    // Add base vertices to the mesh
    uint32 BaseIndex = static_cast<uint32>(MeshInfo.Vertices.Size());
    MeshInfo.Vertices.Add(v0); // Index 0
    MeshInfo.Vertices.Add(v1); // Index 1
    MeshInfo.Vertices.Add(v2); // Index 2
    MeshInfo.Vertices.Add(v3); // Index 3

    // Add indices for the base (two triangles)
    MeshInfo.Indices.Add(BaseIndex + 0);
    MeshInfo.Indices.Add(BaseIndex + 1);
    MeshInfo.Indices.Add(BaseIndex + 2);

    MeshInfo.Indices.Add(BaseIndex + 0);
    MeshInfo.Indices.Add(BaseIndex + 2);
    MeshInfo.Indices.Add(BaseIndex + 3);

    // Side 1 (v0, v1, v4)
    FVertex s0v0 = v0;
    FVertex s0v1 = v1;
    FVertex s0v4 = v4;
    s0v0.Normal   = Normal0;
    s0v1.Normal   = Normal0;
    s0v4.Normal   = Normal0;
    s0v0.TexCoord = FVector2(0.0f, 0.0f);
    s0v1.TexCoord = FVector2(1.0f, 0.0f);
    s0v4.TexCoord = FVector2(0.5f, 1.0f);

    const uint32 Side0Index = static_cast<uint32>(MeshInfo.Vertices.Size());
    MeshInfo.Vertices.Add(s0v0);
    MeshInfo.Vertices.Add(s0v1);
    MeshInfo.Vertices.Add(s0v4);

    MeshInfo.Indices.Add(Side0Index + 0);
    MeshInfo.Indices.Add(Side0Index + 2);
    MeshInfo.Indices.Add(Side0Index + 1);

    // Side 2 (v1, v2, v4)
    FVertex s1v0 = v1;
    FVertex s1v1 = v2;
    FVertex s1v4 = v4;
    s1v0.Normal   = Normal1;
    s1v1.Normal   = Normal1;
    s1v4.Normal   = Normal1;
    s1v0.TexCoord = FVector2(0.0f, 0.0f);
    s1v1.TexCoord = FVector2(1.0f, 0.0f);
    s1v4.TexCoord = FVector2(0.5f, 1.0f);

    const uint32 Side1Index = static_cast<uint32>(MeshInfo.Vertices.Size());
    MeshInfo.Vertices.Add(s1v0);
    MeshInfo.Vertices.Add(s1v1);
    MeshInfo.Vertices.Add(s1v4);

    MeshInfo.Indices.Add(Side1Index + 0);
    MeshInfo.Indices.Add(Side1Index + 2);
    MeshInfo.Indices.Add(Side1Index + 1);

    // Side 3 (v2, v3, v4)
    FVertex s2v0 = v2;
    FVertex s2v1 = v3;
    FVertex s2v4 = v4;
    s2v0.Normal   = Normal2;
    s2v1.Normal   = Normal2;
    s2v4.Normal   = Normal2;
    s2v0.TexCoord = FVector2(0.0f, 0.0f);
    s2v1.TexCoord = FVector2(1.0f, 0.0f);
    s2v4.TexCoord = FVector2(0.5f, 1.0f);

    const uint32 Side2Index = static_cast<uint32>(MeshInfo.Vertices.Size());
    MeshInfo.Vertices.Add(s2v0);
    MeshInfo.Vertices.Add(s2v1);
    MeshInfo.Vertices.Add(s2v4);

    MeshInfo.Indices.Add(Side2Index + 0);
    MeshInfo.Indices.Add(Side2Index + 2);
    MeshInfo.Indices.Add(Side2Index + 1);

    // Side 4 (v3, v0, v4)
    FVertex s3v0 = v3;
    FVertex s3v1 = v0;
    FVertex s3v4 = v4;
    s3v0.Normal   = Normal3;
    s3v1.Normal   = Normal3;
    s3v4.Normal   = Normal3;
    s3v0.TexCoord = FVector2(0.0f, 0.0f);
    s3v1.TexCoord = FVector2(1.0f, 0.0f);
    s3v4.TexCoord = FVector2(0.5f, 1.0f);

    const uint32 Side3Index = static_cast<uint32>(MeshInfo.Vertices.Size());
    MeshInfo.Vertices.Add(s3v0);
    MeshInfo.Vertices.Add(s3v1);
    MeshInfo.Vertices.Add(s3v4);

    MeshInfo.Indices.Add(Side3Index + 0);
    MeshInfo.Indices.Add(Side3Index + 2);
    MeshInfo.Indices.Add(Side3Index + 1);
    return MeshInfo;
}

// TODO: Finish
FMeshCreateInfo FMeshFactory::CreateCylinder(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    FMeshData data;
    if (sides < 5)
        sides = 5;
    if (Height < 0.1f)
        Height = 0.1f;
    if (radius < 0.1f)
        radius = 0.1f;

    // Num verts = (Sides*2)    (Top, since we need unique normals)
    //          + (Sides*2)    (Bottom)
    //            + 2            (MiddlePoints)
    size_t vertSize = size_t(sides) * 4 + 2;
    data.Vertices.resize(vertSize);

    // Num indices = (Sides*3*2) (Each cap has 'sides' number of tris, each tri has 3 verts)
    //              + (Sides*6)    (Each side has 6 verts)
    size_t indexSize = size_t(sides) * 12;
    data.Indices.resize(indexSize);

    // Angle between verts
    float angle = (pi<float>() * 2.0f) / float(sides);
    float uOffset = 1.0f / float(sides - 1);
    float halfHeight = Height * 0.5f;

    // CREATE VERTICES
    data.Vertices[0].Position = XMFLOAT3(0.0f, halfHeight, 0.0f);
    data.Vertices[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    data.Vertices[0].TexCoord = XMFLOAT2(0.25f, 0.25f);

    size_t offset = size_t(sides) + 1;
    data.Vertices[offset].Position = XMFLOAT3(0.0f, -halfHeight, 0.0f);
    data.Vertices[offset].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
    data.Vertices[offset].TexCoord = XMFLOAT2(0.75f, 0.25f);

    size_t doubleOffset = offset * 2;
    size_t trippleOffset = doubleOffset + size_t(sides);
    for (size_t i = 0; i < sides; i++)
    {
        // TOP CAP VERTICES
        float x = FMath::Cos((pi<float>() / 2.0f) + (angle * i));
        float z = FMath::Sin((pi<float>() / 2.0f) + (angle * i));
        XMFLOAT3 pos = normalize(XMFLOAT3(x, 0.0f, z));
        data.Vertices[i + 1].Position = (pos * radius) + XMFLOAT3(0.0f, halfHeight, 0.0f);
        data.Vertices[i + 1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        data.Vertices[i + 1].TexCoord = XMFLOAT2(x + 1.0f, z + 1.0f) * 0.25f;

        // BOTTOM CAP VERTICES
        data.Vertices[offset + i + 1].Position = data.Vertices[i + 1].Position - XMFLOAT3(0.0f, Height, 0.0f);
        data.Vertices[offset + i + 1].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
        data.Vertices[offset + i + 1].TexCoord = data.Vertices[i + 1].TexCoord + XMFLOAT2(0.5f, 0.5f);

        // TOP SIDE VERTICES
        data.Vertices[doubleOffset + i].Position = data.Vertices[i + 1].Position;
        data.Vertices[doubleOffset + i].Normal = pos;
        data.Vertices[doubleOffset + i].TexCoord = XMFLOAT2(0.0f + (uOffset * i), 1.0f);

        // BOTTOM SIDE VERTICES
        data.Vertices[trippleOffset + i].Position = data.Vertices[offset + i + 1].Position;
        data.Vertices[trippleOffset + i].Normal = pos;
        data.Vertices[trippleOffset + i].TexCoord = XMFLOAT2(0.0f + (uOffset * i), 0.25f);
    }

    // TOP CAP INDICES
    size_t index = 0;
    for (uint32 i = 0; i < sides; i++)
    {
        data.Indices[index + 0] = i + 1;
        data.Indices[index + 1] = (i + 1) % (sides)+1;
        data.Indices[index + 2] = 0;
        index += 3;
    }

    // BOTTOM CAP INDICES
    for (uint32 i = 0; i < sides; i++)
    {
        uint32 base = uint32(sides) + 1;
        data.Indices[index + 0] = base + ((i + 1) % (sides)+1);
        data.Indices[index + 1] = base + i + 1;
        data.Indices[index + 2] = base;
        index += 3;
    }

    // SIDES
    for (uint32 i = 0; i < sides; i++)
    {
        uint32 base = (uint32(sides) + 1) * 2;
        data.Indices[index + 0] = base + i + 1;
        data.Indices[index + 1] = base + i;
        data.Indices[index + 2] = base + i + sides;
        data.Indices[index + 3] = base + ((i + 1) % sides);
        data.Indices[index + 4] = (base + sides - 1) + ((i + 1) % sides);
        data.Indices[index + 5] = (base + sides) + ((i + 1) % sides);
        index += 6;
    }

    CalculateTangents(data);
    return data;
    */

    return FMeshCreateInfo();
}
