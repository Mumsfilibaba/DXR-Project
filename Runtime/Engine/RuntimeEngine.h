#pragma once
#include "Engine/Engine.h"

class ENGINE_API FRuntimeEngine : public FEngine
{
public:
    FRuntimeEngine();
    virtual ~FRuntimeEngine();

    // FEngine Interface
    virtual bool Init() override final;
    virtual void Release() override final;
    virtual void RenderFrame() override final;

private:
    TSharedPtr<class FRuntimeConsoleWidget> ConsoleWidget;
    TSharedPtr<class FFrameProfilerWidget>  ProfilerWidget;
};