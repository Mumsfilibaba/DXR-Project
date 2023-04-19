#pragma once
#include "Application/Widget.h"
#include "Core/Containers/SharedRef.h"

class FRendererInfoWindow : public FWidget
{
    DECLARE_WIDGET(FRendererInfoWindow, FWidget);

public:
    FINITIALIZER_START(FRendererInfoWindow)
    FINITIALIZER_END();

    void Initialize(const FInitializer& Initializer)
    {
    }

     /** @brief - Update the panel, for ImGui this is where the ImGui-Commands should be called */
    virtual void Paint(const FRectangle& AssignedBounds) override final;
};
