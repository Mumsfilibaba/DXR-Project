#pragma once
#include "Application/IInterfaceRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRendererModule

class CInterfaceRendererModule : public IInterfaceRendererModule
{
public:

    CInterfaceRendererModule() = default;
    ~CInterfaceRendererModule() = default;

     /** @brief: Create the renderer */
    virtual IInterfaceRenderer* CreateRenderer() override final;
};