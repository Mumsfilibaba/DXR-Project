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

    /* Init the application module */
    virtual bool Init();

    /* Tick the application module */
    virtual void Tick(CTimestamp Deltatime);

    /* Release the application module */
    virtual bool Release();

    /* Called when the module is first loaded into the application */
    virtual bool Load() override;

    /* Called before the module is unloaded by the application */
    virtual bool Unload() override;

protected:
    CDelegateHandle TickHandle;
};

extern CORE_API CApplicationModule* GApplicationModule;