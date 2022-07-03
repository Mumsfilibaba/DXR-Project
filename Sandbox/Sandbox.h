#include <Core/Core.h>
#include <Core/Modules/ApplicationModule.h>

#include <Engine/Scene/Camera.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CSandbox

class CSandbox : public FApplicationModule
{
public:

    CSandbox()  = default;
    ~CSandbox() = default;

    virtual bool Init() override;

    virtual void Tick(FTimestamp DeltaTime) override;

private:
    CCamera* CurrentCamera = nullptr;
    FVector3 CameraSpeed;
};
