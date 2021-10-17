#include "ApplicationModule.h"

#include "Scene/Scene.h"

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

bool CApplicationModule::Load()
{
    return true;
}

bool CApplicationModule::Unload()
{
    return true;
}

const char* CApplicationModule::GetName()
{
    return "ApplicationModule";
}
