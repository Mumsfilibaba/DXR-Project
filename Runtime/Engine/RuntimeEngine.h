#pragma once
#include "Engine/Engine.h"

class ENGINE_API FRuntimeEngine : public FEngine
{
public:
    FRuntimeEngine();
    virtual ~FRuntimeEngine();

    virtual bool Init() override;
	virtual void Release() override;

private:
	TSharedPtr<class FInGameConsoleWidget>  ConsoleWidget;
	TSharedPtr<class FFrameProfilerWidget>  ProfilerWidget;
	TSharedPtr<class FSceneInspectorWidget> InspectorWidget;
};