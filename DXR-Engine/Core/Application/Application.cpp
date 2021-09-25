#include "Application.h"

#include "Scene/Scene.h"

Application* GApplication;

Application::~Application()
{
    SafeDelete( CurrentScene );
}

bool Application::Init()
{
    return true;
}

void Application::Tick( CTimestamp Deltatime )
{
    UNREFERENCED_VARIABLE( Deltatime );
}

bool Application::Release()
{
    return true;
}
