#pragma once
#include <Core/Core.h>
#include <Core/Modules/ModuleManager.h>
#include <Engine/World/Camera.h>
#include <RHI/RHITexture.h>

class FWorld;

class SANDBOX_API FSandbox : public FGameModule
{
public:
    FSandbox();
    ~FSandbox();

    virtual bool Init() override;

    virtual void Tick(float DeltaTime) override;

private:
    bool CreateSponza(FWorld* InWorld);
    bool CreateBistro(FWorld* InWorld);
    bool CreateSunTemple(FWorld* InWorld);
    bool CreateEmeraldSquare(FWorld* InWorld);
    bool CreateLightDemo(FWorld* InWorld);

    FRHITextureRef LoadSkyboxFromPanorama(const FString& Filename);
};
