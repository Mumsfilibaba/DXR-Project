#include "PrimitiveShapeTests.h"

#include <Engine/Assets/PrimitiveShapes.h>
#include <Engine/Core/BackgroundTasks.h>

#include "TestCommon/TestMacros.h"

#include <Core/Containers/Map.h>

static FString MakePositionKey(const FVector3& Position)
{
    return FString::Printf(TEXT("%.4f,%.4f,%.4f"), Position.X, Position.Y, Position.Z);
}

static bool IsClosed(const FMeshData& MeshData)
{
    TMap<FString, int32> EdgeUses;
    for (int32 Index = 0; Index < MeshData.Indices.Size(); Index += 3)
    {
        for (int32 Corner = 0; Corner < 3; ++Corner)
        {
            const FString First  = MakePositionKey(MeshData.Vertices[MeshData.Indices[Index + Corner]].Position);
            const FString Second = MakePositionKey(MeshData.Vertices[MeshData.Indices[Index + (Corner + 1) % 3]].Position);
            if (First == Second)
            {
                return false;
            }

            EdgeUses.FindOrAdd(First < Second ? First + TEXT("|") + Second : Second + TEXT("|") + First)++;
        }
    }

    for (const TPair<const FString&, int32&>& Edge : EdgeUses)
    {
        if (Edge.Second != 2)
        {
            return false;
        }
    }

    return true;
}

static FPrimitiveShape MakeAt(EPrimitiveShape Type, bool bMaximum)
{
    const FPrimitiveShapeDesc& Desc = PrimitiveShapes::GetDesc(Type);

    FPrimitiveShape Shape = PrimitiveShapes::MakeDefault(Type);
    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        Shape.Values[Index] = bMaximum ? Desc.Parameters[Index].Max : Desc.Parameters[Index].Min;
    }

    return Shape;
}

bool PrimitiveShapes_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Every shape has a name, parameters with defaults inside their ranges, and is found by name"));
    {
        for (int32 Index = 0; Index < PrimitiveShapes::NumShapes; ++Index)
        {
            const EPrimitiveShape      Type = static_cast<EPrimitiveShape>(Index);
            const FPrimitiveShapeDesc& Desc = PrimitiveShapes::GetDesc(Type);

            EPrimitiveShape Found = EPrimitiveShape::Count;
            TEST_EXPECT(PrimitiveShapes::FindByName(Desc.Name, Found) && Found == Type);
            TEST_EXPECT(Desc.NumParameters > 0 && Desc.NumParameters <= FPrimitiveShapeDesc::MaxParameters);

            for (int32 Parameter = 0; Parameter < Desc.NumParameters; ++Parameter)
            {
                const FShapeParameter& Info = Desc.Parameters[Parameter];
                TEST_EXPECT(Info.Min < Info.Max && Info.Default >= Info.Min && Info.Default <= Info.Max);
                TEST_EXPECT(Info.SliderMax > Info.Min && Info.SliderMax <= Info.Max && Info.Default <= Info.SliderMax);
            }

            TEST_EXPECT(PrimitiveShapes::Clamp(PrimitiveShapes::MakeDefault(Type)) == PrimitiveShapes::MakeDefault(Type));
        }

        EPrimitiveShape Found = EPrimitiveShape::Count;
        TEST_EXPECT(!PrimitiveShapes::FindByName(TEXT("Dodecahedron"), Found));
    }

    TEST_SECTION(TEXT("Values are clamped to their ranges and integer parameters are rounded"));
    {
        FPrimitiveShape Sphere = PrimitiveShapes::MakeDefault(EPrimitiveShape::Sphere);
        Sphere.Values[0] = 2.6f;
        Sphere.Values[1] = -4.0f;

        const FPrimitiveShape Clamped = PrimitiveShapes::Clamp(Sphere);
        TEST_EXPECT(Clamped.Values[0] == 3.0f);
        TEST_EXPECT(Clamped.Values[1] == PrimitiveShapes::GetDesc(EPrimitiveShape::Sphere).Parameters[1].Min);

        Sphere.Values[0] = 100.0f;
        TEST_EXPECT(PrimitiveShapes::Clamp(Sphere).Values[0] == PrimitiveShapes::GetDesc(EPrimitiveShape::Sphere).Parameters[0].Max);
    }

    TEST_SECTION(TEXT("Every shape builds at its minimum, default and maximum values"));
    {
        for (int32 Index = 0; Index < PrimitiveShapes::NumShapes; ++Index)
        {
            const EPrimitiveShape Type = static_cast<EPrimitiveShape>(Index);

            for (const FPrimitiveShape& Shape : { MakeAt(Type, false), PrimitiveShapes::MakeDefault(Type), MakeAt(Type, true) })
            {
                const FMeshData MeshData = PrimitiveShapes::BuildMeshData(Shape);
                TEST_EXPECT(MeshData.Vertices.Size() > 0 && MeshData.Indices.Size() > 0 && MeshData.Indices.Size() % 3 == 0);
                TEST_EXPECT(PrimitiveShapes::EstimateTriangles(Shape) > 0);
            }
        }
    }

    TEST_SECTION(TEXT("More subdivisions build more triangles, and the estimate follows"));
    {
        FPrimitiveShape Coarse = PrimitiveShapes::MakeDefault(EPrimitiveShape::Sphere);
        FPrimitiveShape Fine   = Coarse;
        Coarse.Values[0] = 2.0f;
        Fine.Values[0]   = 4.0f;

        TEST_EXPECT(PrimitiveShapes::BuildMeshData(Fine).Indices.Size() > PrimitiveShapes::BuildMeshData(Coarse).Indices.Size());
        TEST_EXPECT(PrimitiveShapes::EstimateTriangles(Fine) == PrimitiveShapes::EstimateTriangles(Coarse) * 16);
    }

    TEST_SECTION(TEXT("Subdivided spheres are closed, and merging their shared vertices keeps them small"));
    {
        for (int32 Subdivisions = 0; Subdivisions <= 3; ++Subdivisions)
        {
            FPrimitiveShape Sphere = PrimitiveShapes::MakeDefault(EPrimitiveShape::Sphere);
            Sphere.Values[0] = static_cast<float>(Subdivisions);

            const FMeshData MeshData = PrimitiveShapes::BuildMeshData(Sphere);
            TEST_EXPECT(IsClosed(MeshData));
            TEST_EXPECT(MeshData.Indices.Size() == 60 << (2 * Subdivisions));

            // An icosphere has 10 * 4^n + 2 distinct positions; the texture seam adds a few copies
            const int32 Distinct = 10 * (1 << (2 * Subdivisions)) + 2;
            TEST_EXPECT(MeshData.Vertices.Size() >= Distinct && MeshData.Vertices.Size() < Distinct * 2);
        }
    }

    TEST_SECTION(TEXT("Equal shapes hash alike, and a changed value or type changes the shape"));
    {
        const FPrimitiveShape Cube  = PrimitiveShapes::MakeDefault(EPrimitiveShape::Cube);
        FPrimitiveShape       Other = Cube;
        TEST_EXPECT(Cube == Other && PrimitiveShapes::GetHash(Cube) == PrimitiveShapes::GetHash(Other));

        Other.Values[1] = 2.0f;
        TEST_EXPECT(Cube != Other);

        Other = Cube;
        Other.Type = EPrimitiveShape::Pyramid;
        TEST_EXPECT(Cube != Other);
    }

    TEST_SECTION(TEXT("A shape is described by its name and values"));
    {
        TEST_EXPECT(PrimitiveShapes::Describe(PrimitiveShapes::MakeDefault(EPrimitiveShape::Sphere)).Equals(TEXT("Sphere, subdivisions 3, radius 0.50 m")));
    }

    TEST_END();
}

bool BackgroundTasks_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("A task is shown once it has run for the delay, and forgotten once finished"));
    {
        FBackgroundTasks Tasks;

        TSharedPtr<FBackgroundTask> Task = Tasks.Begin(TEXT("Building"), true, 10.0);
        TEST_EXPECT(Tasks.GetNumRunningTasks() == 1);
        TEST_EXPECT(Tasks.GetVisibleTasks(10.1).IsEmpty());
        TEST_EXPECT(Tasks.GetVisibleTasks(10.0 + FBackgroundTasks::ShowAfterSeconds).Size() == 1);

        Task->Finish();
        TEST_EXPECT(Tasks.GetVisibleTasks(20.0).IsEmpty());
        TEST_EXPECT(Tasks.GetNumRunningTasks() == 0);
    }

    TEST_SECTION(TEXT("Tasks are shown oldest first"));
    {
        FBackgroundTasks Tasks;

        TSharedPtr<FBackgroundTask> First  = Tasks.Begin(TEXT("First"), false, 1.0);
        TSharedPtr<FBackgroundTask> Second = Tasks.Begin(TEXT("Second"), false, 2.0);

        const TArray<TSharedPtr<FBackgroundTask>> Visible = Tasks.GetVisibleTasks(5.0);
        TEST_EXPECT(Visible.Size() == 2 && Visible[0] == First && Visible[1] == Second);
    }

    TEST_SECTION(TEXT("Progress is unknown until reported and clamped to 0..1"));
    {
        FBackgroundTasks Tasks;

        TSharedPtr<FBackgroundTask> Task = Tasks.Begin(TEXT("Importing"), false, 0.0);
        TEST_EXPECT(Task->GetProgress() == FBackgroundTask::UnknownProgress);

        Task->SetProgress(0.5f);
        TEST_EXPECT(Task->GetProgress() == 0.5f);

        Task->SetProgress(3.0f);
        TEST_EXPECT(Task->GetProgress() == 1.0f);
    }

    TEST_SECTION(TEXT("Only a task that can be cancelled takes a cancel request"));
    {
        FBackgroundTasks Tasks;

        TSharedPtr<FBackgroundTask> Cancellable = Tasks.Begin(TEXT("Cancellable"), true, 0.0);
        TSharedPtr<FBackgroundTask> Fixed       = Tasks.Begin(TEXT("Fixed"), false, 0.0);

        Cancellable->RequestCancel();
        Fixed->RequestCancel();

        TEST_EXPECT(Cancellable->IsCancelRequested() && Cancellable->CanCancel());
        TEST_EXPECT(!Fixed->IsCancelRequested() && !Fixed->CanCancel());
    }

    TEST_END();
}
