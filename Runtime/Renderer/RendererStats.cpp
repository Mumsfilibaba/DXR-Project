#include "Renderer/RendererStats.h"

// Scene Stats
STAT_DEFINE_COUNTER(STAT_Scene_StaticMeshCount, "Static Meshes", "Scene");
STAT_DEFINE_COUNTER(STAT_Scene_PointLightCount, "Point Lights",  "Scene");
STAT_DEFINE_COUNTER(STAT_Scene_MaterialCount,   "Materials",     "Scene");
STAT_DEFINE_COUNTER(STAT_Scene_LightProbeCount, "Light Probes",  "Scene");

// Rendering Stats
STAT_DEFINE_COUNTER(STAT_Render_ObjectsTested,      "Objects Tested",  "Rendering");
STAT_DEFINE_COUNTER(STAT_Render_ObjectsVisible,     "Objects Visible", "Rendering");
STAT_DEFINE_COUNTER(STAT_Render_ObjectsCulled,      "Objects Culled",  "Rendering");
STAT_DEFINE_COUNTER(STAT_Render_MeshBatchCount,     "Mesh Batches",    "Rendering");
STAT_DEFINE_COUNTER(STAT_Render_MeshReferenceCount, "Mesh References", "Rendering");
