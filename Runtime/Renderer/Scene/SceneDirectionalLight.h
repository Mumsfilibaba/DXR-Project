#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FDirectionalLight;

class FSceneDirectionalLight : public FSceneObject
{
public:
    FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight);
    ~FSceneDirectionalLight();

    // FSceneObject Interface
    virtual void Tick() override final;

    FSceneView&       GetShadowView()       { return ShadowView; }
    const FSceneView& GetShadowView() const { return ShadowView; }

    const Vector3& GetColor()           const { return Color; }
    const Vector3& GetDirectionVector() const { return Direction; }
    const Vector3& GetUpVector()        const { return UpVector; }
    const Matrix4& GetShadowMatrix()    const { return ShadowMatrix; }

    float GetShadowNearPlane()      const { return ShadowNearPlane; }
    float GetShadowFarPlane()       const { return ShadowFarPlane; }
    float GetShadowBias()           const { return ShadowBias; }
    float GetShadowPositionOffset() const { return ShadowPositionOffset; }
    float GetCascadeSplitLambda()   const { return CascadeSplitLambda; }
    float GetLightArea()            const { return LightArea; }

private:
    FDirectionalLight* DirectionalLight; // Pointer to the light in the world
    FSceneView         ShadowView;       // View for shadow rendering
    Vector3            Color;
    Vector3            Direction;
    Vector3            UpVector;
    Matrix4            ShadowMatrix;
    float              ShadowNearPlane;
    float              ShadowFarPlane;
    float              ShadowBias;
    float              ShadowPositionOffset;
    float              CascadeSplitLambda;
    float              LightArea;
};
