#pragma once
#include "GenericRenderLayer.h"

#define ENABLE_API_DEBUGGING 0

class RenderLayer
{
public:
    static Bool Init(ERenderLayerApi InRenderApi);
    static void Release();
};