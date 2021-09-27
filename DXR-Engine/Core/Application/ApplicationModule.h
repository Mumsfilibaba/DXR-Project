#pragma once
#include "Core/Time/Timestamp.h"

extern class CApplicationModule* CreateApplicationModule();

class CApplicationModule
{
public:

    CApplicationModule() = default;
    virtual ~CApplicationModule();

    virtual bool Init();

    virtual void Tick( CTimestamp Deltatime );

    virtual bool Release();

    class Scene* CurrentScene = nullptr;
};

extern CApplicationModule* GApplicationModule;