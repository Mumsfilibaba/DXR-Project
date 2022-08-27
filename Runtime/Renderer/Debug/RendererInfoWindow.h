#pragma once
#include "Application/Window.h"

#include "Core/Containers/SharedRef.h"

#include <imgui.h>

class FRendererInfoWindow 
    : public FWindow
{
    INTERFACE_GENERATE_BODY();

public:
    static TSharedRef<FRendererInfoWindow> Create();

     /** @brief: Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief: Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;

private:
    FRendererInfoWindow() = default;
};
