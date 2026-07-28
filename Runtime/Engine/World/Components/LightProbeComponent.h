#pragma once
#include "Core/Math/Vector3.h"
#include "RHI/RHITexture.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FLightProbeComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbeComponent, FSceneComponent);

    FLightProbeComponent(const FObjectInitializer& ObjectInitializer);
    ~FLightProbeComponent();

    void SetBoxExtent(const Vector3& InBoxExtent);
    void SetBoxOffset(const Vector3& InBoxOffset);
    void SetBoxProjection(bool bInBoxProjection);
    void SetCubeMap(const FRHITextureRef& InCubeMap);

    const Vector3& GetPosition() const;

    const Vector3& GetBoxOffset() const
    {
        return BoxOffset;
    }

    const Vector3& GetBoxExtents() const
    {
        return BoxExtent;
    }

    bool GetBoxProjection() const
    {
        return bBoxProjection;
    }

    const FRHITextureRef& GetCubeMap() const
    {
        return CubeMap;
    }

private:
    FRHITextureRef CubeMap;
    Vector3        BoxOffset;
    Vector3        BoxExtent;
    bool           bBoxProjection;
};
