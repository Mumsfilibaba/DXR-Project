#pragma once
#include "GenericRenderLayer.h"

// TODO: Maybe should be in a config file
#define ENABLE_API_DEBUGGING       0
#define ENABLE_API_GPU_DEBUGGING   0
#define ENABLE_API_GPU_BREADCRUMBS 0

class RenderLayer
{
public:
    static bool Init(ERenderLayerApi InRenderApi);
    static void Release();
};