#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FDirectionalLight;
enum class ECascadeSplitMode : uint8;

class FSceneDirectionalLight : public FSceneObject
{
public:
    FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight);
    ~FSceneDirectionalLight();

    // FSceneObject Interface
    virtual void Tick() override final;

    FSceneView&       GetShadowView()       { return ShadowView; }
    const FSceneView& GetShadowView() const { return ShadowView; }

    const FVector3& GetColor()           const { return Color; }
    const FVector3& GetDirectionVector() const { return Direction; }
    const FVector3& GetUpVector()        const { return UpVector; }
    const FMatrix4& GetShadowMatrix()    const { return ShadowMatrix; }

    float GetShadowNearPlane()      const { return ShadowNearPlane; }
    float GetShadowFarPlane()       const { return ShadowFarPlane; }
    float GetShadowBias()           const { return ShadowBias; }
    float GetShadowPositionOffset() const { return ShadowPositionOffset; }
    float GetCascadeSplitLambda()   const { return CascadeSplitLambda; }
    ECascadeSplitMode GetCascadeSplitMode() const { return CascadeSplitMode; }
    float GetManualCascadeSplitDistance(int32 Index) const
    {
        return (Index >= 0 && Index < 3) ? ManualCascadeSplitDistances[Index] : 0.0f;
    }
    float GetLightArea()            const { return LightArea; }

private:

    // Pointer to the light in the world
    FDirectionalLight* DirectionalLight;

    // View for shadow rendering
    FSceneView ShadowView;

    // Light properties
    FVector3 Color;
    FVector3 Direction;
    FVector3 UpVector;
    FMatrix4 ShadowMatrix;
    float    ShadowNearPlane;
    float    ShadowFarPlane;
    float    ShadowBias;
    float    ShadowPositionOffset;
    float    CascadeSplitLambda;
    ECascadeSplitMode CascadeSplitMode;
    float    ManualCascadeSplitDistances[3];
    float    LightArea;
};
