#pragma once
#include "MetalRHI/MetalCore.h"

// -------------------------------------------------------------------------------------------
// Metal Device Feature Support
// -------------------------------------------------------------------------------------------

extern METALRHI_API MTLGPUFamily           GMetalHighestGPUFamily;
extern METALRHI_API MTLArgumentBuffersTier GMetalArgumentBuffersTier;
extern METALRHI_API MTLReadWriteTextureTier GMetalReadWriteTextureTier;
extern METALRHI_API bool                   GMetalSupportsRayTracing;
extern METALRHI_API bool                   GMetalSupportsRayTracingFromRender;
extern METALRHI_API bool                   GMetalSupportsMeshShaders;
extern METALRHI_API bool                   GMetalSupportsUnifiedMemory;
extern METALRHI_API uint64                 GMetalMaxBufferLength;
extern METALRHI_API uint32                 GMetalMaxThreadsPerThreadgroup;
extern METALRHI_API uint32                 GMetalMaxTexture2DSize;
extern METALRHI_API bool                   GMetalSupportsCounterSampling;
extern METALRHI_API bool                   GMetalSupportsBCTextureCompression;

// -------------------------------------------------------------------------------------------
// Metal Capability Logging
// -------------------------------------------------------------------------------------------

extern METALRHI_API void DumpMetalCapabilities();
