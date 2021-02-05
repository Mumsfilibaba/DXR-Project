#pragma once
#include "BaseLight.h"

#include "Core/TSharedRef.h"

#include "RenderLayer/Texture.h"

class Skylight : public BaseLight
{
    CORE_OBJECT(Skylight, BaseLight);

public:
    Skylight()
        : Skybox(nullptr)
        , Intensisty(0.0f)
    {
        CORE_OBJECT_INIT();
    }

    ~Skylight() = default;

    void SetSkybox(const TSharedRef<TextureCube>& InSkybox) { Skybox = InSkybox; }
    TSharedRef<TextureCube> GetSkybox() const { return Skybox; }

    void SetIntensity(Float InIntensisty) { Intensisty = InIntensisty; }
    Float GetIntensity() const { return Intensisty; }

private:
    TSharedRef<TextureCube> Skybox;
    Float Intensisty = 0.0f;
};