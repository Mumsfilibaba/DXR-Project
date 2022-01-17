#include <Core/Core.h>
#include <Core/Modules/ApplicationModule.h>

#include <Engine/Scene/Camera.h>

class CSandbox : public CApplicationModule
{
public:

    CSandbox()  = default;
    ~CSandbox() = default;

    virtual bool Init() override;

    virtual void Tick(CTimestamp DeltaTime) override;

private:
    CCamera* CurrentCamera = nullptr;
    CVector3 CameraSpeed;
};
