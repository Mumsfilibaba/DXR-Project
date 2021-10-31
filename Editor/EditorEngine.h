#pragma once
#include "Engine/Engine.h"
#include "Windows/InspectorWindow.h"
#include "Windows/EditorMenuWidget.h"

class CEditorEngine : public CEngine
{
public:

    static CEditorEngine* Make();

    /* Init engine */
    virtual bool Init() override;

    /* Tick should be called once per frame */
    virtual void Tick( CTimestamp DeltaTime ) override;

private:

    /* Scene inspector window */
    TSharedRef<CInspectorWindow> InspectorWindow;

    /* Menu bar */
    TSharedRef<CEditorMenuWidget> MenuBar;
};