#include "Renderer/RendererStats.h"

// Scene Stats
STAT_DEFINE_COUNTER(STAT_Scene_StaticMeshCount, TEXT("Static Meshes"), TEXT("Scene"));
STAT_DEFINE_COUNTER(STAT_Scene_PointLightCount, TEXT("Point Lights"),  TEXT("Scene"));
STAT_DEFINE_COUNTER(STAT_Scene_MaterialCount,   TEXT("Materials"),     TEXT("Scene"));
STAT_DEFINE_COUNTER(STAT_Scene_LightProbeCount, TEXT("Light Probes"),  TEXT("Scene"));

// Rendering Stats
STAT_DEFINE_COUNTER(STAT_Render_ObjectsTested,      TEXT("Objects Tested"),  TEXT("Rendering"));
STAT_DEFINE_COUNTER(STAT_Render_ObjectsVisible,     TEXT("Objects Visible"), TEXT("Rendering"));
STAT_DEFINE_COUNTER(STAT_Render_ObjectsCulled,      TEXT("Objects Culled"),  TEXT("Rendering"));
STAT_DEFINE_COUNTER(STAT_Render_MeshBatchCount,     TEXT("Mesh Batches"),    TEXT("Rendering"));
STAT_DEFINE_COUNTER(STAT_Render_MeshReferenceCount, TEXT("Mesh References"), TEXT("Rendering"));

// Ray Tracing Stats
STAT_DEFINE_COUNTER(STAT_RT_Active,                   TEXT("Active"),              TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_InstanceCount,            TEXT("Instances"),           TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_HitGroupCount,            TEXT("Hit Groups"),          TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_GeometryTableRows,        TEXT("Geometry Table Rows"), TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_LazyBLASBuildsThisFrame,  TEXT("Lazy BLAS Builds"),    TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_BLASCacheLoadsThisFrame,  TEXT("BLAS Cache Loads"),    TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_AlphaTests,               TEXT("Alpha Tests"),         TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_OpacityMicromapInstances, TEXT("Micromap Instances"), TEXT("Ray Tracing"));
STAT_DEFINE_COUNTER(STAT_RT_SkippedNullGeometry,      TEXT("Skipped Null Geom"),   TEXT("Ray Tracing"));
