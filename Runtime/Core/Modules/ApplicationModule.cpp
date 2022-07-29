#include "ApplicationModule.h"

#include "Core/Misc/EngineLoopTicker.h"

CORE_API FApplicationModule* GApplicationModule;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationModule

bool FApplicationModule::Init()
{
    FTickDelegate TickDelegate = FTickDelegate::CreateRaw(this, &FApplicationModule::Tick);
    TickHandle = TickDelegate.GetHandle();

    FEngineLoopTicker::Get().AddDelegate(TickDelegate);

    return true;
}

void FApplicationModule::Tick(FTimestamp)
{
}

bool FApplicationModule::Release()
{
    FEngineLoopTicker::Get().RemoveDelegate(TickHandle);
    return true;
}

// TODO: Remove init and release? 
bool FApplicationModule::Load()
{
    return Init();
}

bool FApplicationModule::Unload()
{
    return Release();
}
