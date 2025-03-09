#pragma once
#include "Core/Math/Matrix4.h"
#include "RHI/RHITexture.h"
#include "Engine/World/Lights/Light.h"

class ENGINE_API FSkyLight : public FLight
{
public:
    FOBJECT_DECLARE_CLASS(FSkyLight, FLight);

    FSkyLight(const FObjectInitializer& ObjectInitializer);
    ~FSkyLight() = default;

    void SetCubeMap(const FRHITextureRef& InCubeMap);

    FORCEINLINE const FRHITextureRef& GetCubeMap() const
    {
        return CubeMap;
    } 

private:
    FRHITextureRef CubeMap;
};
