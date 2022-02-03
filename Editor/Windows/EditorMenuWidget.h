#pragma once
#include "Interface/IWindow.h"

#include <imgui.h>

class CEditorMenuWidget : public IWindow
{
    INTERFACE_GENERATE_BODY();

public:

    CEditorMenuWidget() = default;
    ~CEditorMenuWidget() = default;

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;
};