#pragma once
#include "Engine/Engine.h"

class FEditorDockspaceWidget;
class FEditorConsoleInputFieldWidget;
class FEditorLogOutputWidget;
class FEditorViewportWidget;
class FEditorSceneHierarchyWidget;

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    // FEngine Interface
    virtual bool Init() override final;
	virtual void Release() override final;

    virtual void Tick(float DeltaTime) override final;
    virtual void RenderFrame() override final;

    // Editor Widgets
	const TSharedPtr<FEditorDockspaceWidget>&          GetDockspaceWidget() const { return DockspaceWidget; }
    const TSharedPtr<FEditorConsoleInputFieldWidget>&  GetConsoleWidget() const { return ConsoleWidget; }
    const TSharedPtr<FEditorLogOutputWidget>&          GetLogOutputWidget() const { return LogOutputWidget; }
    const TSharedPtr<FEditorSceneHierarchyWidget>&     GetSceneHierarchyWidget() const { return SceneHierarchyWidget; }
    const TSharedPtr<FEditorViewportWidget>&           GetEditorViewportWidget() const { return ViewportWidget; }

private:
    bool CreateViewportRenderTarget();

	TSharedPtr<FEditorDockspaceWidget>         DockspaceWidget;
    TSharedPtr<FEditorConsoleInputFieldWidget> ConsoleWidget;
    TSharedPtr<FEditorLogOutputWidget>         LogOutputWidget;
    TSharedPtr<FEditorSceneHierarchyWidget>    SceneHierarchyWidget;
    TSharedPtr<FEditorViewportWidget>          ViewportWidget;
    FRHITextureRef                             ViewportImage;
    FIntVector2                                ViewportImageSize;
};