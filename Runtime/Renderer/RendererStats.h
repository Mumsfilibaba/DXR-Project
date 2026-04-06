#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// Scene Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RENDERER_API, STAT_Scene_StaticMeshCount);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Scene_PointLightCount);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Scene_MaterialCount);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Scene_LightProbeCount);

// -------------------------------------------------------------------------------------------
// Rendering Stats (per-frame, reset each frame)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RENDERER_API, STAT_Render_ObjectsTested);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Render_ObjectsVisible);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Render_ObjectsCulled);

STAT_DECLARE_EXTERN(RENDERER_API, STAT_Render_MeshBatchCount);
STAT_DECLARE_EXTERN(RENDERER_API, STAT_Render_MeshReferenceCount);
