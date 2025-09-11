#include "Engine/RuntimeEngine.h"
#include "Engine/EngineUI/InGameConsoleWidget.h"
#include "Engine/EngineUI/FrameProfilerWidget.h"
#include "Engine/EngineUI/SceneInspectorWidget.h"

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
