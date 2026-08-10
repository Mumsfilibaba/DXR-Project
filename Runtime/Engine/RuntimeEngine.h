#pragma once
#include "Engine/Engine.h"

class FCameraComponent;

class ENGINE_API FRuntimeEngine : public FEngine
{
public:
    FRuntimeEngine();
    virtual ~FRuntimeEngine();

    // FEngine Interface
    virtual bool Init() override final;
    virtual bool Start() override final;
    virtual void Release() override final;

    virtual FSceneRenderPacket BuildRenderPacket() override final;

private:
    FCameraComponent*                            LastRenderCamera;
    TSharedPtr<class FRuntimeConsoleWidget>      ConsoleWidget;
    TSharedPtr<class FEditorFrameProfilerWidget> ProfilerWidget;
};