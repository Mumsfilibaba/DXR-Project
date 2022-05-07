#pragma once
#include "Canvas/ICanvasRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRendererModule

class CInterfaceRendererModule : public IInterfaceRendererModule
{
public:

    CInterfaceRendererModule() = default;
    ~CInterfaceRendererModule() = default;

     /** @brief: Create the renderer */
    virtual ICanvasRenderer* CreateRenderer() override final;
};
