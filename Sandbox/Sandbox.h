#include <Core/Application/ApplicationModule.h>

#include <Scene/Camera.h>

class CSandbox : public CApplicationModule
{
public:

    CSandbox() = default;
    ~CSandbox() = default;

    virtual bool Init() override;

    virtual void Tick( CTimestamp DeltaTime ) override;

private:
    Camera* CurrentCamera = nullptr;
    CVector3 CameraSpeed;
};
