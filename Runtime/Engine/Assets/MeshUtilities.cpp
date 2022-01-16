#include "MeshUtilities.h"

void CMeshUtilities::Subdivide(SMeshData& OutData, uint32 Subdivisions) noexcept
{
    if (Subdivisions < 1)
    {
        return;
    }

    SVertex TempVertices[3];
    uint32 IndexCount = 0;
    uint32 VertexCount = 0;
    uint32 OldVertexCount = 0;

    OutData.Vertices.Reserve((OutData.Vertices.Size() * static_cast<uint32>(pow(2, Subdivisions))));
    OutData.Indices.Reserve((OutData.Indices.Size() * static_cast<uint32>(pow(4, Subdivisions))));

    for (uint32 i = 0; i < Subdivisions; i++)
    {
        OldVertexCount = uint32(OutData.Vertices.Size());
        IndexCount = uint32(OutData.Indices.Size());

        Assert(IndexCount % 3 == 0);

        for (uint32 j = 0; j < IndexCount; j += 3)
        {
            // Calculate Position
            CVector3 Position0 = OutData.Vertices[OutData.Indices[j]].Position;
            CVector3 Position1 = OutData.Vertices[OutData.Indices[j + 1]].Position;
            CVector3 Position2 = OutData.Vertices[OutData.Indices[j + 2]].Position;

            CVector3 Position = Position0 + Position1;
            TempVertices[0].Position = Position * 0.5f;

            Position = Position0 + Position2;
            TempVertices[1].Position = Position * 0.5f;

            Position = Position1 + Position2;
            TempVertices[2].Position = Position * 0.5f;

            // Calculate TexCoord
            CVector2 TexCoord0 = OutData.Vertices[OutData.Indices[j]].TexCoord;
            CVector2 TexCoord1 = OutData.Vertices[OutData.Indices[j + 1]].TexCoord;
            CVector2 TexCoord2 = OutData.Vertices[OutData.Indices[j + 2]].TexCoord;

            CVector2 TexCoord = TexCoord0 + TexCoord1;
            TempVertices[0].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord0 + TexCoord2;
            TempVertices[1].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord1 + TexCoord2;
            TempVertices[2].TexCoord = TexCoord * 0.5f;

            // Calculate Normal
            CVector3 Normal0 = OutData.Vertices[OutData.Indices[j]].Normal;
            CVector3 Normal1 = OutData.Vertices[OutData.Indices[j + 1]].Normal;
            CVector3 Normal2 = OutData.Vertices[OutData.Indices[j + 2]].Normal;

            CVector3 Normal = Normal0 + Normal1;
            Normal = Normal * 0.5f;
            TempVertices[0].Normal = Normal.GetNormalized();

            Normal = Normal0 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[1].Normal = Normal.GetNormalized();

            Normal = Normal1 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[2].Normal = Normal.GetNormalized();

            // Calculate Tangent
            CVector3 Tangent0 = OutData.Vertices[OutData.Indices[j]].Tangent;
            CVector3 Tangent1 = OutData.Vertices[OutData.Indices[j + 1]].Tangent;
            CVector3 Tangent2 = OutData.Vertices[OutData.Indices[j + 2]].Tangent;

            CVector3 Tangent = Tangent0 + Tangent1;
            Tangent = Tangent * 0.5f;
            TempVertices[0].Tangent = Tangent.GetNormalized();

            Tangent = Tangent0 + Tangent2;
            Tangent = Tangent * 0.5f;
            TempVertices[1].Tangent = Tangent.GetNormalized();

            Tangent = Tangent1 + Tangent2;
            Tangent = Tangent * 0.5f;
            TempVertices[2].Tangent = Tangent.GetNormalized();

            // Push the new Vertices
            OutData.Vertices.Emplace(TempVertices[0]);
            OutData.Vertices.Emplace(TempVertices[1]);
            OutData.Vertices.Emplace(TempVertices[2]);

            // Push index of the new triangles
            VertexCount = uint32(OutData.Vertices.Size());
            OutData.Indices.Emplace(VertexCount - 3);
            OutData.Indices.Emplace(VertexCount - 1);
            OutData.Indices.Emplace(VertexCount - 2);

            OutData.Indices.Emplace(VertexCount - 3);
            OutData.Indices.Emplace(OutData.Indices[j + 1]);
            OutData.Indices.Emplace(VertexCount - 1);

            OutData.Indices.Emplace(VertexCount - 2);
            OutData.Indices.Emplace(VertexCount - 1);
            OutData.Indices.Emplace(OutData.Indices[j + 2]);

            // Reassign the old indexes
            OutData.Indices[j + 1] = VertexCount - 3;
            OutData.Indices[j + 2] = VertexCount - 2;
        }

        Optimize(OutData, OldVertexCount);
    }

    OutData.Vertices.ShrinkToFit();
    OutData.Indices.ShrinkToFit();
}

void CMeshUtilities::Optimize(SMeshData& OutData, uint32 StartVertex) noexcept
{
    uint32 VertexCount = static_cast<uint32>(OutData.Vertices.Size());
    uint32 IndexCount = static_cast<uint32>(OutData.Indices.Size());

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
                    OutData.Vertices.RemoveAt(i);
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

void CMeshUtilities::CalculateHardNormals(SMeshData& OutData) noexcept
{
    Assert(OutData.Indices.Size() % 3 == 0);

    for (int32 i = 0; i < OutData.Indices.Size(); i += 3)
    {
        SVertex& Vertex0 = OutData.Vertices[OutData.Indices[i + 0]];
        SVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 1]];
        SVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 2]];

        CVector3 Edge0 = Vertex2.Position - Vertex0.Position;
        CVector3 Edge1 = Vertex1.Position - Vertex0.Position;
        CVector3 Normal = Edge0.CrossProduct(Edge1);
        Normal.Normalize();

        Vertex0.Normal = Normal;
        Vertex1.Normal = Normal;
        Vertex2.Normal = Normal;
    }
}

void CMeshUtilities::CalculateSoftNormals(SMeshData& OutData) noexcept
{
    Assert(OutData.Indices.Size() % 3 == 0);

    // TODO: Write better version. For now calculate the hard normals and then average all of them
    CalculateHardNormals(OutData);

    for (int32 i = 0; i < OutData.Indices.Size(); i += 3)
    {
        SVertex& Vertex0 = OutData.Vertices[OutData.Indices[i + 0]];
        SVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 1]];
        SVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 2]];

        CVector3 Edge0 = Vertex2.Position - Vertex0.Position;
        CVector3 Edge1 = Vertex1.Position - Vertex0.Position;
        CVector3 Normal = Edge0.CrossProduct(Edge1);
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

void CMeshUtilities::CalculateTangents(SMeshData& OutData) noexcept
{
    Assert(OutData.Indices.Size() % 3 == 0);

    auto CalculateTangentFromVectors = [](SVertex& Vertex1, const SVertex& Vertex2, const SVertex& Vertex3)
    {
        CVector3 Edge1 = Vertex2.Position - Vertex1.Position;
        CVector3 Edge2 = Vertex3.Position - Vertex1.Position;

        CVector2 UVEdge1 = Vertex2.TexCoord - Vertex1.TexCoord;
        CVector2 UVEdge2 = Vertex3.TexCoord - Vertex1.TexCoord;

        const float RecipDenominator = 1.0f / (UVEdge1.x * UVEdge2.y - UVEdge2.x * UVEdge1.y);

        CVector3 Tangent = RecipDenominator * ((UVEdge2.y * Edge1) - (UVEdge1.y * Edge2));
        Tangent.Normalize();

        Vertex1.Tangent = Tangent;
    };

    for (int32 i = 0; i < OutData.Indices.Size(); i += 3)
    {
        SVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 0]];
        SVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 1]];
        SVertex& Vertex3 = OutData.Vertices[OutData.Indices[i + 2]];

        CalculateTangentFromVectors(Vertex1, Vertex2, Vertex3);
        CalculateTangentFromVectors(Vertex2, Vertex3, Vertex1);
        CalculateTangentFromVectors(Vertex3, Vertex1, Vertex2);
    }
}

void CMeshUtilities::ReverseHandedness(SMeshData& OutData) noexcept
{
    Assert(OutData.Indices.Size() % 3 == 0);

    for (int32 i = 0; i < OutData.Indices.Size(); i += 3)
    {
        uint32 TempIndex = OutData.Indices[i + 1];
        OutData.Indices[i + 1] = OutData.Indices[i + 2];
        OutData.Indices[i + 2] = TempIndex;
    }

    for (int32 i = 0; i < OutData.Vertices.Size(); i++)
    {
        OutData.Vertices[i].Position.z = OutData.Vertices[i].Position.z * -1.0f;
        OutData.Vertices[i].Normal.z = OutData.Vertices[i].Normal.z * -1.0f;
    }
}
