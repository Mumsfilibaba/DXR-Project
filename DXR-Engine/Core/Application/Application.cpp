#include "Application.h"

#include "Scene/Scene.h"

Application* GApplication;

Application::~Application()
{
    SafeDelete( Scene );
}

bool Application::Init()
{
    return true;
}

void Application::Tick( Timestamp Deltatime )
{
    UNREFERENCED_VARIABLE( Deltatime );
}

bool Application::Release()
{
    return true;
}
