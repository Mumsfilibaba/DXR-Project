#pragma once
#include "Interface/IInterfaceWindow.h"

#include "Core/Containers/SharedRef.h"

#include <imgui.h>

class CInspectorWindow : public IInterfaceWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CInspectorWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CInspectorWindow() = default;
    ~CInspectorWindow() = default;

    /* Draws the scene info, should only be called from tick */
    void DrawSceneInfo();
};