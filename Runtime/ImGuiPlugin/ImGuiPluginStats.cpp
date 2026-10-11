#include "ImGuiPlugin/ImGuiPluginStats.h"

STAT_DEFINE_COUNTER(STAT_ImGui_Viewports,      TEXT("Viewports"),       TEXT("ImGui"));
STAT_DEFINE_COUNTER(STAT_ImGui_DrawLists,      TEXT("Draw Lists"),      TEXT("ImGui"));
STAT_DEFINE_COUNTER(STAT_ImGui_DrawCalls,      TEXT("Draw Calls"),      TEXT("ImGui"));
STAT_DEFINE_COUNTER(STAT_ImGui_Vertices,       TEXT("Vertices"),        TEXT("ImGui"));
STAT_DEFINE_COUNTER(STAT_ImGui_Triangles,      TEXT("Triangles"),       TEXT("ImGui"));
STAT_DEFINE_COUNTER(STAT_ImGui_UniqueTextures, TEXT("Unique Textures"), TEXT("ImGui"));

STAT_DEFINE_MEMORY(STAT_ImGui_UploadedGeometry, TEXT("Uploaded Geometry"), TEXT("ImGui"));
STAT_DEFINE_MEMORY(STAT_ImGui_GeometryBuffers,  TEXT("Geometry Buffers"),  TEXT("ImGui"));
