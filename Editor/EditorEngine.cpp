#include "EditorEngine.h"

#include "Renderer/UIRenderer.h"
#include "Renderer/Renderer.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Components/MeshComponent.h"

#include "Core/Math/Math.h"
#include "Core/Application/Application.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Debug/Console/ConsoleManager.h"

CEditorEngine* CEditorEngine::Make()
{
    return dbg_new CEditorEngine();
}

bool CEditorEngine::Init()
{
    if (!CEngine::Init())
    {
        return false;
    }

    /* Create Editor windows */
    InspectorWindow = dbg_new CInspectorWindow();
    MenuBar = dbg_new CEditorMenuWidget();

    CApplication& Application = CApplication::Get();
    Application.AddWindow( InspectorWindow );
    Application.AddWindow( MenuBar );

    return true;
}

void CEditorEngine::Tick( CTimestamp Deltatime )
{
    CEngine::Tick( Deltatime );
}
