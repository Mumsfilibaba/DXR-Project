#include "SandboxAPI.h"

#include <Core/Modules/ApplicationModule.h>

#include <Engine/Scene/Camera.h>

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'
#endif

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

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif