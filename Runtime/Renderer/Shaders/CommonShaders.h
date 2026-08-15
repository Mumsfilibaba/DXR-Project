#pragma once
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderType.h"

class FFullscreenVS
{
    DECLARE_SHADER_TYPE(FFullscreenVS, EShaderStage::Vertex);

    using FPermutation = TShaderPermutation<>;
};
