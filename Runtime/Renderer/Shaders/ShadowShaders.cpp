#include "Renderer/Shaders/ShadowShaders.h"

IMPLEMENT_SHADER_TYPE(FPointLightShadowVS, TEXT("Shaders/Shadows/PointLightShadows.hlsl"), TEXT("Point_VSMain"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPointLightShadowGS, TEXT("Shaders/Shadows/PointLightShadows.hlsl"), TEXT("Point_GSMain"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FPointLightShadowPS, TEXT("Shaders/Shadows/PointLightShadows.hlsl"), TEXT("Point_PSMain"), EShaderModel::SM_6_2);

IMPLEMENT_SHADER_TYPE(FCascadeShadowVS, TEXT("Shaders/Shadows/CascadedShadows.hlsl"), TEXT("Cascade_VSMain"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FCascadeShadowGS, TEXT("Shaders/Shadows/CascadedShadows.hlsl"), TEXT("Cascade_GSMain"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FCascadeShadowPS, TEXT("Shaders/Shadows/CascadedShadows.hlsl"), TEXT("Cascade_PSMain"), EShaderModel::SM_6_2);

IMPLEMENT_SHADER_TYPE(FCascadeMatrixGenCS, TEXT("Shaders/Shadows/CascadeMatrixGen.hlsl"), TEXT("Main"), EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FShadowMaskCS,       TEXT("Shaders/Shadows/ShadowMaskGen.hlsl"),    TEXT("Main"), EShaderModel::SM_6_2);
