#pragma once
#include "Scene.h"
#include "Application/Viewport.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class ENGINE_API FSceneViewport
    : public FViewport
{
public:
    FSceneViewport(const FViewportInitializer& InInitializer);
    ~FSceneViewport();

    virtual bool OnKeyEvent(const FKeyEvent& KeyEvent)   { return false; }
    virtual bool OnKeyTyped(FKeyCharEvent KeyTypedEvent) { return false; }

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent)         { return false; }
    virtual bool OnMouseButtonEvent(const FMouseButtonEvent& MouseEvent) { return false; }
    virtual bool OnMouseScrolled(const FMouseScrolledEvent& MouseEvent)  { return false; }

    virtual bool OnHighPrecisionMouseInput(const FHighPrecisionMouseEvent& MouseEvent) { return false; }

    FScene* GetScene() const { return Scene; }

    void SetScene(FScene* InScene) { Scene = InScene; }

private:
    FScene* Scene;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING