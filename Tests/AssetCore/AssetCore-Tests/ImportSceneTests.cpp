#include "ImportSceneTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Math/Math.h>
#include <Core/Platform/PlatformFile.h>
#include <AssetCore/ModelImporters/OBJImporter.h>
#include <AssetCore/ImportScene.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

static bool IsNear(const FVector3& A, const FVector3& B)
{
    return Math::Abs(A.X - B.X) < 1e-4f && Math::Abs(A.Y - B.Y) < 1e-4f && Math::Abs(A.Z - B.Z) < 1e-4f;
}

static FVector3 MirrorAlong(FVector3 Vector, int32 Axis)
{
    (Axis == 0 ? Vector.X : (Axis == 1 ? Vector.Y : Vector.Z)) *= -1.0f;
    return Vector;
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

bool ImportScene_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("OBJ objects become nodes with identity transforms, their materials picked through slots and textures listed once"));
    {
        const FString Root = File::MakeAbsolute(TEXT("ImportSceneTests.Scratch"));
        File::DeleteDirectoryTree(Root);

        TEST_EXPECT(WriteText(Root + TEXT("/Crate.mtl"),
            TEXT("newmtl Wood\nKd 0.5 0.4 0.3\nmap_Kd Textures\\wood.png\n\nnewmtl Metal\nmap_Kd Textures/wood.png\nmap_bump Textures/metal_n.png\n")));
        TEST_EXPECT(WriteText(Root + TEXT("/Crate.obj"),
            TEXT("mtllib Crate.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nv 1 1 0\n")
            TEXT("o Crate\nusemtl Wood\nf 1 2 3\nusemtl Metal\nf 2 4 3\n")
            TEXT("o Lid\nusemtl Wood\nf 1 2 4\n")));

        FOBJImporter Importer;
        FImportScene Scene;
        TEST_EXPECT(Importer.ImportFromFile(FStringView(Root + TEXT("/Crate.obj")), EMeshImportFlags::Default, Scene));

        TEST_EXPECT(Scene.Nodes.Size() == 2 && Scene.Meshes.Size() == 2 && Scene.Materials.Size() == 2 && Scene.Textures.Size() == 2);
        if (Scene.Nodes.Size() == 2 && Scene.Meshes.Size() == 2 && Scene.Materials.Size() == 2 && Scene.Textures.Size() == 2)
        {
            const FImportNode& Crate = Scene.Nodes[0];
            const FImportNode& Lid   = Scene.Nodes[1];
            TEST_EXPECT(Crate.Name.Equals(TEXT("Crate")) && Lid.Name.Equals(TEXT("Lid")));
            TEST_EXPECT(Crate.Parent == -1 && Lid.Parent == -1);
            TEST_EXPECT(Crate.LocalTransform == FMatrix4::Identity() && Lid.LocalTransform == FMatrix4::Identity());
            TEST_EXPECT(Crate.Mesh == 0 && Lid.Mesh == 1);

            TEST_EXPECT(Crate.Materials.Size() == 2 && Crate.Materials[0] == 0 && Crate.Materials[1] == 1);
            TEST_EXPECT(Lid.Materials.Size() == 1 && Lid.Materials[0] == 0);
            TEST_EXPECT(Scene.Meshes[0].SubMeshes.Size() == 2 && Scene.Meshes[0].SubMeshes[0].MaterialIndex == 0 && Scene.Meshes[0].SubMeshes[1].MaterialIndex == 1);
            TEST_EXPECT(Scene.Meshes[0].Vertices.Size() == 4 && Scene.Meshes[0].Indices.Size() == 6);

            TEST_EXPECT(Scene.Textures[0].SourcePath.Equals(Root + TEXT("/Textures/wood.png")));
            TEST_EXPECT(Scene.Textures[1].SourcePath.Equals(Root + TEXT("/Textures/metal_n.png")));

            const FImportMaterial& Wood  = Scene.Materials[0];
            const FImportMaterial& Metal = Scene.Materials[1];
            TEST_EXPECT(Wood.Name.Equals(TEXT("Wood")) && Metal.Name.Equals(TEXT("Metal")));
            TEST_EXPECT(Wood.Textures[EMaterialTexture::Diffuse] == 0 && Wood.Textures[EMaterialTexture::Normal] == -1);
            TEST_EXPECT(Metal.Textures[EMaterialTexture::Diffuse] == 0 && Metal.Textures[EMaterialTexture::Normal] == 1);
            TEST_EXPECT(IsNear(Wood.Diffuse, FVector3(0.5f, 0.4f, 0.3f)));
        }

        File::DeleteDirectoryTree(Root);
    }

    TEST_SECTION(TEXT("Baking places each node's mesh with its transform relative to the scene, through its own material slots"));
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

        TEST_EXPECT(ImportScenes::GetWorldTransform(Scene, 1).GetTranslation() == FVector3(10.0f, 5.0f, 0.0f));

        const FMeshData Meshes[] = { ImportScenes::BakeNodeMesh(Scene, 1), ImportScenes::BakeNodeMesh(Scene, 2), ImportScenes::BakeNodeMesh(Scene, 3) };
        {
            const FMeshData& Placed = Meshes[0];
            TEST_EXPECT(Placed.Name.Equals(TEXT("Child")));
            TEST_EXPECT(IsNear(Placed.Vertices[1].Position, FVector3(11.0f, 5.0f, 0.0f)));
            TEST_EXPECT(Placed.SubMeshes[0].MaterialIndex == 1);

            const FMeshData& Stretch = Meshes[1];
            TEST_EXPECT(Stretch.Name.Equals(TEXT("Stretched")) && IsNear(Stretch.Vertices[1].Position, FVector3(2.0f, 0.0f, 0.0f)));
            TEST_EXPECT(IsNear(Stretch.Vertices[0].Normal, FVector3(0.5f, 0.0f, 1.0f).GetNormalized()));
            TEST_EXPECT(Stretch.SubMeshes[0].MaterialIndex == 0);

            const FMeshData& Mirror = Meshes[2];
            TEST_EXPECT(IsNear(Mirror.Vertices[1].Position, FVector3(-1.0f, 0.0f, 0.0f)));
            TEST_EXPECT(Mirror.Indices[0] == 0 && Mirror.Indices[1] == 2 && Mirror.Indices[2] == 1);
            TEST_EXPECT(Mirror.Vertices[0].TangentSign == -1.0f && IsNear(Mirror.Vertices[0].Tangent, FVector3(-1.0f, 0.0f, 0.0f)));
            TEST_EXPECT(Mirror.SubMeshes[0].MaterialIndex == -1);
        }

        TEST_EXPECT(Scene.Meshes.Size() == 1 && IsNear(Scene.Meshes[0].Vertices[1].Position, FVector3(1.0f, 0.0f, 0.0f)));
    }

    TEST_SECTION(TEXT("A transform mirrored with its meshes places them where the original did, mirrored"));
    {
        FMatrix4 Transform = FMatrix4::Identity();
        Transform.M[0][0] = 0.0f;
        Transform.M[0][2] = -1.0f;
        Transform.M[2][0] = 1.0f;
        Transform.M[2][2] = 0.0f;
        Transform.SetTranslation(FVector3(1.0f, 2.0f, 3.0f));

        const FVector3 Point(1.0f, 2.0f, 3.0f);
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            const FVector3 Expected = MirrorAlong(Transform.Transform(Point), Axis);
            TEST_EXPECT(IsNear(ImportScenes::MirrorTransform(Transform, Axis).Transform(MirrorAlong(Point, Axis)), Expected));
        }
    }

    TEST_END();
}
