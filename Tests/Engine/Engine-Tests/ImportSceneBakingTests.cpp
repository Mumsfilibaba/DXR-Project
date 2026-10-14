#include "ImportSceneBakingTests.h"

#include <Core/Math/Math.h>
#include <Engine/Assets/ImportSceneBaking.h>

#include "TestCommon/TestMacros.h"

static bool IsNear(const FVector3& A, const FVector3& B)
{
    return Math::Abs(A.X - B.X) < 1e-4f && Math::Abs(A.Y - B.Y) < 1e-4f && Math::Abs(A.Z - B.Z) < 1e-4f;
}

static FMeshData MakeTriangle()
{
    FMeshData Mesh;
    Mesh.Name = TEXT("Triangle");

    const FVector3 Positions[] = { FVector3(0.0f, 0.0f, 0.0f), FVector3(1.0f, 0.0f, 0.0f), FVector3(0.0f, 1.0f, 0.0f) };
    for (const FVector3& Position : Positions)
    {
        FSourceVertex& Vertex = Mesh.Vertices.Emplace();
        Vertex.Position    = Position;
        Vertex.Normal      = FVector3(1.0f, 0.0f, 1.0f).GetNormalized();
        Vertex.Tangent     = FVector3(1.0f, 0.0f, 0.0f);
        Vertex.TangentSign = 1.0f;
    }

    Mesh.Indices = { 0, 1, 2 };

    FSubMesh& SubMesh = Mesh.SubMeshes.Emplace();
    SubMesh.VertexCount   = 3;
    SubMesh.IndexCount    = 3;
    SubMesh.MaterialIndex = 0;
    return Mesh;
}

bool ImportSceneBaking_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The legacy model bake loads each used texture once, keeps the materials and bakes a mesh per placed node"));
    {
        FImportScene Scene;
        Scene.Meshes.Add(MakeTriangle());
        Scene.Materials.Emplace().Name = TEXT("First");
        Scene.Materials.Emplace().Name = TEXT("Second");
        Scene.Materials[1].Textures[EMaterialTexture::Diffuse] = 0;
        Scene.Textures.Emplace().SourcePath = TEXT("C:/Art/Second.png");

        FImportNode& Root = Scene.Nodes.Emplace();
        Root.Name           = TEXT("Root");
        Root.LocalTransform = FMatrix4::Translation(10.0f, 0.0f, 0.0f);

        FImportNode& Child = Scene.Nodes.Emplace();
        Child.Name           = TEXT("Child");
        Child.Parent         = 0;
        Child.LocalTransform = FMatrix4::Translation(0.0f, 5.0f, 0.0f);
        Child.Mesh           = 0;
        Child.Materials      = { 1 };

        FImportNode& Stretched = Scene.Nodes.Emplace();
        Stretched.Name           = TEXT("Stretched");
        Stretched.LocalTransform = FMatrix4::Scale(2.0f, 1.0f, 1.0f);
        Stretched.Mesh           = 0;
        Stretched.Materials      = { 0 };

        FImportNode& Mirrored = Scene.Nodes.Emplace();
        Mirrored.Name           = TEXT("Mirrored");
        Mirrored.LocalTransform = FMatrix4::Scale(-1.0f, 1.0f, 1.0f);
        Mirrored.Mesh           = 0;

        TArray<FString> LoadedTextures;
        FModelData      ModelData;
        TEST_EXPECT(ImportSceneBaking::BakeToModelData(Scene, ModelData, [&](const FString& Path)
        {
            LoadedTextures.Add(Path);
            return TSharedPtr<FTexture2D>();
        }));

        TEST_EXPECT(LoadedTextures.Size() == 1 && LoadedTextures[0].Equals(TEXT("C:/Art/Second.png")));
        TEST_EXPECT(ModelData.Materials.Size() == 2 && ModelData.Materials[1].Name.Equals(TEXT("Second")));
        TEST_EXPECT(ModelData.Meshes.Size() == 3);

        if (ModelData.Meshes.Size() == 3)
        {
            TEST_EXPECT(ModelData.Meshes[0].Name.Equals(TEXT("Child")) && IsNear(ModelData.Meshes[0].Vertices[1].Position, FVector3(11.0f, 5.0f, 0.0f)));
            TEST_EXPECT(ModelData.Meshes[0].SubMeshes[0].MaterialIndex == 1);
        }
    }

    TEST_END();
}
