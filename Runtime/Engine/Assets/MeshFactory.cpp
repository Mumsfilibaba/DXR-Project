#include "MeshFactory.h"
#include "MeshUtilities.h"

#include "Core/Math/MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMeshFactory

SMeshData CMeshFactory::CreateCube(float Width, float Height, float Depth) noexcept
{
    const float HalfWidth = Width * 0.5f;
    const float HalfHeight = Height * 0.5f;
    const float HalfDepth = Depth * 0.5f;

    SMeshData Cube;
    Cube.Vertices =
    {
        // FRONT FACE
        { CVector3(-HalfWidth,  HalfHeight, -HalfDepth), CVector3(0.0f,  0.0f, -1.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 0.0f) },
        { CVector3(HalfWidth,  HalfHeight, -HalfDepth), CVector3(0.0f,  0.0f, -1.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 0.0f) },
        { CVector3(-HalfWidth, -HalfHeight, -HalfDepth), CVector3(0.0f,  0.0f, -1.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 1.0f) },
        { CVector3(HalfWidth, -HalfHeight, -HalfDepth), CVector3(0.0f,  0.0f, -1.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 1.0f) },

        // BACK FACE
        { CVector3(HalfWidth,  HalfHeight,  HalfDepth), CVector3(0.0f,  0.0f,  1.0f), CVector3(-1.0f,  0.0f, 0.0f), CVector2(0.0f, 0.0f) },
        { CVector3(-HalfWidth,  HalfHeight,  HalfDepth), CVector3(0.0f,  0.0f,  1.0f), CVector3(-1.0f,  0.0f, 0.0f), CVector2(1.0f, 0.0f) },
        { CVector3(HalfWidth, -HalfHeight,  HalfDepth), CVector3(0.0f,  0.0f,  1.0f), CVector3(-1.0f,  0.0f, 0.0f), CVector2(0.0f, 1.0f) },
        { CVector3(-HalfWidth, -HalfHeight,  HalfDepth), CVector3(0.0f,  0.0f,  1.0f), CVector3(-1.0f,  0.0f, 0.0f), CVector2(1.0f, 1.0f) },

        // RIGHT FACE
        { CVector3(HalfWidth,  HalfHeight, -HalfDepth), CVector3(1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(0.0f, 0.0f) },
        { CVector3(HalfWidth,  HalfHeight,  HalfDepth), CVector3(1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(1.0f, 0.0f) },
        { CVector3(HalfWidth, -HalfHeight, -HalfDepth), CVector3(1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(0.0f, 1.0f) },
        { CVector3(HalfWidth, -HalfHeight,  HalfDepth), CVector3(1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(1.0f, 1.0f) },

        // LEFT FACE
        { CVector3(-HalfWidth,  HalfHeight, -HalfDepth), CVector3(-1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(0.0f, 1.0f) },
        { CVector3(-HalfWidth,  HalfHeight,  HalfDepth), CVector3(-1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(1.0f, 1.0f) },
        { CVector3(-HalfWidth, -HalfHeight, -HalfDepth), CVector3(-1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(0.0f, 0.0f) },
        { CVector3(-HalfWidth, -HalfHeight,  HalfDepth), CVector3(-1.0f,  0.0f,  0.0f), CVector3(0.0f,  0.0f, 1.0f), CVector2(1.0f, 0.0f) },

        // TOP FACE
        { CVector3(-HalfWidth,  HalfHeight,  HalfDepth), CVector3(0.0f,  1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 0.0f) },
        { CVector3(HalfWidth,  HalfHeight,  HalfDepth), CVector3(0.0f,  1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 0.0f) },
        { CVector3(-HalfWidth,  HalfHeight, -HalfDepth), CVector3(0.0f,  1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 1.0f) },
        { CVector3(HalfWidth,  HalfHeight, -HalfDepth), CVector3(0.0f,  1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 1.0f) },

        // BOTTOM FACE
        { CVector3(-HalfWidth, -HalfHeight, -HalfDepth), CVector3(0.0f, -1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 0.0f) },
        { CVector3(HalfWidth, -HalfHeight, -HalfDepth), CVector3(0.0f, -1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 0.0f) },
        { CVector3(-HalfWidth, -HalfHeight,  HalfDepth), CVector3(0.0f, -1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(0.0f, 1.0f) },
        { CVector3(HalfWidth, -HalfHeight,  HalfDepth), CVector3(0.0f, -1.0f,  0.0f), CVector3(1.0f,  0.0f, 0.0f), CVector2(1.0f, 1.0f) },
    };

    Cube.Indices =
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

    return Cube;
}

SMeshData CMeshFactory::CreatePlane(uint32 Width, uint32 Height) noexcept
{
    SMeshData Data;
    if (Width < 1)
    {
        Width = 1;
    }
    if (Height < 1)
    {
        Height = 1;
    }

    Data.Vertices.Resize((Width + 1) * (Height + 1));
    Data.Indices.Resize((Width * Height) * 6);

    // Size of each quad, size of the plane will always be between -0.5 and 0.5
    CVector2 QuadSize = CVector2(1.0f / float(Width), 1.0f / float(Height));
    CVector2 UvQuadSize = CVector2(1.0f / float(Width), 1.0f / float(Height));

    for (uint32 x = 0; x <= Width; x++)
    {
        for (uint32 y = 0; y <= Height; y++)
        {
            int32 v = ((1 + Height) * x) + y;
            Data.Vertices[v].Position = CVector3(0.5f - (QuadSize.x * x), 0.5f - (QuadSize.y * y), 0.0f);
            // TODO: Fix vertices so normal is positive
            Data.Vertices[v].Normal = CVector3(0.0f, 0.0f, -1.0f);
            Data.Vertices[v].Tangent = CVector3(1.0f, 0.0f, 0.0f);
            Data.Vertices[v].TexCoord = CVector2(0.0f + (UvQuadSize.x * x), 0.0f + (UvQuadSize.y * y));
        }
    }

    for (uint8 x = 0; x < Width; x++)
    {
        for (uint8 y = 0; y < Height; y++)
        {
            int32 quad = (Height * x) + y;
            Data.Indices[(quad * 6) + 0] = (x * (1 + Height)) + y + 1;
            Data.Indices[(quad * 6) + 1] = (Data.Indices[quad * 6] + 2 + (Height - 1));
            Data.Indices[(quad * 6) + 2] = Data.Indices[quad * 6] - 1;
            Data.Indices[(quad * 6) + 3] = Data.Indices[(quad * 6) + 1];
            Data.Indices[(quad * 6) + 4] = Data.Indices[(quad * 6) + 1] - 1;
            Data.Indices[(quad * 6) + 5] = Data.Indices[(quad * 6) + 2];
        }
    }

    Data.Vertices.ShrinkToFit();
    Data.Indices.ShrinkToFit();

    return Data;
}

SMeshData CMeshFactory::CreateSphere(uint32 Subdivisions, float Radius) noexcept
{
    SMeshData Sphere;
    Sphere.Vertices.Resize(12);

    const float t = (1.0f + NMath::Sqrt(5.0f)) / 2.0f;
    Sphere.Vertices[0].Position = CVector3(-1.0f, t, 0.0f);
    Sphere.Vertices[1].Position = CVector3(1.0f, t, 0.0f);
    Sphere.Vertices[2].Position = CVector3(-1.0f, -t, 0.0f);
    Sphere.Vertices[3].Position = CVector3(1.0f, -t, 0.0f);
    Sphere.Vertices[4].Position = CVector3(0.0f, -1.0f, t);
    Sphere.Vertices[5].Position = CVector3(0.0f, 1.0f, t);
    Sphere.Vertices[6].Position = CVector3(0.0f, -1.0f, -t);
    Sphere.Vertices[7].Position = CVector3(0.0f, 1.0f, -t);
    Sphere.Vertices[8].Position = CVector3(t, 0.0f, -1.0f);
    Sphere.Vertices[9].Position = CVector3(t, 0.0f, 1.0f);
    Sphere.Vertices[10].Position = CVector3(-t, 0.0f, -1.0f);
    Sphere.Vertices[11].Position = CVector3(-t, 0.0f, 1.0f);

    Sphere.Indices =
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
        CMeshUtilities::Subdivide(Sphere, Subdivisions);
    }

    for (uint32 i = 0; i < static_cast<uint32>(Sphere.Vertices.Size()); i++)
    {
        // Calculate the new position, normal and tangent
        CVector3 Position = Sphere.Vertices[i].Position;
        Position.Normalize();

        Sphere.Vertices[i].Normal = Position;
        Sphere.Vertices[i].Position = Position * Radius;

        // Calculate uvs
        Sphere.Vertices[i].TexCoord.y = (NMath::Asin(Sphere.Vertices[i].Position.y) / NMath::kPI_f) + 0.5f;
        Sphere.Vertices[i].TexCoord.x = (NMath::Atan2(Sphere.Vertices[i].Position.z, Sphere.Vertices[i].Position.x) + NMath::kPI_f) / (2.0f * NMath::kPI_f);
    }

    Sphere.Indices.ShrinkToFit();
    Sphere.Vertices.ShrinkToFit();

    CMeshUtilities::CalculateTangents(Sphere);

    return Sphere;
}

// TODO: Finish
SMeshData CMeshFactory::CreateCone(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    SMeshData data;
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
        float x = NMath::Cos((pi<float>() / 2.0f) + (angle * i));
        float z = NMath::Sin((pi<float>() / 2.0f) + (angle * i));

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
    return SMeshData();
}

// TODO: Finish
SMeshData CMeshFactory::CreatePyramid() noexcept
{
    /*
    SMeshData data;
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
    return SMeshData();
}

// TODO: Finish
SMeshData CMeshFactory::CreateCylinder(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    SMeshData data;
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
        float x = NMath::Cos((pi<float>() / 2.0f) + (angle * i));
        float z = NMath::Sin((pi<float>() / 2.0f) + (angle * i));
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
    return SMeshData();
}
