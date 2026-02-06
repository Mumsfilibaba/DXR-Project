#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Core/Math/Vector3.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

enum class ETonemappingType : int32
{
    Unknown    = 0,
    ACES       = 1,
    Reinhard   = 2,
    Uncharted2 = 3,
};

struct FTonemapInfoHLSL
{
    ETonemappingType TonemappingType;
    int32            bOutputSRGB;
    float            ReinhardIntensity;
    float            Padding0;
};

MARK_AS_REALLOCATABLE(FTonemapInfoHLSL);

class FTonemapPass : public FRenderPass
{
public:
    FTonemapPass(FSceneRenderer* InRenderer);
    virtual ~FTonemapPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* OutputTarget, bool bOutputSRGB);

private:
    FRHIGraphicsPipelineStateRef TonemapPSO_Linear;
    FRHIGraphicsPipelineStateRef TonemapPSO_BackBuffer;
    FRHIPixelShaderRef           TonemapShader;
};

struct FFinalCompositeInfoHLSL
{
    int32  bEnableSelectionOutline;
    int32  bEnableGrid;
    float  OutlineAlpha;
    float  GridPlaneY;

    float  GridMinorSize;
    float  GridMajorSize;
    float  GridMinorWidth;
    float  GridMajorWidth;

    FVector3 OutlineColor;
    float    GridFadeDistance;

    FVector3 GridMinorColor;
    float    GridMinorAlpha;

    FVector3 GridMajorColor;
    float    GridMajorAlpha;

    float    GridHorizonFade;
    float    GridDepthBias;
    float    GridMaxTraceDistance;
    float    Padding2;
};

MARK_AS_REALLOCATABLE(FFinalCompositeInfoHLSL);

#if EDITOR_BUILD
class FFinalCompositePass : public FRenderPass
{
public:
    FFinalCompositePass(FSceneRenderer* InRenderer);
    virtual ~FFinalCompositePass();

    bool Initialize(const FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources);

private:
    FRHIGraphicsPipelineStateRef CompositePSO;
    FRHIPixelShaderRef           CompositeShader;
};
#endif

class FFXAAPass : public FRenderPass
{
public:
    FFXAAPass(FSceneRenderer* InRenderer);
    virtual ~FFXAAPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources);

private:
    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;
};
