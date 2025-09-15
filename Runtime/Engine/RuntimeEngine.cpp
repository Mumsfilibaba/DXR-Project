#include "Engine/RuntimeEngine.h"
#include "Engine/EngineUI/InGameConsoleWidget.h"
#include "Engine/EngineUI/FrameProfilerWidget.h"
#include "Engine/EngineUI/SceneInspectorWidget.h"
#include "RendererCore/Interfaces/IRendererModule.h"

FRuntimeEngine::FRuntimeEngine()
    : FEngine()
	, ConsoleWidget(nullptr)
	, ProfilerWidget(nullptr)
	, InspectorWidget(nullptr)
{
}

FRuntimeEngine::~FRuntimeEngine()
{
}

bool FRuntimeEngine::Init()
{
    if (!FEngine::Init())
    {
        return false;
    }

	if (IImguiPlugin::IsEnabled())
	{
		ProfilerWidget  = MakeSharedPtr<FFrameProfilerWidget>();
		ConsoleWidget   = MakeSharedPtr<FInGameConsoleWidget>();
		InspectorWidget = MakeSharedPtr<FSceneInspectorWidget>();
	}

	return true;
}

void FRuntimeEngine::Release()
{
	if (IImguiPlugin::IsEnabled())
	{
		ProfilerWidget.Reset();
		ConsoleWidget.Reset();
		InspectorWidget.Reset();
	}

	FEngine::Release();
}

void FRuntimeEngine::RenderFrame()
{
	TRACE_FUNCTION_SCOPE();

	// Render directly to the BackBuffer
	FSceneRenderView RenderView;
	RenderView.Scene        = GetWorld()->GetSceneInterface();
	RenderView.RenderTarget = GetSceneViewport()->GetRHISwapChain()->GetBackBuffer();

	IRendererModule* RendererModule = IRendererModule::Get();
	RendererModule->RenderSceneView(RenderView);

	// Render the rest
	FEngine::RenderFrame();
}
