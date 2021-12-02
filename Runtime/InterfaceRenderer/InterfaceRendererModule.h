#pragma once
#include "Interface/IInterfaceRenderer.h"

class CInterfaceRendererModule : public IInterfaceRendererModule
{
public:

    CInterfaceRendererModule() = default;
    ~CInterfaceRendererModule() = default;

    /* Create the renderer */
    virtual IInterfaceRenderer* CreateRenderer() override final;
};