#include <Core/Application/Application.h>

#include <Scene/Camera.h>

class Sandbox : public Application
{
public:
    virtual bool Init() override;

	virtual void Tick( CTimestamp DeltaTime ) override;

private:
    Camera* CurrentCamera = nullptr;
    CVector3 CameraSpeed;
};
