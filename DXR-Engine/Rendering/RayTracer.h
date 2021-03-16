#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

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

    TRef<RayTracingPipelineState> Pipeline;
    TRef<ComputePipelineState>    RTSpatialPSO;
    TRef<ComputeShader>        RTSpatialShader;
    TRef<RayGenShader>         RayGenShader;
    TRef<RayMissShader>        RayMissShader;
    TRef<RayClosestHitShader>  RayClosestHitShader;
    TRef<ConstantBuffer>       RandomDataBuffer;
    TRef<ComputePipelineState> BlurHorizontalPSO;
    TRef<ComputeShader>        BlurHorizontalShader;
    TRef<ComputePipelineState> BlurVerticalPSO;
    TRef<ComputeShader>        BlurVerticalShader;

    TRef<Texture2D> RTColorDepth;
    TRef<Texture2D> RTHistory;
    TRef<Texture2D> RTMomentBuffer;
};