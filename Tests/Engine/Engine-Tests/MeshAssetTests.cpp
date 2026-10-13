#include "MeshAssetTests.h"

#include <Engine/Assets/MeshAsset.h>
#include <Engine/Assets/MeshFactory.h>

#include "TestCommon/TestMacros.h"

static FGuid MakeGuid(const CHAR_T* Text)
{
    FGuid Guid;
    CHECK(FGuid::Parse(FString(Text), Guid));
    return Guid;
}

static FMeshAssetData MakeCube()
{
    FMeshAssetData Cube;
    Cube.Mesh      = MeshFactory::CreateCube();
    Cube.Mesh.Name = TEXT("Cube");

    FSubMesh& First = Cube.Mesh.SubMeshes.Emplace();
    First.VertexCount   = Cube.Mesh.Vertices.Size();
    First.IndexCount    = 12;
    First.MaterialIndex = 0;

    uint32 FirstVertex = Cube.Mesh.Vertices.Size();
    for (int32 Index = 12; Index < Cube.Mesh.Indices.Size(); ++Index)
    {
        FirstVertex = Math::Min(FirstVertex, Cube.Mesh.Indices[Index]);
    }

    FSubMesh& Second = Cube.Mesh.SubMeshes.Emplace();
    Second.BaseVertex    = FirstVertex;
    Second.VertexCount   = Cube.Mesh.Vertices.Size() - FirstVertex;
    Second.StartIndex    = 12;
    Second.IndexCount    = Cube.Mesh.Indices.Size() - 12;
    Second.MaterialIndex = 1;

    FMeshMaterialSlot& Bricks = Cube.MaterialSlots.Emplace();
    Bricks.Name     = TEXT("Bricks");
    Bricks.Material = TAssetRef<FMaterial>(MakeGuid(TEXT("55555555000000010000000100000001")), TEXT("/Project/Materials/Bricks.dxrmaterial"));

    FMeshMaterialSlot& Trim = Cube.MaterialSlots.Emplace();
    Trim.Name = TEXT("Trim");
    return Cube;
}

bool MeshAsset_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("A mesh's packed streams, indices, submeshes and material slots survive a round trip"));
    {
        const FMeshAssetData Cube = MakeCube();

        TArray<uint8> Payload;
        FString       Error;
        TEST_EXPECT(MeshAssets::Write(Cube, Payload, Error));

        FMeshAssetData Read;
        TEST_EXPECT(MeshAssets::Read(Payload, Read, Error));

        TArray<uint8> Streams[VERTEX_MAX_STREAMS];
        TEST_EXPECT(Cube.Mesh.PackVertexStreams(Streams));

        TEST_EXPECT(Read.Mesh.PackedVertexCount == Cube.Mesh.Vertices.Size());
        TEST_EXPECT(Read.Mesh.Declaration.GetAttributeFlags() == Cube.Mesh.Declaration.GetAttributeFlags());
        for (int32 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; ++StreamIndex)
        {
            TEST_EXPECT(Read.Mesh.PackedStreams[StreamIndex] == Streams[StreamIndex]);
        }

        TEST_EXPECT(Read.Mesh.Indices == Cube.Mesh.Indices);
        TEST_EXPECT(Read.Mesh.SubMeshes.Size() == 2);
        TEST_EXPECT(Read.Mesh.SubMeshes.Size() == 2 && Read.Mesh.SubMeshes[1].StartIndex == 12 && Read.Mesh.SubMeshes[1].IndexCount == Cube.Mesh.SubMeshes[1].IndexCount);
        TEST_EXPECT(Read.Mesh.SubMeshes.Size() == 2 && Read.Mesh.SubMeshes[1].BaseVertex > 0 && Read.Mesh.SubMeshes[1].BaseVertex == Cube.Mesh.SubMeshes[1].BaseVertex);
        TEST_EXPECT(Read.Mesh.SubMeshes.Size() == 2 && Read.Mesh.SubMeshes[0].MaterialIndex == 0 && Read.Mesh.SubMeshes[1].MaterialIndex == 1);

        TEST_EXPECT(Read.MaterialSlots.Size() == 2);
        TEST_EXPECT(Read.MaterialSlots.Size() == 2 && Read.MaterialSlots[0].Name.Equals(TEXT("Bricks")) && Read.MaterialSlots[0].Material == Cube.MaterialSlots[0].Material);
        TEST_EXPECT(Read.MaterialSlots.Size() == 2 && Read.MaterialSlots[0].Material.GetPath().Equals(TEXT("/Project/Materials/Bricks.dxrmaterial")));
        TEST_EXPECT(Read.MaterialSlots.Size() == 2 && Read.MaterialSlots[1].Name.Equals(TEXT("Trim")) && !Read.MaterialSlots[1].Material.IsSet());

        TArray<uint8> Rewritten;
        TEST_EXPECT(MeshAssets::Write(Read, Rewritten, Error) && Rewritten == Payload);

        const TArray<FGuid> Dependencies = MeshAssets::GetDependencies(Cube);
        TEST_EXPECT(Dependencies.Size() == 1 && Dependencies[0] == Cube.MaterialSlots[0].Material.GetGuid());
    }

    TEST_SECTION(TEXT("A mesh without submeshes or materials is written as it is"));
    {
        FMeshAssetData Sphere;
        Sphere.Mesh = MeshFactory::CreateSphere(2);

        TArray<uint8>  Payload;
        FMeshAssetData Read;
        FString        Error;
        TEST_EXPECT(MeshAssets::Write(Sphere, Payload, Error) && MeshAssets::Read(Payload, Read, Error));
        TEST_EXPECT(Read.Mesh.PackedVertexCount == Sphere.Mesh.Vertices.Size() && Read.Mesh.Indices == Sphere.Mesh.Indices);
        TEST_EXPECT(Read.Mesh.SubMeshes.IsEmpty() && Read.MaterialSlots.IsEmpty());
    }

    TEST_SECTION(TEXT("A mesh that isn't consistent isn't written"));
    {
        FString Error;
        TArray<uint8> Payload;

        FMeshAssetData Empty;
        TEST_EXPECT(!MeshAssets::Write(Empty, Payload, Error) && Error.Contains(TEXT("no vertices")));

        FMeshAssetData NoSlot = MakeCube();
        NoSlot.Mesh.SubMeshes[1].MaterialIndex = 2;
        TEST_EXPECT(!MeshAssets::Write(NoSlot, Payload, Error) && Error.Contains(TEXT("Cube")) && Error.Contains(TEXT("material 2 of 2")));

        FMeshAssetData PastTheEnd = MakeCube();
        PastTheEnd.Mesh.SubMeshes[1].IndexCount += 3;
        TEST_EXPECT(!MeshAssets::Write(PastTheEnd, Payload, Error) && Error.Contains(TEXT("submesh")));

        FMeshAssetData BadIndex = MakeCube();
        BadIndex.Mesh.Indices[5] = 1000;
        TEST_EXPECT(!MeshAssets::Write(BadIndex, Payload, Error) && Error.Contains(TEXT("index 5")));
    }

    TEST_SECTION(TEXT("Mesh data that is damaged is refused"));
    {
        TArray<uint8> Payload;
        FString       Error;
        TEST_EXPECT(MeshAssets::Write(MakeCube(), Payload, Error));

        FMeshAssetData Read;

        TArray<uint8> WrongMagic = Payload;
        WrongMagic[0] = 'X';
        TEST_EXPECT(!MeshAssets::Read(WrongMagic, Read, Error) && Error.Contains(TEXT("DXMS")));

        TArray<uint8> Truncated = Payload;
        Truncated.Resize(Payload.Size() - 40);
        TEST_EXPECT(!MeshAssets::Read(Truncated, Read, Error) && Error.Contains(TEXT("past")));

        TArray<uint8> MoreVertices = Payload;
        MoreVertices[12] += 1;
        TEST_EXPECT(!MeshAssets::Read(MoreVertices, Read, Error) && Error.Contains(TEXT("Stream 0")));

        TArray<uint8> NoAttributes = Payload;
        NoAttributes[8] = 0;
        NoAttributes[9] = 0;
        TEST_EXPECT(!MeshAssets::Read(NoAttributes, Read, Error) && Error.Contains(TEXT("vertex attributes")));
    }

    TEST_END();
}
