#pragma once
#include "Application/Widget.h"

#include "Core/Containers/SharedRef.h"

#include <imgui.h>

class FRendererInfoWindow 
    : public FWidget
{
    INTERFACE_GENERATE_BODY();

    FRendererInfoWindow() = default;

public:
    static TSharedRef<FRendererInfoWindow> Create();

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Tick() override final;

     /** @brief - Returns true if the panel should be updated this frame */
    virtual bool IsTickable() override final;
};
