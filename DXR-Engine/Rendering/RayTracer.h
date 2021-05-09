#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

// Tests
// 0 - Reference
// 1 - RayGen HalfRes
// 2 - Inline FullRes
// 3 - Inline HalfRes
// 4 - Inline VRS Roughness
// 5 - Inline VRS Grazing Angles
#define TEST 5

#ifndef TEST
    #error No test defined
#endif

#if TEST == 2 || TEST == 3 || TEST == 4 || TEST == 5
    #define ENABLE_INLINE_RAY_GEN 1
#else
    #define ENABLE_INLINE_RAY_GEN 0
#endif

#if TEST == 1 || TEST == 3 || TEST == 4 || TEST == 5
    #define ENABLE_HALF_RES 1
#else
    #define ENABLE_HALF_RES 0
#endif

#if TEST == 4 || TEST == 5
    #define ENABLE_VRS 1
#else
    #define ENABLE_VRS 0
#endif

#if TEST == 5
    #define VRS_IMAGE_ROUGHNESS 0
#else
    #define VRS_IMAGE_ROUGHNESS 1
#endif

#define MAX_RT_MATERIALS 320

struct RandomData
{
    UInt32 FrameIndex;
    UInt32 Seed;
    UInt32 Padding1;
    UInt32 Padding2;
};

class RayTracer
{
public:
    RayTracer()  = default;
    ~RayTracer() = default;

    Bool Init(FrameResources& Resources);

    void Release();

    void Render(CommandList& CmdList, FrameResources& Resources, LightSetup& LightSetup, const Scene& Scene);

    Bool OnResize(FrameResources& FrameResources);

private:
    Bool CreateRenderTargets(FrameResources& FrameResources);

    Bool InitShadingImage(FrameResources& Resources);

    TRef<ComputePipelineState> ShadingRateGenPSO;
    TRef<ComputeShader>        ShadingRateGenShader;
    TRef<Texture2D>            ShadingRateImage;

    TRef<ComputePipelineState> RTSpatialPSO;
    TRef<ComputeShader>        RTSpatialShader;
    
    TRef<StructuredBuffer>   RTMaterialBuffer;
    TRef<ShaderResourceView> RTMaterialBufferSRV;

    TRef<GraphicsPipelineState> InlineRTPipeline;
    TRef<VertexShader>          FullscreenShader;
    TRef<PixelShader>           InlineRayGen;

    TRef<RayTracingPipelineState> Pipeline;

    TRef<RayGenShader>        RayGenShader;
    TRef<RayMissShader>       RayMissShader;
    TRef<RayClosestHitShader> RayClosestHitShader;
    
    TRef<ConstantBuffer> RandomDataBuffer;

    TRef<ComputePipelineState> BlurHorizontalPSO;
    TRef<ComputeShader>        BlurHorizontalShader;
    TRef<ComputePipelineState> BlurVerticalPSO;
    TRef<ComputeShader>        BlurVerticalShader;

    TRef<Texture2D> RTColorDepth;
    TRef<Texture2D> RTReconstructed;
    TRef<Texture2D> RTHistory0;
    TRef<Texture2D> RTHistory1;
};