#pragma once
#if MONOLITHIC_BUILD
    #define RHI_API
#else
    #if RHI_IMPL
        #define RHI_API MODULE_EXPORT
    #else
        #define RHI_API MODULE_IMPORT
    #endif
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Config

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       (0)
#define ENABLE_API_GPU_DEBUGGING   (0) // D3D12
#define ENABLE_API_GPU_BREADCRUMBS (0) // D3D12