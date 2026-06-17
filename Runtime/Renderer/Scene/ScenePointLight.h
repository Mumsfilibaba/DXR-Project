#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FPointLight;

struct FScenePointLight : public FSceneObject
{
    struct FShadowData
    {
        Matrix4 ViewProjMatrix;
        Vector3 Position;
        float   NearPlane;
        float   FarPlane;
    };

    FScenePointLight(FScene* InScene, FPointLight* InPointLight);
    ~FScenePointLight();

    virtual void Tick() override final;

    FPointLight* PointLight;                     // Pointer to the light in the world
    FShadowData  ShadowData[RHI_NUM_CUBE_FACES]; // Shadow generation information
    FSceneView   ShadowView[RHI_NUM_CUBE_FACES]; // Store data for each face
    FSceneView   SinglePassShadowView;           // Store data for a single pass cube-map
    Vector3      Position;
    Vector3      Color;
    float        ShadowBias;
};
