#pragma once
#include "Core/Containers/SharedRef.h"

#include "Interface/IInterfaceWindow.h"

#include <imgui.h>

class CRendererInfoWindow : public IInterfaceWindow
{
    INTERFACE_GENERATE_BODY();

public:

    static TSharedRef<CRendererInfoWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CRendererInfoWindow() = default;
};