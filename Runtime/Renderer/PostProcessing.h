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

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    FRHIGraphicsPipelineStateRef TonemapPSO;
    EFormat                      TonemapPSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          TonemapVertexShader;
    FRHIPixelShaderRef           TonemapShader;
    FRHIDepthStencilStateRef     TonemapDepthStencilState;
    FRHIRasterizerStateRef       TonemapRasterizerState;
    FRHIBlendStateRef            TonemapBlendState;
};

struct FFinalCompositeInfoHLSL
{
    int32   bEnableSelectionOutline;
    int32   bEnableGrid;
    float   OutlineAlpha;
    float   GridPlaneY;
    float   GridMinorSize;
    float   GridMajorSize;
    float   GridMinorWidth;
    float   GridMajorWidth;
    Vector3 OutlineColor;
    float   GridFadeDistance;
    Vector3 GridMinorColor;
    float   GridMinorAlpha;
    Vector3 GridMajorColor;
    float   GridMajorAlpha;
    float   GridHorizonFade;
    float   GridDepthBias;
    float   GridMaxTraceDistance;
    float   Padding2;
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

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    FRHIGraphicsPipelineStateRef CompositePSO;
    EFormat                      CompositePSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          CompositeVertexShader;
    FRHIPixelShaderRef           CompositeShader;
    FRHIDepthStencilStateRef     CompositeDepthStencilState;
    FRHIRasterizerStateRef       CompositeRasterizerState;
    FRHIBlendStateRef            CompositeBlendState;
};
#endif

class FFXAAPass : public FRenderPass
{
public:
    FFXAAPass(FSceneRenderer* InRenderer);
    virtual ~FFXAAPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources);

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    FRHIGraphicsPipelineStateRef FXAAPSO;
    EFormat                      FXAAPSOFormat = EFormat::Unknown;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    EFormat                      FXAADebugPSOFormat = EFormat::Unknown;
    FRHIPixelShaderRef           FXAADebugShader;
    FRHIVertexShaderRef          FXAAVertexShader;
    FRHIDepthStencilStateRef     FXAADepthStencilState;
    FRHIRasterizerStateRef       FXAARasterizerState;
    FRHIBlendStateRef            FXAABlendState;
};
