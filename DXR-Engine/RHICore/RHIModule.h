#pragma once
#include "RHICore.h"

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       ()
#define ENABLE_API_GPU_DEBUGGING   (0)
#define ENABLE_API_GPU_BREADCRUMBS (0)

class CRHIModule
{
public:
    static bool Init( ERenderLayerApi InRenderApi );
    static void Release();
};