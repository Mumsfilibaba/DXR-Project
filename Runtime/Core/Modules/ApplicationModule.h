#pragma once
#include "Core/Core.h"
#include "Core/Time/Timespan.h"
#include "Core/Modules/ModuleInterface.h"
#include "Core/Delegates/DelegateInstance.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationInterfaceModule

class CORE_API FApplicationInterfaceModule 
    : public IModule
{
public:
    FApplicationInterfaceModule() = default;
    virtual ~FApplicationInterfaceModule() = default;

    /** @return: Returns true if the initialization is successful */
    virtual bool Init();

    /**
     * @brief: Tick the application module 
     * 
     * @param DeltaTime: Time since last time the application was ticked
     */
    virtual void Tick(FTimespan Deltatime);

    /** @return: Returns true if the release is successful */
    virtual bool Release();

    /** @return: Returns true if the load is successful */
    virtual bool Load() override;

    /** @return: Returns true if the unload is successful */
    virtual bool Unload() override;

protected:
    FDelegateHandle TickHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global application pointer

extern CORE_API FApplicationInterfaceModule* GApplicationModule;