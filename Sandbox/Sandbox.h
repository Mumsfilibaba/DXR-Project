#pragma once
#include <Core/Core.h>
#include <Core/Modules/ModuleManager.h>
#include <Core/Containers/SharedPtr.h>
#include <Engine/Resources/Environment.h>

class FWorld;

class SANDBOX_API FSandbox : public FGameModule
{
public:
    FSandbox();
    ~FSandbox();

    virtual bool Init() override;
    virtual void Release() override;

    virtual void Tick(float DeltaTime) override;

private:
    bool CreateSponza(FWorld* InWorld);
    bool CreateBistro(FWorld* InWorld);
    bool CreateSunTemple(FWorld* InWorld);
    bool CreateEmeraldSquare(FWorld* InWorld);
    bool CreateLightSandbox(FWorld* InWorld);

    TSharedPtr<FEnvironment> LoadEnvironmentFromPanorama(const FString& Filename);
};
