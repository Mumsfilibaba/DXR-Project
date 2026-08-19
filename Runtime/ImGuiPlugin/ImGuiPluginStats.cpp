#include "ImGuiPlugin/ImGuiPluginStats.h"

STAT_DEFINE_COUNTER(STAT_ImGui_Viewports,      "Viewports",       "ImGui");
STAT_DEFINE_COUNTER(STAT_ImGui_DrawLists,      "Draw Lists",      "ImGui");
STAT_DEFINE_COUNTER(STAT_ImGui_DrawCalls,      "Draw Calls",      "ImGui");
STAT_DEFINE_COUNTER(STAT_ImGui_Vertices,       "Vertices",        "ImGui");
STAT_DEFINE_COUNTER(STAT_ImGui_Triangles,      "Triangles",       "ImGui");
STAT_DEFINE_COUNTER(STAT_ImGui_UniqueTextures, "Unique Textures", "ImGui");

STAT_DEFINE_MEMORY(STAT_ImGui_UploadedGeometry, "Uploaded Geometry", "ImGui");
STAT_DEFINE_MEMORY(STAT_ImGui_GeometryBuffers,  "Geometry Buffers",  "ImGui");
