#pragma once
#include "Time/Timestamp.h"

extern class Application* CreateApplication();

class Application
{
public:
    virtual ~Application();

    virtual bool Init();

    virtual void Tick( Timestamp Deltatime );

    virtual bool Release();

    class Scene* Scene = nullptr;
};

extern Application* GApplication;