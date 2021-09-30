#pragma once
#include "Core/Time/Timestamp.h"

extern class CApplicationModule* CreateApplicationModule();

class CApplicationModule
{
public:

    CApplicationModule() = default;
    virtual ~CApplicationModule() = default;

    /* Init the application module */
    virtual bool Init();

    /* Tick the application module */
    virtual void Tick( CTimestamp Deltatime );

    /* Release the application module */
    virtual bool Release();
};

extern CApplicationModule* GApplicationModule;