#include "ApplicationModule.h"

#include "Core/Misc/EngineLoopTicker.h"

CORE_API CApplicationModule* GApplicationModule;

///////////////////////////////////////////////////////////////////////////////////////////////////

bool CApplicationModule::Init()
{
    // Bind the application module to the EngineLoopTicker
    CTickDelegate TickDelegate = CTickDelegate::CreateRaw( this, &CApplicationModule::Tick );
    TickHandle = TickDelegate.GetHandle();
    
    CEngineLoopTicker::Get().AddElement( TickDelegate );

    return true;
}

void CApplicationModule::Tick( CTimestamp )
{
}

bool CApplicationModule::Release()
{
    CEngineLoopTicker::Get().RemoveElement( TickHandle );   
    return true;
}

// TODO: Remove init and release? 
bool CApplicationModule::Load()
{
    return Init();
}

bool CApplicationModule::Unload()
{
    return Release();
}

const char* CApplicationModule::GetName() const
{
    return "ApplicationModule";
}