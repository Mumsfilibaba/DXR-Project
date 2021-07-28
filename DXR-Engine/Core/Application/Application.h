#pragma once
#include "Core/Time/Timestamp.h"

extern class Application* CreateApplication();

class Application
{
public:
    virtual ~Application();

    virtual bool Init();

    virtual void Tick( Timestamp Deltatime );

    virtual bool Release();

    class Scene* CurrentScene = nullptr;
};

extern Application* GApplication;