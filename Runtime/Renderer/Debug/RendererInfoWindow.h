#pragma once
#include "Application/Widget.h"
#include "Core/Containers/SharedRef.h"

struct FRendererInfoWindow : public FWidget
{
     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Paint() override final;
};
