#pragma once
#include "Core/Delegates/MulticastDelegate.h"
#include "Engine/Assets/PrimitiveShapes.h"
#include "Engine/World/Components/StaticMeshComponent.h"

DECLARE_MULTICAST_DELEGATE(FOnPrimitiveShapeMeshChanged);

class ENGINE_API FPrimitiveShapeComponent : public FStaticMeshComponent
{
public:
    FOBJECT_DECLARE_CLASS(FPrimitiveShapeComponent, FStaticMeshComponent);

    FPrimitiveShapeComponent(const FObjectInitializer& ObjectInitializer);
    ~FPrimitiveShapeComponent();

    /**
     * @brief Set the shape and its parameters. The mesh changes now when it is quick to build, and otherwise once it
     * is built in the background, keeping the current mesh until then. A newer shape replaces a build still running.
     */
    void SetShape(const FPrimitiveShape& InShape);

    /** @return Returns the shape asked for last, which the mesh shows once IsBuilding is false */
    FORCEINLINE const FPrimitiveShape& GetShape() const
    {
        return Shape;
    }

    FORCEINLINE bool IsBuilding() const
    {
        return PendingRequest != 0;
    }

    /** @brief Broadcast on the main thread after the mesh changes, and when a build is cancelled */
    FORCEINLINE FOnPrimitiveShapeMeshChanged& GetOnMeshChanged()
    {
        return OnMeshChanged;
    }

private:
    void OnMeshBuilt(const TSharedPtr<FMesh>& InMesh, FPrimitiveShape InShape);
    void ApplyMesh(const TSharedPtr<FMesh>& InMesh);

    FPrimitiveShape              Shape;
    FPrimitiveShape              BuiltShape;
    uint64                       PendingRequest;
    FOnPrimitiveShapeMeshChanged OnMeshChanged;
};
