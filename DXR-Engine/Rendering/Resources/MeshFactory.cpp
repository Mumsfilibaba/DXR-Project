#include "Rendering/Resources/MeshFactory.h"

#include <tiny_obj_loader.h>

MeshData MeshFactory::CreateFromFile(const std::string& Filename, bool LeftHanded) noexcept
{
    std::string Error;
    std::string Warning;
    std::vector<tinyobj::shape_t> Shapes;
    tinyobj::attrib_t Attributes;

    MeshData Result;
    
    if (!tinyobj::LoadObj(&Attributes, &Shapes, nullptr, &Warning, &Error, Filename.c_str(), nullptr, true, false))
    {
        LOG_WARNING("[MeshFactory]: Failed to load mesh '" + Filename + "'." + " Warning: " + Warning + " Error: " + Error);
        return Result;
    }
    else
    {
        LOG_INFO("[MeshFactory]: Loaded mesh'" + Filename + "'");
    }

    std::unordered_map<Vertex, uint32, VertexHasher> UniqueVertices;
    for (const tinyobj::shape_t& Shape : Shapes)
    {
        for (uint32 i = 0; i < Shape.mesh.indices.size(); i++)
        {
            const tinyobj::index_t& Index = Shape.mesh.indices[i];
            Vertex TempVertex;

            // Normals and texcoords are optional, Positions are required
            Assert(Index.vertex_index >= 0);

            size_t PositionIndex = 3 * static_cast<size_t>(Index.vertex_index);
            TempVertex.Position =
            {
                Attributes.vertices[PositionIndex + 0],
                Attributes.vertices[PositionIndex + 1],
                Attributes.vertices[PositionIndex + 2],
            };

            if (Index.normal_index >= 0)
            {
                size_t NormalIndex = 3 * static_cast<size_t>(Index.normal_index);
                TempVertex.Normal =
                {
                    Attributes.normals[NormalIndex + 0],
                    Attributes.normals[NormalIndex + 1],
                    Attributes.normals[NormalIndex + 2],
                };
            }

            if (Index.texcoord_index >= 0)
            {
                size_t TexCoordIndex = 2 * static_cast<size_t>(Index.texcoord_index);
                TempVertex.TexCoord =
                {
                    Attributes.texcoords[TexCoordIndex + 0],
                    Attributes.texcoords[TexCoordIndex + 1],
                };
            }

            if (UniqueVertices.count(TempVertex) == 0)
            {
                UniqueVertices[TempVertex] = static_cast<uint32>(Result.Vertices.Size());
                Result.Vertices.PushBack(TempVertex);
            }

            Result.Indices.EmplaceBack(UniqueVertices[TempVertex]);
        }
    }

    if (LeftHanded)
    {
        for (Vertex& Vertex : Result.Vertices)
        {
            Vertex.Position.z = -Vertex.Position.z;
            Vertex.Normal.z   = -Vertex.Normal.z;
            Vertex.TexCoord.y = 1.0f - Vertex.TexCoord.y;
        }
    }

    MeshFactory::CalculateTangents(Result);

    return Result;
}

MeshData MeshFactory::CreateCube(float Width, float Height, float Depth) noexcept
{
    const float HalfWidth  = Width * 0.5f;
    const float HalfHeight = Height * 0.5f;
    const float HalfDepth  = Depth * 0.5f;

    MeshData Cube;
    Cube.Vertices =
    {
        // FRONT FACE
        { XMFLOAT3(-HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  0.0f, -1.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // BACK FACE
        { XMFLOAT3( HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  0.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  0.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  0.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  0.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // RIGHT FACE
        { XMFLOAT3(HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

        // LEFT FACE
        { XMFLOAT3(-HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT3(0.0f,  0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

        // TOP FACE
        { XMFLOAT3(-HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( HalfWidth,  HalfHeight,  HalfDepth), XMFLOAT3(0.0f,  1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( HalfWidth,  HalfHeight, -HalfDepth), XMFLOAT3(0.0f,  1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // BOTTOM FACE
        { XMFLOAT3(-HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(0.0f, -1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( HalfWidth, -HalfHeight, -HalfDepth), XMFLOAT3(0.0f, -1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(0.0f, -1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( HalfWidth, -HalfHeight,  HalfDepth), XMFLOAT3(0.0f, -1.0f,  0.0f), XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
    };

    Cube.Indices =
    {
        // FRONT FACE
        0, 1, 2,
        1, 3, 2,

        // BACK FACE
        4, 5, 6,
        5, 7, 6,

        // RIGHT FACE
        8, 9, 10,
        9, 11, 10,

        // LEFT FACE
        14, 13, 12,
        14, 15, 13,

        // TOP FACE
        16, 17, 18,
        17, 19, 18,

        // BOTTOM FACE
        20, 21, 22,
        21, 23, 22
    };

    return Cube;
}

MeshData MeshFactory::CreatePlane(uint32 Width, uint32 Height) noexcept
{
    MeshData data;
    if (Width < 1)
    {
        Width = 1;
    }
    if (Height < 1)
    {
        Height = 1;
    }

    data.Vertices.Resize((Width + 1) * (Height + 1));
    data.Indices.Resize((Width * Height) * 6);

    // Size of each quad, size of the plane will always be between -0.5 and 0.5
    XMFLOAT2 quadSize   = XMFLOAT2(1.0f / float(Width), 1.0f / float(Height));
    XMFLOAT2 uvQuadSize = XMFLOAT2(1.0f / float(Width), 1.0f / float(Height));

    for (uint32 x = 0; x <= Width; x++)
    {
        for (uint32 y = 0; y <= Height; y++)
        {
            int32 v = ((1 + Height) * x) + y;
            data.Vertices[v].Position = XMFLOAT3(0.5f - (quadSize.x * x), 0.5f - (quadSize.y * y), 0.0f);
            // TODO: Fix vertices so normal is positive
            data.Vertices[v].Normal   = XMFLOAT3(0.0f, 0.0f, -1.0f);
            data.Vertices[v].Tangent  = XMFLOAT3(1.0f, 0.0f, 0.0f);
            data.Vertices[v].TexCoord = XMFLOAT2(0.0f + (uvQuadSize.x * x), 0.0f + (uvQuadSize.y * y));
        }
    }

    for (uint8 x = 0; x < Width; x++)
    {
        for (uint8 y = 0; y < Height; y++)
        {
            int32 quad = (Height * x) + y;
            data.Indices[(quad * 6) + 0] = (x * (1 + Height)) + y + 1;
            data.Indices[(quad * 6) + 1] = (data.Indices[quad * 6] + 2 + (Height - 1));
            data.Indices[(quad * 6) + 2] = data.Indices[quad * 6] - 1;
            data.Indices[(quad * 6) + 3] = data.Indices[(quad * 6) + 1];
            data.Indices[(quad * 6) + 4] = data.Indices[(quad * 6) + 1] - 1;
            data.Indices[(quad * 6) + 5] = data.Indices[(quad * 6) + 2];
        }
    }

    data.Vertices.ShrinkToFit();
    data.Indices.ShrinkToFit();

    return data;
}

MeshData MeshFactory::CreateSphere(uint32 Subdivisions, float Radius) noexcept
{
    MeshData Sphere;
    Sphere.Vertices.Resize(12);

    float T = (1.0f + sqrt(5.0f)) / 2.0f;
    Sphere.Vertices[0].Position  = XMFLOAT3(-1.0f,  T,     0.0f);
    Sphere.Vertices[1].Position  = XMFLOAT3( 1.0f,  T,     0.0f);
    Sphere.Vertices[2].Position  = XMFLOAT3(-1.0f, -T,     0.0f);
    Sphere.Vertices[3].Position  = XMFLOAT3( 1.0f, -T,     0.0f);
    Sphere.Vertices[4].Position  = XMFLOAT3( 0.0f, -1.0f,  T);
    Sphere.Vertices[5].Position  = XMFLOAT3( 0.0f,  1.0f,  T);
    Sphere.Vertices[6].Position  = XMFLOAT3( 0.0f, -1.0f, -T);
    Sphere.Vertices[7].Position  = XMFLOAT3( 0.0f,  1.0f, -T);
    Sphere.Vertices[8].Position  = XMFLOAT3( T,     0.0f, -1.0f);
    Sphere.Vertices[9].Position  = XMFLOAT3( T,     0.0f,  1.0f);
    Sphere.Vertices[10].Position = XMFLOAT3(-T,     0.0f, -1.0f);
    Sphere.Vertices[11].Position = XMFLOAT3(-T,     0.0f,  1.0f);

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
        Subdivide(Sphere, Subdivisions);
    }

    for (uint32 i = 0; i < static_cast<uint32>(Sphere.Vertices.Size()); i++)
    {
        // Calculate the new position, normal and tangent
        XMVECTOR Position = XMLoadFloat3(&Sphere.Vertices[i].Position);
        Position          = XMVector3Normalize(Position);
        XMStoreFloat3(&Sphere.Vertices[i].Normal, Position);

        Position = XMVectorScale(Position, Radius);
        XMStoreFloat3(&Sphere.Vertices[i].Position, Position);
    
        // Calculate uvs
        Sphere.Vertices[i].TexCoord.y = (asin(Sphere.Vertices[i].Position.y) / XM_PI) + 0.5f;
        Sphere.Vertices[i].TexCoord.x = (atan2f(Sphere.Vertices[i].Position.z, Sphere.Vertices[i].Position.x) + XM_PI) / (2.0f * XM_PI);
    }

    Sphere.Indices.ShrinkToFit();
    Sphere.Vertices.ShrinkToFit();
    
    CalculateTangents(Sphere);

    return Sphere;
}

MeshData MeshFactory::CreateCone(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    MeshData data;
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
        float x = cosf((pi<float>() / 2.0f) + (angle * i));
        float z = sinf((pi<float>() / 2.0f) + (angle * i));

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
    return MeshData();
}

MeshData MeshFactory::CreatePyramid() noexcept
{
    /*
    MeshData data;
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
    return MeshData();
}

MeshData MeshFactory::CreateCylinder(uint32 Sides, float Radius, float Height) noexcept
{
    UNREFERENCED_VARIABLE(Sides);
    UNREFERENCED_VARIABLE(Radius);
    UNREFERENCED_VARIABLE(Height);

    /*
    MeshData data;
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
        float x = cosf((pi<float>() / 2.0f) + (angle * i));
        float z = sinf((pi<float>() / 2.0f) + (angle * i));
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
    return MeshData();
}

void MeshFactory::Subdivide(MeshData& OutData, uint32 Subdivisions) noexcept
{    
    if (Subdivisions < 1)
    {
        return;
    }

    Vertex TempVertices[3];
    uint32 IndexCount     = 0;
    uint32 VertexCount    = 0;
    uint32 OldVertexCount = 0;
    OutData.Vertices.Reserve((OutData.Vertices.Size() * static_cast<uint32>(pow(2, Subdivisions))));
    OutData.Indices.Reserve((OutData.Indices.Size() * static_cast<uint32>(pow(4, Subdivisions))));

    for (uint32 i = 0; i < Subdivisions; i++)
    {
        OldVertexCount = uint32(OutData.Vertices.Size());
        IndexCount     = uint32(OutData.Indices.Size());
        for (uint32 j = 0; j < IndexCount; j += 3)
        {
            // Calculate Position
            XMVECTOR Position0 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j]].Position);
            XMVECTOR Position1 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 1]].Position);
            XMVECTOR Position2 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 2]].Position);

            XMVECTOR Position = XMVectorAdd(Position0, Position1);
            Position = XMVectorScale(Position, 0.5f);
            XMStoreFloat3(&TempVertices[0].Position, Position);

            Position = XMVectorAdd(Position0, Position2);
            Position = XMVectorScale(Position, 0.5f);
            XMStoreFloat3(&TempVertices[1].Position, Position);

            Position = XMVectorAdd(Position1, Position2);
            Position = XMVectorScale(Position, 0.5f);
            XMStoreFloat3(&TempVertices[2].Position, Position);
            
            // Calculate TexCoord
            XMVECTOR TexCoord0 = XMLoadFloat2(&OutData.Vertices[OutData.Indices[j]].TexCoord);
            XMVECTOR TexCoord1 = XMLoadFloat2(&OutData.Vertices[OutData.Indices[j + 1]].TexCoord);
            XMVECTOR TexCoord2 = XMLoadFloat2(&OutData.Vertices[OutData.Indices[j + 2]].TexCoord);

            XMVECTOR TexCoord = XMVectorAdd(TexCoord0, TexCoord1);
            TexCoord = XMVectorScale(TexCoord, 0.5f);
            XMStoreFloat2(&TempVertices[0].TexCoord, TexCoord);

            TexCoord = XMVectorAdd(TexCoord0, TexCoord2);
            TexCoord = XMVectorScale(TexCoord, 0.5f);
            XMStoreFloat2(&TempVertices[1].TexCoord, TexCoord);

            TexCoord = XMVectorAdd(TexCoord1, TexCoord2);
            TexCoord = XMVectorScale(TexCoord, 0.5f);
            XMStoreFloat2(&TempVertices[2].TexCoord, TexCoord);

            // Calculate Normal
            XMVECTOR Normal0 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j]].Normal);
            XMVECTOR Normal1 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 1]].Normal);
            XMVECTOR Normal2 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 2]].Normal);

            XMVECTOR Normal = XMVectorAdd(Normal0, Normal1);
            Normal = XMVectorScale(Normal, 0.5f);
            Normal = XMVector3Normalize(Normal);
            XMStoreFloat3(&TempVertices[0].Normal, Normal);

            Normal = XMVectorAdd(Normal0, Normal2);
            Normal = XMVectorScale(Normal, 0.5f);
            Normal = XMVector3Normalize(Normal);
            XMStoreFloat3(&TempVertices[1].Normal, Normal);

            Normal = XMVectorAdd(Normal1, Normal2);
            Normal = XMVectorScale(Normal, 0.5f);
            Normal = XMVector3Normalize(Normal);
            XMStoreFloat3(&TempVertices[2].Normal, Normal);

            // Calculate Tangent
            XMVECTOR Tangent0 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j]].Tangent);
            XMVECTOR Tangent1 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 1]].Tangent);
            XMVECTOR Tangent2 = XMLoadFloat3(&OutData.Vertices[OutData.Indices[j + 2]].Tangent);

            XMVECTOR Tangent = XMVectorAdd(Tangent0, Tangent1);
            Tangent = XMVectorScale(Tangent, 0.5f);
            Tangent = XMVector3Normalize(Tangent);
            XMStoreFloat3(&TempVertices[0].Tangent, Tangent);

            Tangent = XMVectorAdd(Tangent0, Tangent2);
            Tangent = XMVectorScale(Tangent, 0.5f);
            Tangent = XMVector3Normalize(Tangent);
            XMStoreFloat3(&TempVertices[1].Tangent, Tangent);

            Tangent = XMVectorAdd(Tangent1, Tangent2);
            Tangent = XMVectorScale(Tangent, 0.5f);
            Tangent = XMVector3Normalize(Tangent);
            XMStoreFloat3(&TempVertices[2].Tangent, Tangent);

            // Push the new Vertices
            OutData.Vertices.EmplaceBack(TempVertices[0]);
            OutData.Vertices.EmplaceBack(TempVertices[1]);
            OutData.Vertices.EmplaceBack(TempVertices[2]);

            // Push index of the new triangles
            VertexCount = uint32(OutData.Vertices.Size());
            OutData.Indices.EmplaceBack(VertexCount - 3);
            OutData.Indices.EmplaceBack(VertexCount - 1);
            OutData.Indices.EmplaceBack(VertexCount - 2);

            OutData.Indices.EmplaceBack(VertexCount - 3);
            OutData.Indices.EmplaceBack(OutData.Indices[j + 1]);
            OutData.Indices.EmplaceBack(VertexCount - 1);

            OutData.Indices.EmplaceBack(VertexCount - 2);
            OutData.Indices.EmplaceBack(VertexCount - 1);
            OutData.Indices.EmplaceBack(OutData.Indices[j + 2]);

            // Reassign the old indexes
            OutData.Indices[j + 1] = VertexCount - 3;
            OutData.Indices[j + 2] = VertexCount - 2;
        }

        Optimize(OutData, OldVertexCount);
    }

    OutData.Vertices.ShrinkToFit();
    OutData.Indices.ShrinkToFit();
}

void MeshFactory::Optimize(MeshData& OutData, uint32 StartVertex) noexcept
{
    uint32 VertexCount = static_cast<uint32>(OutData.Vertices.Size());
    uint32 IndexCount  = static_cast<uint32>(OutData.Indices.Size());
        
    uint32 k = 0;
    uint32 j = 0;
    for (uint32 i = StartVertex; i < VertexCount; i++)
    {
        for (j = 0; j < VertexCount; j++)
        {
            if (OutData.Vertices[i] == OutData.Vertices[j])
            {
                if (i != j)
                {
                    OutData.Vertices.Erase(OutData.Vertices.Begin() + i);
                    VertexCount--;
                    j--;

                    for (k = 0; k < IndexCount; k++)
                    {
                        if (OutData.Indices[k] == i)
                        {
                            OutData.Indices[k] = j;
                        }
                        else if (OutData.Indices[k] > i)
                        {
                            OutData.Indices[k]--;
                        }
                    }

                    i--;
                    break;
                }
            }
        }
    }
}

void MeshFactory::CalculateHardNormals(MeshData& Data) noexcept
{
    UNREFERENCED_VARIABLE(Data);

    /*
    XMFLOAT3 e1;
    XMFLOAT3 e2;
    XMFLOAT3 n;

    for (size_t i = 0; i < data.Indices.GetSize(); i += 3)
    {
        e1 = data.Vertices[data.Indices[i + 2]].Position - data.Vertices[data.Indices[i]].Position;
        e2 = data.Vertices[data.Indices[i + 1]].Position - data.Vertices[data.Indices[i]].Position;
        n = cross(e1, e2);

        data.Vertices[data.Indices[i]].Normal = n;
        data.Vertices[data.Indices[i + 1]].Normal = n;
        data.Vertices[data.Indices[i + 2]].Normal = n;
    }
    */
}

void MeshFactory::CalculateTangents(MeshData& OutData) noexcept
{
    auto CalculateTangentFromVectors = [](Vertex& Vertex1, const Vertex& Vertex2, const Vertex& Vertex3)
    {
        XMFLOAT3 Edge1;
        Edge1.x = Vertex2.Position.x - Vertex1.Position.x;
        Edge1.y = Vertex2.Position.y - Vertex1.Position.y;
        Edge1.z = Vertex2.Position.z - Vertex1.Position.z;

        XMFLOAT3 Edge2;
        Edge2.x = Vertex3.Position.x - Vertex1.Position.x;
        Edge2.y = Vertex3.Position.y - Vertex1.Position.y;
        Edge2.z = Vertex3.Position.z - Vertex1.Position.z;

        XMFLOAT2 UVEdge1;
        UVEdge1.x = Vertex2.TexCoord.x - Vertex1.TexCoord.x;
        UVEdge1.y = Vertex2.TexCoord.y - Vertex1.TexCoord.y;

        XMFLOAT3 UVEdge2;
        UVEdge2.x = Vertex3.TexCoord.x - Vertex1.TexCoord.x;
        UVEdge2.y = Vertex3.TexCoord.y - Vertex1.TexCoord.y;

        float Denominator = 1.0f / (UVEdge1.x * UVEdge2.y - UVEdge2.x * UVEdge1.y);

        XMFLOAT3 Tangent;
        Tangent.x = Denominator * (UVEdge2.y * Edge1.x - UVEdge1.y * Edge2.x);
        Tangent.y = Denominator * (UVEdge2.y * Edge1.y - UVEdge1.y * Edge2.y);
        Tangent.z = Denominator * (UVEdge2.y * Edge1.z - UVEdge1.y * Edge2.z);

        float Length = std::sqrt((Tangent.x * Tangent.x) + (Tangent.y * Tangent.y) + (Tangent.z * Tangent.z));
        if (Length != 0.0f) 
        {        
            Tangent.x /= Length;
            Tangent.y /= Length;
            Tangent.z /= Length;
        }

        Vertex1.Tangent = Tangent;
    };

    for (uint32 i = 0; i < OutData.Indices.Size(); i += 3)
    {
        Vertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 0]];
        Vertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 1]];
        Vertex& Vertex3 = OutData.Vertices[OutData.Indices[i + 2]];

        CalculateTangentFromVectors(Vertex1, Vertex2, Vertex3);
        CalculateTangentFromVectors(Vertex2, Vertex3, Vertex1);
        CalculateTangentFromVectors(Vertex3, Vertex1, Vertex2);
    }
}

/*void Mesh::calcNormal()
{
    using namespace Math;

    for (uint32 i = 0; i < indexBuffer->GetSize(); i += 3)
    {
        XMFLOAT3 edge1 = (*vertexBuffer)[(*indexBuffer)[i + 2]].Position - (*vertexBuffer)[(*indexBuffer)[i + 1]].Position;
        XMFLOAT3 edge2 = (*vertexBuffer)[(*indexBuffer)[i]].Position - (*vertexBuffer)[(*indexBuffer)[i + 2]].Position;
        XMFLOAT3 edge3 = (*vertexBuffer)[(*indexBuffer)[i + 1]].Position - (*vertexBuffer)[(*indexBuffer)[i + 0]].Position;

        XMFLOAT3 Normal = edge1.Cross(edge2);
        Normal.Normalize();

        (*vertexBuffer)[(*indexBuffer)[i]].Normal = Normal;
        (*vertexBuffer)[(*indexBuffer)[i + 1]].Normal = Normal;
        (*vertexBuffer)[(*indexBuffer)[i + 2]].Normal = Normal;
    }
}*/

