#pragma once
#include "Application/IApplicationRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationInterfaceRendererModule

class FApplicationInterfaceRendererModule 
    : public IApplicationRendererModule
{
public:
    FApplicationInterfaceRendererModule()  = default;
    ~FApplicationInterfaceRendererModule() = default;

     /** @brief: Create the renderer */
    virtual IApplicationRenderer* CreateRenderer() override final;
};
