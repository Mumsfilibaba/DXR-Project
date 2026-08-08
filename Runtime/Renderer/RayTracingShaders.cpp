#include "Renderer/RayTracingShaders.h"

IMPLEMENT_SHADER_TYPE(FRayGenShader,        "Shaders/RayGen.hlsl",      "RayGen",     EShaderModel::SM_6_3);
IMPLEMENT_SHADER_TYPE(FRayMissShader,       "Shaders/Miss.hlsl",        "Miss",       EShaderModel::SM_6_3);
IMPLEMENT_SHADER_TYPE(FRayClosestHitShader, "Shaders/ClosestHit.hlsl",  "ClosestHit", EShaderModel::SM_6_3);

IMPLEMENT_SHADER_TYPE(FInlineReflectionsCS, "Shaders/InlineReflections.hlsl", "Main", EShaderModel::SM_6_6);
IMPLEMENT_SHADER_TYPE(FPrimaryRayDebugCS,   "Shaders/PrimaryRayDebug.hlsl",   "Main", EShaderModel::SM_6_6);

IMPLEMENT_SHADER_TYPE(FReflectionTemporalCS, "Shaders/Reflections/ReflectionTemporal.hlsl", "Main", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FReflectionAtrousCS,   "Shaders/Reflections/ReflectionAtrous.hlsl",   "Main", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FReflectionUpsampleCS, "Shaders/Reflections/ReflectionUpsample.hlsl", "Main", EShaderModel::SM_6_2);
