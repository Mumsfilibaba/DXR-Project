#include "ApplicationModule.h"

CORE_API CApplicationModule* GApplicationModule;

bool CApplicationModule::Init()
{
    return true;
}

void CApplicationModule::Tick( CTimestamp Deltatime )
{
    UNREFERENCED_VARIABLE( Deltatime );
}

bool CApplicationModule::Release()
{
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

const char* CApplicationModule::GetName()
{
    return "ApplicationModule";
}
