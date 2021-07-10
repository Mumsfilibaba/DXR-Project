#include <Core/Application/Application.h>

#include <Scene/Camera.h>

class Sandbox : public Application
{
public:
    virtual bool Init() override;

    virtual void Tick( Timestamp DeltaTime ) override;

private:
    Camera* CurrentCamera = nullptr;
    XMFLOAT3 CameraSpeed = XMFLOAT3( 0.0f, 0.0f, 0.0f );
};