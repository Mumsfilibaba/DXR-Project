#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/DockspaceWidget.h"

FEditorEngine::FEditorEngine()
    : FEngine()
    , DockspaceWidget(nullptr)
{
}

FEditorEngine::~FEditorEngine()
{
}

bool FEditorEngine::Init()
{
    if (!FEngine::Init())
    {
        return false;
    }

	if (IImguiPlugin::IsEnabled())
	{
		DockspaceWidget = MakeSharedPtr<FDockspaceWidget>();
	}

	return true;
}

void FEditorEngine::Release()
{
	if (IImguiPlugin::IsEnabled())
	{
		DockspaceWidget.Reset();
	}

	FEngine::Release();
}
