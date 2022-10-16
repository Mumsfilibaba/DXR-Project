#include <Core/Core.h>
#include <Core/Modules/ModuleInterface.h>

#include <Engine/Scene/Camera.h>

class FSandbox 
    : public FApplicationModule
{
public:
    FSandbox()  = default;
    ~FSandbox() = default;

    virtual bool Init() override;

    virtual void Tick(FTimespan DeltaTime) override;

private:
    FCamera* CurrentCamera = nullptr;
    FVector3 CameraSpeed;
};
