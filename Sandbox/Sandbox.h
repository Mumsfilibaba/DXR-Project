#pragma once
#include <Core/Core.h>
#include <Core/Modules/ModuleManager.h>
#include <Engine/World/Camera.h>

class SANDBOX_API FSandbox : public FGameModule
{
public:
    FSandbox()  = default;
    ~FSandbox() = default;

    virtual bool Init() override;

    virtual void Tick(FTimespan DeltaTime) override;
};
