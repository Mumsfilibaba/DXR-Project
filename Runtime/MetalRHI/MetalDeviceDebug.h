#pragma once
#include "MetalRHI/MetalConfiguration.h"

#if METAL_ENABLE_DEBUG_LAYER

/** @brief Turns on Metal's debug layer when RHI.EnableDebugLayer is set, before any MTLDevice is created. */
void MetalEnableDebugLayer();

/** @brief Mirrors Metal validation text from stderr into the engine log. */
void MetalStartValidationCapture();

/** @brief Restores stderr and joins the capture thread. */
void MetalStopValidationCapture();

#endif
