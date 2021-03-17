#include <Game/Game.h>

class Sandbox : public Game
{
public:
    Sandbox()   = default;
    ~Sandbox()  = default;

    virtual bool Init() override;

    virtual void Tick(Timestamp DeltaTime) override;

private:
    XMFLOAT3 CameraSpeed = XMFLOAT3(0.0f, 0.0f, 0.0f);
};