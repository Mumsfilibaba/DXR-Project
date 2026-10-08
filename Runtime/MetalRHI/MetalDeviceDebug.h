#pragma once
#include "MetalRHI/MetalCore.h"

#if METAL_ENABLE_DEBUG_LAYER

// Has to run before any MTLDevice is created
void MetalEnableDebugLayer();
bool MetalIsDebugLayerRequested();

void MetalStartValidationCapture();
void MetalStopValidationCapture();

void MetalForceGPUHang(StringView Arguments);

#endif

void MetalResetValidationErrors();
bool MetalHasValidationErrors();

void MetalBeginFrameCapture(id<MTLDevice> Device);
void MetalEndFrameCapture();
