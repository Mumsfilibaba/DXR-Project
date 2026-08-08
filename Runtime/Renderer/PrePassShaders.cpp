#include "Renderer/PrePassShaders.h"

IMPLEMENT_SHADER_TYPE(FPrePassVS, "Shaders/PrePass.hlsl", "VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPrePassPS, "Shaders/PrePass.hlsl", "PSMain", EShaderModel::SM_6_2);
