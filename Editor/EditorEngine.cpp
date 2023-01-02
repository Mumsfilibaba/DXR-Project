#include "EditorEngine.h"

#include "Renderer/Renderer.h"

#include "ViewportRenderer/ViewportRenderer.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Components/MeshComponent.h"

#include "Core/Math/Math.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Debug/Console/ConsoleManager.h"

FEditorEngine* FEditorEngine::Make()
{
    return dbg_new FEditorEngine();
}

bool FEditorEngine::Init()
{
    if ( !FEngine::Init() )
    {
        return false;
    }

    /* Create Editor windows */
    FApplication& Application = FApplication::Get();

    TSharedRef<CInspectorWindow> InspectorWindow = CInspectorWindow::Make();
    Application.AddWindow( InspectorWindow );

    TSharedRef<CEditorMenuWidget> MenuBar = dbg_new CEditorMenuWidget();
    Application.AddWindow( MenuBar );

    return true;
}

void FEditorEngine::Tick( FTimespan Deltatime )
{
    FEngine::Tick( Deltatime );
}
