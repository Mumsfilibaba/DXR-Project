#pragma once
#include "Core/Core.h"
#include "Core/Time/Timestamp.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Delegates/DelegateInstance.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Application-Module interface

class CORE_API CApplicationModule : public IEngineModule
{
public:

    CApplicationModule() = default;
    virtual ~CApplicationModule() = default;

    /**
     * Initialize the application module 
     * 
     * @return: Returns true if the initialization is successful
     */
    virtual bool Init();

    /**
     * Tick the application module 
     * 
     * @param DeltaTime: Time since last time the application was ticked
     */
    virtual void Tick(CTimestamp Deltatime);

    /**
     * Release the application module
     *
     * @return: Returns true if the release is successful
     */
    virtual bool Release();

    /**
     * Called when the module is first loaded into the application
     *
     * @return: Returns true if the load is successful
     */
    virtual bool Load() override;

    /**
     * Called before the module is unloaded by the application
     *
     * @return: Returns true if the unload is successful
     */
    virtual bool Unload() override;

protected:
    CDelegateHandle TickHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global application pointer

extern CORE_API CApplicationModule* GApplicationModule;