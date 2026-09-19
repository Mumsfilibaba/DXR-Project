#pragma once
#include "MetalRHI/MetalCore.h"

// -------------------------------------------------------------------------------------------
// Metal Device Feature Support
// -------------------------------------------------------------------------------------------

extern METALRHI_API MTLGPUFamily            GMetalHighestGPUFamily;
extern METALRHI_API MTLArgumentBuffersTier  GMetalArgumentBuffersTier;
extern METALRHI_API MTLReadWriteTextureTier GMetalReadWriteTextureTier;
extern METALRHI_API bool                    GMetalSupportsRayTracing;
extern METALRHI_API bool                    GMetalSupportsRayTracingFromRender;
extern METALRHI_API bool                    GMetalSupportsMeshShaders;
extern METALRHI_API bool                    GMetalSupportsUnifiedMemory;
extern METALRHI_API uint64                  GMetalMaxBufferLength;
extern METALRHI_API uint32                  GMetalMaxThreadsPerThreadgroup;
extern METALRHI_API uint32                  GMetalMaxVertexAmplificationCount;
extern METALRHI_API uint32                  GMetalMaxTextureArrayLayers;
extern METALRHI_API uint32                  GMetalMaxTexture2DSize;
extern METALRHI_API bool                    GMetalSupportsCounterSampling;
extern METALRHI_API bool                    GMetalSupportsTimestampQueries;
extern METALRHI_API bool                    GMetalSupportsTimestampStageBoundary;
extern METALRHI_API bool                    GMetalSupportsTimestampDrawBoundary;
extern METALRHI_API bool                    GMetalSupportsTimestampDispatchBoundary;
extern METALRHI_API bool                    GMetalSupportsTimestampBlitBoundary;
extern METALRHI_API bool                    GMetalSupportsBCTextureCompression;

// -------------------------------------------------------------------------------------------
// Metal Capability Logging
// -------------------------------------------------------------------------------------------

extern METALRHI_API void DumpMetalCapabilities();
