#pragma once
#include "Canvas/IApplicationRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationRendererModule

class FApplicationRendererModule 
    : public IApplicationRendererModule
{
public:
    FApplicationRendererModule()  = default;
    ~FApplicationRendererModule() = default;

     /** @brief: Create the renderer */
    virtual IApplicationRenderer* CreateRenderer() override final;
};
