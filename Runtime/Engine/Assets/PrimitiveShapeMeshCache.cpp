#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Tasks/Tasks.h"
#include "Engine/Assets/PrimitiveShapeMeshCache.h"

FPrimitiveShapeMeshCache& FPrimitiveShapeMeshCache::Get()
{
    static FPrimitiveShapeMeshCache Instance;
    return Instance;
}

FPrimitiveShapeMeshCache::FPrimitiveShapeMeshCache()
    : Meshes()
    , Builds()
    , NextRequest(1)
{
}

FPrimitiveShapeMeshCache::~FPrimitiveShapeMeshCache() = default;

TSharedPtr<FMesh> FPrimitiveShapeMeshCache::Request(const FPrimitiveShape& InShape, const FOnShapeMeshBuilt& OnBuilt, uint64& OutRequest)
{
    const FPrimitiveShape Shape = PrimitiveShapes::Clamp(InShape);
    OutRequest = 0;

    if (TWeakPtr<FMesh>* Existing = Meshes.Find(Shape))
    {
        if (TSharedPtr<FMesh> Mesh = Existing->ToSharedPtr())
        {
            return Mesh;
        }
    }

    if (PrimitiveShapes::EstimateTriangles(Shape) <= BackgroundTriangles)
    {
        return CreateMesh(Shape, PrimitiveShapes::BuildMeshData(Shape));
    }

    OutRequest = NextRequest++;

    for (const TSharedPtr<FBuild>& Build : Builds)
    {
        if (Build->Shape == Shape && !Build->Task->IsCancelRequested())
        {
            Build->Requests.Add({ OutRequest, OnBuilt });
            return nullptr;
        }
    }

    TSharedPtr<FBuild> Build = MakeSharedPtr<FBuild>();
    Build->Shape = Shape;
    Build->Task  = FBackgroundTasks::Get().Begin(FString::Printf(TEXT("Building %s"), *PrimitiveShapes::Describe(Shape)), true);
    Build->Requests.Add({ OutRequest, OnBuilt });

    Build->Handle = Tasks::Async([Build]()
    {
        if (!Build->Task->IsCancelRequested())
        {
            FMeshData MeshData = PrimitiveShapes::BuildMeshData(Build->Shape);
            if (!Build->Task->IsCancelRequested() && MeshData.PackVertexStreams(MeshData.PackedStreams))
            {
                MeshData.PackedVertexCount = MeshData.Vertices.Size();
                Build->Result = ::Move(MeshData);
            }
        }
    });

    Builds.Add(Build);
    return nullptr;
}

void FPrimitiveShapeMeshCache::Cancel(uint64 Request)
{
    for (const TSharedPtr<FBuild>& Build : Builds)
    {
        for (int32 Index = 0; Index < Build->Requests.Size(); ++Index)
        {
            if (Build->Requests[Index].Id == Request)
            {
                Build->Requests.RemoveAt(Index);
                if (Build->Requests.IsEmpty())
                {
                    Build->Task->RequestCancel();
                }

                return;
            }
        }
    }
}

void FPrimitiveShapeMeshCache::Tick()
{
    for (int32 Index = 0; Index < Builds.Size();)
    {
        const TSharedPtr<FBuild> Build = Builds[Index];
        if (!Build->Handle.IsComplete())
        {
            ++Index;
            continue;
        }

        Builds.RemoveAt(Index);
        Build->Task->Finish();

        TSharedPtr<FMesh> Mesh;
        if (!Build->Task->IsCancelRequested())
        {
            Mesh = CreateMesh(Build->Shape, Build->Result);
        }

        for (const FRequest& Request : Build->Requests)
        {
            Request.OnBuilt.ExecuteIfBound(Mesh);
        }
    }

    TArray<FPrimitiveShape> Expired;
    for (const TPair<const FPrimitiveShape&, TWeakPtr<FMesh>&>& Pair : Meshes)
    {
        if (Pair.Second.IsExpired())
        {
            Expired.Add(Pair.First);
        }
    }

    for (const FPrimitiveShape& Shape : Expired)
    {
        Meshes.Remove(Shape);
    }
}

void FPrimitiveShapeMeshCache::Release()
{
    for (const TSharedPtr<FBuild>& Build : Builds)
    {
        Build->Task->RequestCancel();
        Build->Handle.Wait();
        Build->Task->Finish();
    }

    Builds.Clear();
    Meshes.Clear();
}

int32 FPrimitiveShapeMeshCache::GetNumMeshes() const
{
    int32 NumMeshes = 0;
    for (const TPair<const FPrimitiveShape&, const TWeakPtr<FMesh>&>& Pair : Meshes)
    {
        NumMeshes += Pair.Second.IsExpired() ? 0 : 1;
    }

    return NumMeshes;
}

TSharedPtr<FMesh> FPrimitiveShapeMeshCache::CreateMesh(const FPrimitiveShape& Shape, const FMeshData& MeshData)
{
    if (MeshData.Indices.IsEmpty() || (MeshData.Vertices.IsEmpty() && MeshData.PackedVertexCount == 0))
    {
        LOG_ERROR("[FPrimitiveShapeMeshCache]: %s built no geometry", *PrimitiveShapes::Describe(Shape));
        return nullptr;
    }

    TSharedPtr<FMesh> Mesh = FMesh::Create(MeshData);
    if (Mesh)
    {
        Meshes.Add(Shape, TWeakPtr<FMesh>(Mesh));
    }

    return Mesh;
}
