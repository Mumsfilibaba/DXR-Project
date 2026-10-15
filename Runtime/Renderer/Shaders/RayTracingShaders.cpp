#include "Renderer/Shaders/RayTracingShaders.h"

IMPLEMENT_SHADER_TYPE(FRayGenShader,        TEXT("Shaders/RayGen.hlsl"),      TEXT("RayGen"),     EShaderModel::SM_6_3);
IMPLEMENT_SHADER_TYPE(FRayMissShader,       TEXT("Shaders/Miss.hlsl"),        TEXT("Miss"),       EShaderModel::SM_6_3);
IMPLEMENT_SHADER_TYPE(FRayClosestHitShader, TEXT("Shaders/ClosestHit.hlsl"),  TEXT("ClosestHit"), EShaderModel::SM_6_5);
IMPLEMENT_SHADER_TYPE(FRayAnyHitShader,     TEXT("Shaders/AnyHit.hlsl"),      TEXT("AnyHit"),     EShaderModel::SM_6_5);

IMPLEMENT_SHADER_TYPE(FInlineReflectionsCS, TEXT("Shaders/InlineReflections.hlsl"), TEXT("Main"), EShaderModel::SM_6_6);
IMPLEMENT_SHADER_TYPE(FPrimaryRayDebugCS,   TEXT("Shaders/PrimaryRayDebug.hlsl"),   TEXT("Main"), EShaderModel::SM_6_6);

IMPLEMENT_SHADER_TYPE(FReflectionTemporalCS, TEXT("Shaders/Reflections/ReflectionTemporal.hlsl"), TEXT("Main"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FReflectionAtrousCS,   TEXT("Shaders/Reflections/ReflectionAtrous.hlsl"),   TEXT("Main"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FReflectionUpsampleCS, TEXT("Shaders/Reflections/ReflectionUpsample.hlsl"), TEXT("Main"), EShaderModel::SM_6_2);
