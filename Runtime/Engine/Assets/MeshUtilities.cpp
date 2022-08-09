#include "MeshUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshUtilities

void FMeshUtilities::Subdivide(FMeshData& OutData, uint32 Subdivisions) noexcept
{
    if (Subdivisions < 1)
    {
        return;
    }

    FVertex TempVertices[3];
    uint32 IndexCount = 0;
    uint32 VertexCount = 0;
    uint32 OldVertexCount = 0;

    OutData.Vertices.Reserve((OutData.Vertices.GetSize() * static_cast<uint32>(pow(2, Subdivisions))));
    OutData.Indices.Reserve((OutData.Indices.GetSize() * static_cast<uint32>(pow(4, Subdivisions))));

    for (uint32 i = 0; i < Subdivisions; i++)
    {
        OldVertexCount = uint32(OutData.Vertices.GetSize());
        IndexCount = uint32(OutData.Indices.GetSize());

        Check(IndexCount % 3 == 0);

        for (uint32 j = 0; j < IndexCount; j += 3)
        {
            // Calculate Position
            FVector3 Position0 = OutData.Vertices[OutData.Indices[j]].Position;
            FVector3 Position1 = OutData.Vertices[OutData.Indices[j + 1]].Position;
            FVector3 Position2 = OutData.Vertices[OutData.Indices[j + 2]].Position;

            FVector3 Position = Position0 + Position1;
            TempVertices[0].Position = Position * 0.5f;

            Position = Position0 + Position2;
            TempVertices[1].Position = Position * 0.5f;

            Position = Position1 + Position2;
            TempVertices[2].Position = Position * 0.5f;

            // Calculate TexCoord
            FVector2 TexCoord0 = OutData.Vertices[OutData.Indices[j]].TexCoord;
            FVector2 TexCoord1 = OutData.Vertices[OutData.Indices[j + 1]].TexCoord;
            FVector2 TexCoord2 = OutData.Vertices[OutData.Indices[j + 2]].TexCoord;

            FVector2 TexCoord = TexCoord0 + TexCoord1;
            TempVertices[0].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord0 + TexCoord2;
            TempVertices[1].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord1 + TexCoord2;
            TempVertices[2].TexCoord = TexCoord * 0.5f;

            // Calculate Normal
            FVector3 Normal0 = OutData.Vertices[OutData.Indices[j]].Normal;
            FVector3 Normal1 = OutData.Vertices[OutData.Indices[j + 1]].Normal;
            FVector3 Normal2 = OutData.Vertices[OutData.Indices[j + 2]].Normal;

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
            FVector3 Tangent0 = OutData.Vertices[OutData.Indices[j]].Tangent;
            FVector3 Tangent1 = OutData.Vertices[OutData.Indices[j + 1]].Tangent;
            FVector3 Tangent2 = OutData.Vertices[OutData.Indices[j + 2]].Tangent;

            FVector3 Tangent = Tangent0 + Tangent1;
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
            VertexCount = uint32(OutData.Vertices.GetSize());
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

void FMeshUtilities::Optimize(FMeshData& OutData, uint32 StartVertex) noexcept
{
    uint32 VertexCount = static_cast<uint32>(OutData.Vertices.GetSize());
    uint32 IndexCount = static_cast<uint32>(OutData.Indices.GetSize());

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

void FMeshUtilities::CalculateHardNormals(FMeshData& OutData) noexcept
{
    Check(OutData.Indices.GetSize() % 3 == 0);

    for (int32 i = 0; i < OutData.Indices.GetSize(); i += 3)
    {
        FVertex& Vertex0 = OutData.Vertices[OutData.Indices[i + 0]];
        FVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 1]];
        FVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 2]];

        FVector3 Edge0 = Vertex2.Position - Vertex0.Position;
        FVector3 Edge1 = Vertex1.Position - Vertex0.Position;
        FVector3 Normal = Edge0.CrossProduct(Edge1);
        Normal.Normalize();

        Vertex0.Normal = Normal;
        Vertex1.Normal = Normal;
        Vertex2.Normal = Normal;
    }
}

void FMeshUtilities::CalculateSoftNormals(FMeshData& OutData) noexcept
{
    Check(OutData.Indices.GetSize() % 3 == 0);

    // TODO: Write better version. For now calculate the hard normals and then average all of them
    CalculateHardNormals(OutData);

    for (int32 i = 0; i < OutData.Indices.GetSize(); i += 3)
    {
        FVertex& Vertex0 = OutData.Vertices[OutData.Indices[i + 0]];
        FVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 1]];
        FVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 2]];

        FVector3 Edge0 = Vertex2.Position - Vertex0.Position;
        FVector3 Edge1 = Vertex1.Position - Vertex0.Position;
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

void FMeshUtilities::CalculateTangents(FMeshData& OutData) noexcept
{
    Check(OutData.Indices.GetSize() % 3 == 0);

    auto CalculateTangentFromVectors = [](FVertex& Vertex1, const FVertex& Vertex2, const FVertex& Vertex3)
    {
        FVector3 Edge1 = Vertex2.Position - Vertex1.Position;
        FVector3 Edge2 = Vertex3.Position - Vertex1.Position;

        FVector2 UVEdge1 = Vertex2.TexCoord - Vertex1.TexCoord;
        FVector2 UVEdge2 = Vertex3.TexCoord - Vertex1.TexCoord;

        const float RecipDenominator = 1.0f / (UVEdge1.x * UVEdge2.y - UVEdge2.x * UVEdge1.y);

        FVector3 Tangent = RecipDenominator * ((UVEdge2.y * Edge1) - (UVEdge1.y * Edge2));
        Tangent.Normalize();

        Vertex1.Tangent = Tangent;
    };

    for (int32 i = 0; i < OutData.Indices.GetSize(); i += 3)
    {
        FVertex& Vertex1 = OutData.Vertices[OutData.Indices[i + 0]];
        FVertex& Vertex2 = OutData.Vertices[OutData.Indices[i + 1]];
        FVertex& Vertex3 = OutData.Vertices[OutData.Indices[i + 2]];

        CalculateTangentFromVectors(Vertex1, Vertex2, Vertex3);
        CalculateTangentFromVectors(Vertex2, Vertex3, Vertex1);
        CalculateTangentFromVectors(Vertex3, Vertex1, Vertex2);
    }
}

void FMeshUtilities::ReverseHandedness(FMeshData& OutData) noexcept
{
    Check(OutData.Indices.GetSize() % 3 == 0);

    for (int32 i = 0; i < OutData.Indices.GetSize(); i += 3)
    {
        uint32 TempIndex = OutData.Indices[i + 1];
        OutData.Indices[i + 1] = OutData.Indices[i + 2];
        OutData.Indices[i + 2] = TempIndex;
    }

    for (int32 i = 0; i < OutData.Vertices.GetSize(); i++)
    {
        OutData.Vertices[i].Position.z = OutData.Vertices[i].Position.z * -1.0f;
        OutData.Vertices[i].Normal.z = OutData.Vertices[i].Normal.z * -1.0f;
    }
}
