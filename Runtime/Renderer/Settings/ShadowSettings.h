#pragma once
#include "Core/Core.h"

// -------------------------------------------------------------------------------------------
// Shadow and Cascade Settings
// -------------------------------------------------------------------------------------------

extern bool  GCSMTightFrustum;         // SceneRenderer.cpp
extern int32 GCSMCascadeSize;          // FrameResources.cpp
extern int32 GPointLightShadowMapSize; // FrameResources.cpp
extern bool  GCSMDebugCascades;        // ShadowMaskRenderPass.cpp
extern bool  GCSMStableCascades;       // CascadeGenerationPass.cpp
