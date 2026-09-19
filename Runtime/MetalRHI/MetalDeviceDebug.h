#pragma once
#include "MetalRHI/MetalCore.h"

#if METAL_ENABLE_DEBUG_LAYER

/** @brief Turns on Metal's debug layer when RHI.EnableDebugLayer is set, before any MTLDevice is created. */
void MetalEnableDebugLayer();

/** @brief Mirrors Metal validation text from stderr into the engine log, leaving stderr itself intact. */
void MetalStartValidationCapture();

/** @brief Restores stderr and joins the capture thread. */
void MetalStopValidationCapture();

#endif

/** @brief Clears the captured Metal validation error count. */
void MetalResetValidationErrors();

/** @return True when the capture has seen a Metal validation error since the last reset. */
bool MetalHasValidationErrors();

/** @brief Starts a one-frame GPU capture when MetalRHI.CaptureNextFrame is set. */
void MetalBeginFrameCapture(id<MTLDevice> Device);

/** @brief Stops a capture started by MetalBeginFrameCapture. */
void MetalEndFrameCapture();
