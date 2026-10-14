#pragma once
#include "Core/Containers/Map.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Tasks/TaskHandle.h"
#include "Engine/Assets/PrimitiveShapes.h"
#include "Engine/Core/BackgroundTasks.h"
#include "Engine/Resources/Model.h"

DECLARE_DELEGATE(FOnShapeMeshBuilt, const TSharedPtr<FMesh>& /*Mesh, or nullptr when the build was cancelled or failed*/);

class ENGINE_API FPrimitiveShapeMeshCache : public FNonCopyable
{
public:

    /** @brief Shapes estimated at more triangles than this are built on a worker thread as a background task */
    static constexpr int64 BackgroundTriangles = 50000;

    static FPrimitiveShapeMeshCache& Get();

public:
    FPrimitiveShapeMeshCache();
    ~FPrimitiveShapeMeshCache();

    /**
     * @brief Get the mesh of a shape. It is returned now when a component already uses it or it is quick to build;
     * otherwise it is built in the background, nullptr is returned, and OnBuilt gets it on the main thread once built.
     * Identical shapes share one mesh, released when nothing uses it.
     *
     * @param OutRequest Set to the background build's request, for Cancel, or to 0 when the mesh is returned now
     */
    TSharedPtr<FMesh> Request(const FPrimitiveShape& Shape, const FOnShapeMeshBuilt& OnBuilt, uint64& OutRequest);

    /** @brief Forget a request, so its OnBuilt is never called; a build nothing waits for any more is cancelled */
    void Cancel(uint64 Request);

    /** @brief Hand finished builds to their requests, on the main thread */
    void Tick();

    /** @brief Wait for the builds and drop every mesh, before the RHI is released */
    void Release();

    /** @return Returns how many shapes have a mesh alive, for tests and stats */
    NODISCARD int32 GetNumMeshes() const;

private:
    struct FRequest
    {
        uint64            Id;
        FOnShapeMeshBuilt OnBuilt;
    };

    struct FBuild
    {
        FPrimitiveShape             Shape;
        TSharedPtr<FBackgroundTask> Task;
        FTaskHandle                 Handle;
        FMeshData                   Result;
        TArray<FRequest>            Requests;
    };

    TSharedPtr<FMesh> CreateMesh(const FPrimitiveShape& Shape, const FMeshData& MeshData);

    TMap<FPrimitiveShape, TWeakPtr<FMesh>> Meshes;
    TArray<TSharedPtr<FBuild>>             Builds;
    uint64                                 NextRequest;
};
