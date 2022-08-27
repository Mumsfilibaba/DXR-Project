#pragma once
#include "Engine/Engine.h"
#include "Windows/InspectorWindow.h"
#include "Windows/EditorMenuWidget.h"

class FEditorEngine : public FEngine
{
public:

    static FEditorEngine* Make();

    /* Init engine */
    virtual bool Init() override;

    /* Tick should be called once per frame */
    virtual void Tick( FTimespan DeltaTime ) override;
};