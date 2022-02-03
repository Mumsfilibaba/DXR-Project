#include "EditorEngine.h"

#include "Renderer/Renderer.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Components/MeshComponent.h"

#include "Core/Math/Math.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Interface/InterfaceApplication.h"

CEditorEngine* CEditorEngine::Make()
{
    return dbg_new CEditorEngine();
}

bool CEditorEngine::Init()
{
    if ( !CEngine::Init() )
    {
        return false;
    }

    /* Create Editor windows */
    CApplicationInstance& Application = CApplicationInstance::Get();

    TSharedRef<CInspectorWindow> InspectorWindow = CInspectorWindow::Make();
    Application.AddWindow( InspectorWindow );

    TSharedRef<CEditorMenuWidget> MenuBar = dbg_new CEditorMenuWidget();
    Application.AddWindow( MenuBar );

    return true;
}

void CEditorEngine::Tick( CTimestamp Deltatime )
{
    CEngine::Tick( Deltatime );
}
