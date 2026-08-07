#include "Engine/Assets/MeshData.h"
#include "Core/Containers/Map.h"
#include "Core/Misc/OutputDeviceLogger.h"

// Below this a tangent is treated as collapsed, since normalizing it in a shader would produce NaN.
constexpr float TangentLengthSquaredEpsilon = 1.0e-12f;

struct FTriangleTangentFrame
{
    Vector3 Tangent;
    Vector3 Bitangent;
    float   Determinant;
};

// Lengyel's method
static FTriangleTangentFrame ComputeTriangleTangentFrame(const FSourceVertex& Vertex0, const FSourceVertex& Vertex1, const FSourceVertex& Vertex2)
{
    const Vector3 Edge1    = Vertex1.Position - Vertex0.Position;
    const Vector3 Edge2    = Vertex2.Position - Vertex0.Position;
    const Vector2 DeltaUV1 = Vertex1.TexCoord - Vertex0.TexCoord;
    const Vector2 DeltaUV2 = Vertex2.TexCoord - Vertex0.TexCoord;

    FTriangleTangentFrame Frame;
    Frame.Determinant = DeltaUV1.X * DeltaUV2.Y - DeltaUV2.X * DeltaUV1.Y;

    const float RcpDenom = Math::Abs<float>(Frame.Determinant) > 0.0f ? 1.0f / Frame.Determinant : 0.0f;

    Frame.Tangent.X = RcpDenom * (DeltaUV2.Y * Edge1.X - DeltaUV1.Y * Edge2.X);
    Frame.Tangent.Y = RcpDenom * (DeltaUV2.Y * Edge1.Y - DeltaUV1.Y * Edge2.Y);
    Frame.Tangent.Z = RcpDenom * (DeltaUV2.Y * Edge1.Z - DeltaUV1.Y * Edge2.Z);

    Frame.Bitangent.X = RcpDenom * (DeltaUV1.X * Edge2.X - DeltaUV2.X * Edge1.X);
    Frame.Bitangent.Y = RcpDenom * (DeltaUV1.X * Edge2.Y - DeltaUV2.X * Edge1.Y);
    Frame.Bitangent.Z = RcpDenom * (DeltaUV1.X * Edge2.Z - DeltaUV2.X * Edge1.Z);

    return Frame;
}

static float DeriveTangentSign(const Vector3& Normal, const Vector3& Tangent, const Vector3& AccumulatedBitangent)
{
    return (Normal.CrossProduct(Tangent).DotProduct(AccumulatedBitangent) < 0.0f) ? -1.0f : 1.0f;
}

FMeshData::FMeshData()
    : Name()
    , SubMeshes()
    , Indices()
    , Vertices()
    , Declaration(FVertexDeclaration::GetStandardStaticMesh())
    , PackedStreams()
    , PackedVertexCount(0)
{
}

FMeshData::FMeshData(const FMeshData& Other) = default;

FMeshData::FMeshData(FMeshData&& Other) = default;

FMeshData::~FMeshData() = default;

FMeshData& FMeshData::operator=(const FMeshData& Other) = default;

FMeshData& FMeshData::operator=(FMeshData&& Other) = default;

static_assert(TIsMoveConstructible<FMeshData>::Value,
    "FMeshData is moved into FModelData::Meshes by the importers; without a move constructor TArray silently deep-copies every mesh");

bool FMeshData::PackVertexStreams(TArray<uint8> (&OutStreams)[VERTEX_MAX_STREAMS]) const
{
    const int32 VertexCount = Vertices.Size();

    for (uint8 StreamIndex = 0; StreamIndex < Declaration.GetNumStreams(); StreamIndex++)
    {
        const uint16 Stride = Declaration.GetStreamStride(StreamIndex);
        if (Stride == 0)
        {
            continue;
        }

        TArray<uint8>& StreamData = OutStreams[StreamIndex];
        StreamData.Resize(VertexCount * Stride);

        for (const FVertexAttributeInfo& Attribute : Declaration.GetAttributes())
        {
            if (Attribute.StreamIndex != StreamIndex)
            {
                continue;
            }

            for (int32 Index = 0; Index < VertexCount; Index++)
            {
                const FSourceVertex& Vertex = Vertices[Index];
                void* Destination = StreamData.Data() + (Index * Stride) + Attribute.ByteOffset;

                switch (Attribute.Element)
                {
                    case EVertexElement::Position:
                        *reinterpret_cast<Vector3*>(Destination) = Vertex.Position;
                        break;

                    case EVertexElement::Normal:
                        *reinterpret_cast<FRGBA16Snorm*>(Destination) = FRGBA16Snorm(Vertex.Normal);
                        break;

                    case EVertexElement::Tangent:
                        *reinterpret_cast<FRGBA16Snorm*>(Destination) = FRGBA16Snorm(Vertex.Tangent, Vertex.TangentSign);
                        break;

                    case EVertexElement::TexCoord0:
                        *reinterpret_cast<Vector2*>(Destination) = Vertex.TexCoord;
                        break;

                    default:
                        LOG_ERROR("Mesh '%s' declares vertex element %u, which FSourceVertex cannot provide", Name.Data(), UnderlyingTypeValue(Attribute.Element));
                        return false;
                }
            }
        }
    }

    return true;
}

TArray<uint16> FMeshData::GetSmallIndices() const
{
    TArray<uint16> NewArray;
    NewArray.Reserve(Indices.Size());

    for (uint32 Index : Indices)
    {
        NewArray.Add(static_cast<uint16>(Index));
    }

    return NewArray;
}

void FMeshData::Optimize(uint32 StartVertex)
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

void FMeshData::CalculateHardNormals()
{
    CHECK(Indices.Size() % 3 == 0);

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        FSourceVertex& Vertex0 = Vertices[Indices[i + 0]];
        FSourceVertex& Vertex1 = Vertices[Indices[i + 1]];
        FSourceVertex& Vertex2 = Vertices[Indices[i + 2]];

        Vector3 Edge0  = Vertex2.Position - Vertex0.Position;
        Vector3 Edge1  = Vertex1.Position - Vertex0.Position;
        Vector3 Normal = Edge0.CrossProduct(Edge1);
        Normal.Normalize();

        Vertex0.Normal = Normal;
        Vertex1.Normal = Normal;
        Vertex2.Normal = Normal;
    }
}

void FMeshData::CalculateSoftNormals()
{
    CHECK(Indices.Size() % 3 == 0);

    // TODO: Write better version. For now calculate the hard normals and then average all of them
    CalculateHardNormals();

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        FSourceVertex& Vertex0 = Vertices[Indices[i + 0]];
        FSourceVertex& Vertex1 = Vertices[Indices[i + 1]];
        FSourceVertex& Vertex2 = Vertices[Indices[i + 2]];

        Vector3 Edge0  = Vertex2.Position - Vertex0.Position;
        Vector3 Edge1  = Vertex1.Position - Vertex0.Position;
        Vector3 Normal = Edge0.CrossProduct(Edge1);
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

void FMeshData::CalculateTangents()
{
    CHECK(Indices.Size() % 3 == 0);

    TArray<Vector3> TangentAccumulation;
    TangentAccumulation.Resize(Vertices.Size());

    TArray<Vector3> BitangentAccumulation;
    BitangentAccumulation.Resize(Vertices.Size());

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        const uint32 Index0 = Indices[i + 0];
        const uint32 Index1 = Indices[i + 1];
        const uint32 Index2 = Indices[i + 2];

        const FTriangleTangentFrame Frame = ComputeTriangleTangentFrame(Vertices[Index0], Vertices[Index1], Vertices[Index2]);
        TangentAccumulation[Index0] += Frame.Tangent;
        TangentAccumulation[Index1] += Frame.Tangent;
        TangentAccumulation[Index2] += Frame.Tangent;

        BitangentAccumulation[Index0] += Frame.Bitangent;
        BitangentAccumulation[Index1] += Frame.Bitangent;
        BitangentAccumulation[Index2] += Frame.Bitangent;
    }

    for (int32 i = 0; i < Vertices.Size(); i++)
    {
        const Vector3 Tangent = TangentAccumulation[i].GetNormalized();
        Vertices[i].Tangent     = Tangent.GetOrthonormalTo(Vertices[i].Normal);
        Vertices[i].TangentSign = DeriveTangentSign(Vertices[i].Normal, Vertices[i].Tangent, BitangentAccumulation[i]);
    }

    ValidateTangents();
    SplitTangentSeams();
}

void FMeshData::CalculateTangentSigns()
{
    CHECK(Indices.Size() % 3 == 0);

    TArray<Vector3> BitangentAccumulation;
    BitangentAccumulation.Resize(Vertices.Size());

    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        const uint32 Index0 = Indices[i + 0];
        const uint32 Index1 = Indices[i + 1];
        const uint32 Index2 = Indices[i + 2];

        const FTriangleTangentFrame Frame = ComputeTriangleTangentFrame(Vertices[Index0], Vertices[Index1], Vertices[Index2]);
        BitangentAccumulation[Index0] += Frame.Bitangent;
        BitangentAccumulation[Index1] += Frame.Bitangent;
        BitangentAccumulation[Index2] += Frame.Bitangent;
    }

    for (int32 i = 0; i < Vertices.Size(); i++)
    {
        Vertices[i].TangentSign = DeriveTangentSign(Vertices[i].Normal, Vertices[i].Tangent, BitangentAccumulation[i]);
    }

    ValidateTangents();
    SplitTangentSeams();
}

void FMeshData::SplitTangentSeams()
{
    CHECK(Indices.Size() % 3 == 0);

    const int32 OriginalVertexCount = Vertices.Size();

    TMap<uint32, uint32> SplitLookup;
    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        const FTriangleTangentFrame Frame = ComputeTriangleTangentFrame(Vertices[Indices[i + 0]], Vertices[Indices[i + 1]], Vertices[Indices[i + 2]]);
        if (Frame.Determinant == 0.0f)
        {
            continue;
        }

        const float TriangleSign = (Frame.Determinant < 0.0f) ? -1.0f : 1.0f;

        for (int32 Corner = 0; Corner < 3; Corner++)
        {
            const uint32 VertexIndex = Indices[i + Corner];
            if (Vertices[VertexIndex].TangentSign == TriangleSign)
            {
                continue;
            }

            // The sign is binary, so one duplicate per original vertex covers every triangle that disagrees with it.
            if (uint32* ExistingSplit = SplitLookup.Find(VertexIndex))
            {
                Indices[i + Corner] = *ExistingSplit;
                continue;
            }

            FSourceVertex SplitVertex = Vertices[VertexIndex];
            SplitVertex.TangentSign = TriangleSign;

            const uint32 SplitIndex = static_cast<uint32>(Vertices.Size());
            Vertices.Add(SplitVertex);

            SplitLookup[VertexIndex] = SplitIndex;
            Indices[i + Corner]      = SplitIndex;
        }
    }

    // Duplicates are appended to the end of the shared vertex array and reached through absolute indices, so they
    // do not belong to whichever submesh happens to be last. Rebuild every range from the indices it actually uses.
    const int32 NumSplitVertices = Vertices.Size() - OriginalVertexCount;
    if (NumSplitVertices > 0)
    {
        for (FSubMesh& SubMesh : SubMeshes)
        {
            if (SubMesh.IndexCount == 0)
            {
                continue;
            }

            uint32 MinIndex = ~uint32(0);
            uint32 MaxIndex = 0;

            const int32 IndexBegin = static_cast<int32>(SubMesh.StartIndex);
            const int32 IndexEnd   = IndexBegin + static_cast<int32>(SubMesh.IndexCount);
            for (int32 i = IndexBegin; i < IndexEnd; i++)
            {
                MinIndex = (Indices[i] < MinIndex) ? Indices[i] : MinIndex;
                MaxIndex = (Indices[i] > MaxIndex) ? Indices[i] : MaxIndex;
            }

            SubMesh.BaseVertex  = MinIndex;
            SubMesh.VertexCount = (MaxIndex - MinIndex) + 1;
        }
    }
}

void FMeshData::ValidateTangents()
{
    // A tangent that collapsed to zero is as unusable as a NaN.
    const auto IsValid = [](const Vector3& Vector)
    {
        return !Vector.ContainsInfinity() && !Vector.ContainsNaN() && Vector.GetLengthSquared() > TangentLengthSquaredEpsilon;
    };

    // Loop over each triangle (assumes indices are in groups of 3).
    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        const uint32 Index0 = Indices[i + 0];
        const uint32 Index1 = Indices[i + 1];
        const uint32 Index2 = Indices[i + 2];

        FSourceVertex& Vertex1 = Vertices[Index0];
        FSourceVertex& Vertex2 = Vertices[Index1];
        FSourceVertex& Vertex3 = Vertices[Index2];

        const bool bValid1 = IsValid(Vertex1.Tangent);
        const bool bValid2 = IsValid(Vertex2.Tangent);
        const bool bValid3 = IsValid(Vertex3.Tangent);

        // If any vertex in the triangle has invalid data, recalculate based on triangle geometry.
        if (!bValid1 || !bValid2 || !bValid3)
        {
            // Compute two edge vectors from the triangle.
            Vector3 Edge1 = Vertex2.Position - Vertex1.Position;
            Vector3 Edge2 = Vertex3.Position - Vertex1.Position;

            // Calculate the triangle's normal using the cross product, then normalize.
            Vector3 TriangleNormal  = Edge1.CrossProduct(Edge2).GetNormalized();
            Vector3 Arbitrary       = (Math::Abs<float>(TriangleNormal.X) < 0.9f) ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
            Vector3 TriangleTangent = TriangleNormal.CrossProduct(Arbitrary).GetNormalized();

            // Update vertices with invalid tangent.
            if (!bValid1)
            {
                Vertex1.Tangent     = TriangleTangent.GetOrthonormalTo(TriangleNormal);
                Vertex1.TangentSign = 1.0f;
            }

            if (!bValid2)
            {
                Vertex2.Tangent     = TriangleTangent.GetOrthonormalTo(TriangleNormal);
                Vertex2.TangentSign = 1.0f;
            }

            if (!bValid3)
            {
                Vertex3.Tangent     = TriangleTangent.GetOrthonormalTo(TriangleNormal);
                Vertex3.TangentSign = 1.0f;
            }
        }
    }
}

void FMeshData::ReverseHandedness()
{
    CHECK(Indices.Size() % 3 == 0);

    // Reverse the triangle winding order
    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        uint32 TempIndex = Indices[i + 1];
        Indices[i + 1]   = Indices[i + 2];
        Indices[i + 2]   = TempIndex;
    }

    for (int32 i = 0; i < Vertices.Size(); ++i)
    {
        Vertices[i].Position.Z  *= -1.0f;
        Vertices[i].Normal.Z    *= -1.0f;
        Vertices[i].Tangent.Z   *= -1.0f;
        Vertices[i].TangentSign *= -1.0f;
    }
}

void FMeshData::InvertAxisX()
{
    // Reverse the triangle winding order
    for (int32 i = 0; i < Indices.Size(); i += 3)
    {
        uint32 TempIndex = Indices[i + 1];
        Indices[i + 1] = Indices[i + 2];
        Indices[i + 2] = TempIndex;
    }

    for (int32 i = 0; i < Vertices.Size(); ++i)
    {
        Vertices[i].Position.X  *= -1.0f;
        Vertices[i].Normal.X    *= -1.0f;
        Vertices[i].Tangent.X   *= -1.0f;
        Vertices[i].TangentSign *= -1.0f;
    }
}

void FMeshData::Subdivide(uint32 Subdivisions)
{
    if (Subdivisions < 1)
    {
        return;
    }

    FSourceVertex TempVertices[3];

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
            Vector3 Position0 = Vertices[Indices[j]].Position;
            Vector3 Position1 = Vertices[Indices[j + 1]].Position;
            Vector3 Position2 = Vertices[Indices[j + 2]].Position;

            Vector3 Position = Position0 + Position1;
            TempVertices[0].Position = Position * 0.5f;

            Position = Position0 + Position2;
            TempVertices[1].Position = Position * 0.5f;

            Position = Position1 + Position2;
            TempVertices[2].Position = Position * 0.5f;

            // Calculate TexCoord
            Vector2 TexCoord0 = Vertices[Indices[j]].TexCoord;
            Vector2 TexCoord1 = Vertices[Indices[j + 1]].TexCoord;
            Vector2 TexCoord2 = Vertices[Indices[j + 2]].TexCoord;

            Vector2 TexCoord = TexCoord0 + TexCoord1;
            TempVertices[0].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord0 + TexCoord2;
            TempVertices[1].TexCoord = TexCoord * 0.5f;

            TexCoord = TexCoord1 + TexCoord2;
            TempVertices[2].TexCoord = TexCoord * 0.5f;

            // Calculate Normal
            Vector3 Normal0 = Vertices[Indices[j]].Normal;
            Vector3 Normal1 = Vertices[Indices[j + 1]].Normal;
            Vector3 Normal2 = Vertices[Indices[j + 2]].Normal;

            Vector3 Normal = Normal0 + Normal1;
            Normal = Normal * 0.5f;
            TempVertices[0].Normal = Normal.GetNormalized();

            Normal = Normal0 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[1].Normal = Normal.GetNormalized();

            Normal = Normal1 + Normal2;
            Normal = Normal * 0.5f;
            TempVertices[2].Normal = Normal.GetNormalized();

            // Calculate Tangent
            Vector3 Tangent0 = Vertices[Indices[j]].Tangent;
            Vector3 Tangent1 = Vertices[Indices[j + 1]].Tangent;
            Vector3 Tangent2 = Vertices[Indices[j + 2]].Tangent;

            Vector3 Tangent         = Tangent0 + Tangent1;
            Tangent                 = Tangent * 0.5f;
            TempVertices[0].Tangent = Tangent.GetNormalized();

            Tangent                 = Tangent0 + Tangent2;
            Tangent                 = Tangent * 0.5f;
            TempVertices[1].Tangent = Tangent.GetNormalized();

            Tangent                 = Tangent1 + Tangent2;
            Tangent                 = Tangent * 0.5f;
            TempVertices[2].Tangent = Tangent.GetNormalized();

            const float TangentSign0 = Vertices[Indices[j]].TangentSign;
            const float TangentSign1 = Vertices[Indices[j + 1]].TangentSign;
            const float TangentSign2 = Vertices[Indices[j + 2]].TangentSign;

            TempVertices[0].TangentSign = TangentSign0 == TangentSign1 ? TangentSign0 : 1.0f;
            TempVertices[1].TangentSign = TangentSign0 == TangentSign2 ? TangentSign0 : 1.0f;
            TempVertices[2].TangentSign = TangentSign1 == TangentSign2 ? TangentSign1 : 1.0f;

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
