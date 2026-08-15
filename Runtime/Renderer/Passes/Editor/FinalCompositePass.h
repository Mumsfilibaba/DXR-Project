#pragma once
#if EDITOR_BUILD
#include "Core/Containers/ArrayView.h"
#include "Core/Math/Vector3.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "RendererCore/Interfaces/IRendererModule.h"

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

class FRenderGraphBuilder;

class FFinalCompositePass : public FRenderPass
{
public:
    FFinalCompositePass(FSceneRenderer* InRenderer);
    virtual ~FFinalCompositePass();

    bool Initialize(const FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources);

    FRHIGraphicsPipelineStateRef CompositePSO;
    EFormat                      CompositePSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          CompositeVertexShader;
    FRHIPixelShaderRef           CompositeShader;
    FRHIDepthStencilStateRef     CompositeDepthStencilState;
    FRHIRasterizerStateRef       CompositeRasterizerState;
    FRHIBlendStateRef            CompositeBlendState;
};
#endif
