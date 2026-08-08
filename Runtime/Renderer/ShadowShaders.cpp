#include "Renderer/ShadowShaders.h"

IMPLEMENT_SHADER_TYPE(FPointLightShadowVS, "Shaders/Shadows/PointLightShadows.hlsl", "Point_VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPointLightShadowGS, "Shaders/Shadows/PointLightShadows.hlsl", "Point_GSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPointLightShadowPS, "Shaders/Shadows/PointLightShadows.hlsl", "Point_PSMain", EShaderModel::SM_6_2);

IMPLEMENT_SHADER_TYPE(FCascadeShadowVS, "Shaders/Shadows/CascadedShadows.hlsl", "Cascade_VSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FCascadeShadowGS, "Shaders/Shadows/CascadedShadows.hlsl", "Cascade_GSMain", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FCascadeShadowPS, "Shaders/Shadows/CascadedShadows.hlsl", "Cascade_PSMain", EShaderModel::SM_6_2);

IMPLEMENT_SHADER_TYPE(FCascadeMatrixGenCS, "Shaders/Shadows/CascadeMatrixGen.hlsl", "Main", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FShadowMaskCS,       "Shaders/Shadows/ShadowMaskGen.hlsl",    "Main", EShaderModel::SM_6_2);
