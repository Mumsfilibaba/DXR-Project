#include "SandboxCore.h"

#include <Core/Application/ApplicationModule.h>

#include <Scene/Camera.h>

class SANDBOX_API CSandbox : public CApplicationModule
{
public:

    CSandbox() = default;
    ~CSandbox() = default;

    virtual bool Init() override;

    virtual void Tick( CTimestamp DeltaTime ) override;

private:
    CCamera* CurrentCamera = nullptr;
    CVector3 CameraSpeed;
};
