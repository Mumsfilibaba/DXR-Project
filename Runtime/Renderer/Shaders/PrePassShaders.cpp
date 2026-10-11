#include "Renderer/Shaders/PrePassShaders.h"

IMPLEMENT_SHADER_TYPE(FPrePassVS, TEXT("Shaders/PrePass.hlsl"), TEXT("VSMain"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPrePassPS, TEXT("Shaders/PrePass.hlsl"), TEXT("PSMain"), EShaderModel::SM_6_2);
