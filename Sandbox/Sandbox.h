#include <Core/Core.h>
#include <Core/Modules/ApplicationModule.h>

#include <Engine/Scene/Camera.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSandbox

class FSandbox : public FApplicationModule
{
public:

    FSandbox()  = default;
    ~FSandbox() = default;

    virtual bool Init() override;

    virtual void Tick(FTimestamp DeltaTime) override;

private:
    CCamera* CurrentCamera = nullptr;
    FVector3 CameraSpeed;
};
