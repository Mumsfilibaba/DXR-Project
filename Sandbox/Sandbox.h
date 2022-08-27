#include <Core/Core.h>
#include <Core/Modules/ApplicationModule.h>

#include <Engine/Scene/Camera.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSandbox

class FSandbox 
    : public FApplicationInterfaceModule
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
