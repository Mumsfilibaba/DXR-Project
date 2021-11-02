#pragma once
#include "Core/Application/UI/IUIWindow.h"
#include "Core/Containers/SharedRef.h"

class CRendererInfoWindow : public IUIWindow
{
    INTERFACE_GENERATE_BODY

public:

    static TSharedRef<CRendererInfoWindow> Make();

    /* Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

    /* Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:

    CRendererInfoWindow() = default;
};