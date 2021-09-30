#include "ApplicationModule.h"

#include "Scene/Scene.h"

CApplicationModule* GApplicationModule;

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
