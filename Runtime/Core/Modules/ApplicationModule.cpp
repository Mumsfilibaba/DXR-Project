#include "ApplicationModule.h"

#include "Core/Misc/EngineLoopTicker.h"

CORE_API FApplicationInterfaceModule* GApplicationModule;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationInterfaceModule

bool FApplicationInterfaceModule::Init()
{
    FTickDelegate TickDelegate = FTickDelegate::CreateRaw(this, &FApplicationInterfaceModule::Tick);
    TickHandle = TickDelegate.GetHandle();

    FEngineLoopTicker::Get().AddDelegate(TickDelegate);

    return true;
}

void FApplicationInterfaceModule::Tick(FTimespan)
{
}

bool FApplicationInterfaceModule::Release()
{
    FEngineLoopTicker::Get().RemoveDelegate(TickHandle);
    return true;
}

// TODO: Remove init and release? 
bool FApplicationInterfaceModule::Load()
{
    return Init();
}

bool FApplicationInterfaceModule::Unload()
{
    return Release();
}
