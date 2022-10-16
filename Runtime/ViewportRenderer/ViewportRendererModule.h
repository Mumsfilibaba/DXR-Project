#pragma once
#include "Application/IViewportRenderer.h"

class FViewportRendererModule 
    : public IViewportRendererModule
{
public:
    FViewportRendererModule()  = default;
    ~FViewportRendererModule() = default;

     /** @brief - Create the renderer */
    virtual IViewportRenderer* CreateRenderer() override final;
};
