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

    virtual bool OnKeyEvent(const FKeyEvent& KeyEvent);
    virtual bool OnKeyTyped(FKeyCharEvent KeyTypedEvent);

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent);
    virtual bool OnMouseButtonEvent(const FMouseButtonEvent& MouseEvent);
    virtual bool OnMouseScrolled(const FMouseScrolledEvent& MouseEvent);

    virtual bool OnHighPrecisionMouseInput(const FHighPrecisionMouseEvent& MouseEvent);

    FScene* GetScene() const { return Scene; }

    void SetScene(FScene* InScene) { Scene = InScene; }

private:
    FScene* Scene;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING