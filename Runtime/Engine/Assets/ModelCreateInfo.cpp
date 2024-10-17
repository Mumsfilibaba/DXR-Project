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

// TODO: Finish
FMeshCreateInfo FMeshFactory::CreateCone(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    FMeshData data;
    // Num verts = (Sides*2)    (Bottom, since we need unique normals)
    //            +  Sides    (1 MiddlePoint per side)
    //            +  1        (One middlepoint on the underside)
    size_t vertSize = size_t(sides) * 3 + 1;
    data.Vertices.resize(vertSize);

    // Num indices = (Sides*3*2) (Cap has 'sides' number of tris + sides tris for the sides, each tri has 3 verts)
    size_t indexSize = size_t(sides) * 6;
    data.Indices.resize(indexSize);

    // Angle between verts
    float angle = (pi<float>() * 2.0f) / float(sides);
    float uOffset = 1.0f / float(sides - 1);

    // CREATE VERTICES
    data.Vertices[0].Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    data.Vertices[0].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
    data.Vertices[0].TexCoord = XMFLOAT2(0.25f, 0.25f);

    size_t offset = size_t(sides) + 1;
    size_t topOffset = offset + size_t(sides);
    for (size_t i = 0; i < sides; i++)
    {
        // BOTTOM CAP VERTICES
        float x = FMath::Cos((pi<float>() / 2.0f) + (angle * i));
        float z = FMath::Sin((pi<float>() / 2.0f) + (angle * i));

        XMFLOAT3 pos = normalize(XMFLOAT3(x, 0.0f, z));
        data.Vertices[i + 1].Position = (pos * radius);
        data.Vertices[i + 1].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
        data.Vertices[i + 1].TexCoord = (XMFLOAT2(x + 1.0f, z + 1.0f) * 0.25f);

        // BOTTOM SIDE VERTICES
        XMFLOAT3 normal = normalize(pos + XMFLOAT3(0.0f, sin(atan(Height / radius)), 0.0f));
        data.Vertices[offset + i].Position = data.Vertices[i + 1].Position;
        data.Vertices[offset + i].Normal = normal;
        data.Vertices[offset + i].TexCoord = XMFLOAT2(0.0f + (uOffset * i), 1.0f);

        // TOP
        data.Vertices[topOffset + i].Position = XMFLOAT3(0.0f, Height, 0.0f);
        data.Vertices[topOffset + i].Normal = normal;
        data.Vertices[topOffset + i].TexCoord = XMFLOAT2(0.0f + (uOffset * i), 0.25f);
    }

    // BOTTOM CAP INDICES
    size_t index = 0;
    for (uint32 i = 0; i < sides; i++)
    {
        data.Indices[index + 0] = ((i + 1) % sides) + 10;
        data.Indices[index + 1] = i + 1;
        data.Indices[index + 2] = 0;
        index += 3;
    }

    // SIDES INDICES
    for (uint32 i = 0; i < sides; i++)
    {
        data.Indices[index + 0] = uint32(offset) + i;
        data.Indices[index + 1] = uint32(offset) + ((i + 1) % sides);
        data.Indices[index + 2] = uint32(topOffset) + i;
        index += 3;
    }

    //Get tangents
    CalculateTangents(data);

    return data;
    */

    return FMeshCreateInfo();
}

// TODO: Finish
FMeshCreateInfo FMeshFactory::CreatePyramid() noexcept
{
    /*
    FMeshData data;
    data.Vertices.resize(16);
    data.Indices.resize(18);

    // FLOOR FACE (Seen from FRONT FACE)
    data.Vertices[0].TexCoord = XMFLOAT2(0.33f, 0.33f);
    data.Vertices[0].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    data.Vertices[1].TexCoord = XMFLOAT2(0.66f, 0.33f);
    data.Vertices[1].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);
    data.Vertices[2].TexCoord = XMFLOAT2(0.33f, 0.66f);
    data.Vertices[2].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);
    data.Vertices[3].TexCoord = XMFLOAT2(0.66f, 0.66f);
    data.Vertices[3].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);

    // TOP VERTICES
    data.Vertices[4].Position =
        data.Vertices[5].Position =
        data.Vertices[6].Position =
        data.Vertices[7].Position = XMFLOAT3(0.0f, 0.5f, 0.0f);
    data.Vertices[4].TexCoord = XMFLOAT2(0.495f, 0.0f);
    data.Vertices[5].TexCoord = XMFLOAT2(0.0f, 0.495f);
    data.Vertices[6].TexCoord = XMFLOAT2(0.495f, 0.99f);
    data.Vertices[7].TexCoord = XMFLOAT2(0.99f, 0.495f);

    // BACK
    data.Vertices[8].TexCoord = XMFLOAT2(0.33f, 0.33f);
    data.Vertices[8].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    data.Vertices[9].TexCoord = XMFLOAT2(0.66f, 0.33f);
    data.Vertices[9].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);

    // FRONT
    data.Vertices[10].TexCoord = XMFLOAT2(0.33f, 0.66f);
    data.Vertices[10].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);
    data.Vertices[11].TexCoord = XMFLOAT2(0.66f, 0.66f);
    data.Vertices[11].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);

    // LEFT
    data.Vertices[12].TexCoord = XMFLOAT2(0.33f, 0.33f);
    data.Vertices[12].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    data.Vertices[13].TexCoord = XMFLOAT2(0.33f, 0.66f);
    data.Vertices[13].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);

    // RIGHT
    data.Vertices[14].TexCoord = XMFLOAT2(0.66f, 0.33f);
    data.Vertices[14].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);
    data.Vertices[15].TexCoord = XMFLOAT2(0.66f, 0.66f);
    data.Vertices[15].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);

    // FLOOR FACE
    data.Indices[0] = 2;
    data.Indices[1] = 1;
    data.Indices[2] = 0;
    data.Indices[3] = 2;
    data.Indices[4] = 3;
    data.Indices[5] = 1;

    // BACK FACE
    data.Indices[6] = 8;
    data.Indices[7] = 9;
    data.Indices[8] = 4;

    // LEFT FACE
    data.Indices[9] = 13;
    data.Indices[10] = 12;
    data.Indices[11] = 5;

    // FRONT FACE
    data.Indices[12] = 11;
    data.Indices[13] = 10;
    data.Indices[14] = 6;

    // RIGHT FACE
    data.Indices[15] = 14;
    data.Indices[16] = 15;
    data.Indices[17] = 7;

    data.Indices.shrink_to_fit();
    data.Vertices.shrink_to_fit();

    CalculateHardNormals(data);
    CalculateTangents(data);

    return data;
    */

    return FMeshCreateInfo();
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
