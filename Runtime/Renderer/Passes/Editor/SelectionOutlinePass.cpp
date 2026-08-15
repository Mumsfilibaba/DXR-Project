#include "Renderer/Passes/Editor/SelectionOutlinePass.h"
#if EDITOR_BUILD
    #include "RHI/RHI.h"
    #include "Core/Math/Math.h"
    #include "Core/Misc/FrameProfiler.h"
    #include "Renderer/Performance/GPUProfiler.h"
    #include "Renderer/SceneRenderer.h"
    #include "Renderer/Shaders/CommonShaders.h"
    #include "Renderer/Settings/SelectionOutlineSettings.h"
    #include "RendererCore/RenderGraph/RenderGraphBuilder.h"

struct FSelectionOutlineConstantsHLSL
{
    int32 ScreenSize[2];
    int32 Direction[2];
    int32 Radius;
    uint32 SelectedCount;
    float InvViewport[2];
    float Smoothness;
    float Padding0;
};

static FRHISamplerState* GetOutlineSampler(const FFrameResources& FrameResources)
{
    return FrameResources.FXAASampler ? FrameResources.FXAASampler.Get() : FrameResources.GBufferSampler.Get();
}

static FSelectionOutlineConstantsHLSL CreateOutlineConstants(const FFrameResources& FrameResources, uint32 SelectedCount)
{
    const FSelectionOutlineSettings Settings = GetSelectionOutlineSettings();

    const uint32 RenderWidth  = FrameResources.CurrentRenderWidth;
    const uint32 RenderHeight = FrameResources.CurrentRenderHeight;

    FSelectionOutlineConstantsHLSL Constants;
    Constants.ScreenSize[0]  = int32(RenderWidth);
    Constants.ScreenSize[1]  = int32(RenderHeight);
    Constants.Direction[0]   = 0;
    Constants.Direction[1]   = 0;
    Constants.Radius         = 0;
    Constants.SelectedCount  = SelectedCount;
    Constants.InvViewport[0] = 1.0f / float(RenderWidth);
    Constants.InvViewport[1] = 1.0f / float(RenderHeight);
    Constants.Smoothness     = Settings.Smoothness;
    Constants.Padding0       = 0.0f;
    return Constants;
}

static void SetOutlineViewport(FRHICommandList& CommandList, const FFrameResources& FrameResources)
{
    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0.0f, 0.0f);
    CommandList.SetScissorRect(ScissorRegion);
}

static void RecordFilterStep(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHIGraphicsPipelineState* PipelineState, 
    FRHIPixelShader* PixelShader, FRHITexture* Source, const FSelectionOutlineConstantsHLSL& Constants)
{
    SetOutlineViewport(CommandList, FrameResources);

    CommandList.SetGraphicsPipelineState(PipelineState);

    CommandList.SetShaderResourceView(PixelShader, Source->GetShaderResourceView(), 2);
    CommandList.SetSamplerState(PixelShader, GetOutlineSampler(FrameResources), 0);

    constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(PixelShader, &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);
}

class FSelectionMaskPS
{
    DECLARE_SHADER_TYPE(FSelectionMaskPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSelectionMaskPS, "Shaders/SelectionOutline.hlsl", "SelectionMaskPS", EShaderModel::SM_6_2);

class FSelectionDilateMaxPS
{
    DECLARE_SHADER_TYPE(FSelectionDilateMaxPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSelectionDilateMaxPS, "Shaders/SelectionOutline.hlsl", "DilateMaxPS", EShaderModel::SM_6_2);

class FSelectionErodeMinPS
{
    DECLARE_SHADER_TYPE(FSelectionErodeMinPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSelectionErodeMinPS, "Shaders/SelectionOutline.hlsl", "ErodeMinPS", EShaderModel::SM_6_2);

class FSelectionRingPS
{
    DECLARE_SHADER_TYPE(FSelectionRingPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSelectionRingPS, "Shaders/SelectionOutline.hlsl", "SelectionRingPS", EShaderModel::SM_6_2);

FSelectionOutlinePass::FSelectionOutlinePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , SelectionMask(nullptr)
    , DilationTemp(nullptr)
    , DilatedMask(nullptr)
    , ErosionTemp(nullptr)
    , ErodedMask(nullptr)
    , RingMask(nullptr)
    , SelectedIDsBuffer(nullptr)
    , SelectedIDsSRV(nullptr)
    , MaskPSO(nullptr)
    , MaskShader(nullptr)
    , DilatePSO(nullptr)
    , DilateShader(nullptr)
    , ErodePSO(nullptr)
    , ErodeShader(nullptr)
    , ResolvePSO(nullptr)
    , ResolveShader(nullptr)
{
}

FSelectionOutlinePass::~FSelectionOutlinePass()
{
    SelectionMask.Reset();
    DilationTemp.Reset();
    DilatedMask.Reset();
    ErosionTemp.Reset();
    ErodedMask.Reset();
    RingMask.Reset();

    SelectedIDsBuffer.Reset();
    SelectedIDsSRV.Reset();

    MaskPSO.Reset();
    MaskShader.Reset();

    DilatePSO.Reset();
    DilateShader.Reset();

    ErodePSO.Reset();
    ErodeShader.Reset();

    ResolvePSO.Reset();
    ResolveShader.Reset();
}

bool FSelectionOutlinePass::Initialize(const FFrameResources& FrameResources)
{
    if (!CreateResources(FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

    if (!CreateSelectedIDsBuffer())
    {
        return false;
    }

    if (!CreatePipelineStates())
    {
        return false;
    }

    return true;
}

bool FSelectionOutlinePass::CreateResources(uint32 Width, uint32 Height)
{
    if (Width <= 0 || Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue ClearValue(EFormat::R8_Unorm, 0.0f, 0.0f, 0.0f, 1.0f);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Unorm, Width, Height, 1, 1, Usage, ClearValue);
    SelectionMask = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);

    if (!SelectionMask)
    {
        return false;
    }

    SelectionMask->SetDebugName("SelectionMask");

    DilationTemp = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!DilationTemp)
    {
        return false;
    }

    DilationTemp->SetDebugName("SelectionMask Dilate Temp");

    DilatedMask = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!DilatedMask)
    {
        return false;
    }

    DilatedMask->SetDebugName("SelectionMask Dilated");

    ErosionTemp = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!ErosionTemp)
    {
        return false;
    }

    ErosionTemp->SetDebugName("SelectionMask Erode Temp");

    ErodedMask = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!ErodedMask)
    {
        return false;
    }

    ErodedMask->SetDebugName("SelectionMask Eroded");

    RingMask = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource);
    if (!RingMask)
    {
        return false;
    }

    RingMask->SetDebugName("SelectionRing");

    return true;
}

bool FSelectionOutlinePass::CreateSelectedIDsBuffer()
{
    const FRHIBufferDesc BufferDesc = FRHIBufferDesc::CreateStructuredBuffer(sizeof(uint32), MaxSelectedIDs,
        EBufferFlags::Default | EBufferFlags::CopyDest);
    SelectedIDsBuffer = RHI::CreateBuffer(BufferDesc, ERHIResourceState::PixelShaderResource, nullptr);

    if (!SelectedIDsBuffer)
    {
        return false;
    }

    SelectedIDsBuffer->SetDebugName("SelectedIDs Buffer");

    const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, MaxSelectedIDs);
    SelectedIDsSRV = RHI::CreateShaderResourceView(SelectedIDsBuffer.Get(), SRVDesc);
    if (!SelectedIDsSRV)
    {
        return false;
    }

    return true;
}

bool FSelectionOutlinePass::CreatePipelineStates()
{
    FRHIVertexShaderRef FullscreenVS = FShaderCache::Get().GetShader<FFullscreenVS>();
    if (!FullscreenVS)
    {
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        return false;
    }

    // Mask PSO
    {
        MaskShader = FShaderCache::Get().GetShader<FSelectionMaskPS>();
        if (!MaskShader)
        {
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = nullptr;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = FullscreenVS.Get();
        PSODesc.PixelShader                                    = MaskShader.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8_Unorm;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

        MaskPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!MaskPSO)
        {
            return false;
        }

        MaskPSO->SetDebugName("SelectionMask PSO");
    }

    // Dilate PSO
    {
        DilateShader = FShaderCache::Get().GetShader<FSelectionDilateMaxPS>();
        if (!DilateShader)
        {
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = nullptr;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = FullscreenVS.Get();
        PSODesc.PixelShader                                    = DilateShader.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8_Unorm;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

        DilatePSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!DilatePSO)
        {
            return false;
        }

        DilatePSO->SetDebugName("SelectionDilate PSO");
    }

    // Erode PSO (min filter)
    {
        ErodeShader = FShaderCache::Get().GetShader<FSelectionErodeMinPS>();
        if (!ErodeShader)
        {
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = nullptr;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = FullscreenVS.Get();
        PSODesc.PixelShader                                    = ErodeShader.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8_Unorm;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

        ErodePSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!ErodePSO)
        {
            return false;
        }

        ErodePSO->SetDebugName("SelectionErode PSO");
    }

    // Resolve PSO (SelectedMask + DilatedMask -> RingMask)
    {
        ResolveShader = FShaderCache::Get().GetShader<FSelectionRingPS>();
        if (!ResolveShader)
        {
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = nullptr;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = FullscreenVS.Get();
        PSODesc.PixelShader                                    = ResolveShader.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = EFormat::R8_Unorm;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

        ResolvePSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (!ResolvePSO)
        {
            return false;
        }

        ResolvePSO->SetDebugName("SelectionResolve PSO");
    }

    return true;
}

bool FSelectionOutlinePass::IsOutlineActive(const FFrameResources& FrameResources) const
{
    const FSelectionOutlineSettings Settings = GetSelectionOutlineSettings();
    if (!Settings.bEnabled)
    {
        return false;
    }

    if (!SelectionMask || !DilationTemp || !DilatedMask || !ErosionTemp || !ErodedMask || !RingMask || !SelectedIDsBuffer || !SelectedIDsSRV || !MaskPSO || !DilatePSO || !ErodePSO || !ResolvePSO)
    {
        return false;
    }

    if (!GetOutlineSampler(FrameResources))
    {
        return false;
    }

    if (!FrameResources.EditorObjectID_NoJitter)
    {
        return false;
    }

    return FrameResources.CurrentRenderWidth != 0 && FrameResources.CurrentRenderHeight != 0;
}

void FSelectionOutlinePass::RecordSelectedIDsUpload(FRHICommandList& CommandList, TArrayView<const uint32> SelectedIDs)
{
    GPU_TRACE_SCOPE(CommandList, "Selection IDs");

    uint32 IDs[MaxSelectedIDs] = { 0 };
    const uint32 SelectedCount = Math::Min<uint32>(uint32(SelectedIDs.Size()), MaxSelectedIDs);
    for (uint32 Index = 0; Index < SelectedCount; ++Index)
    {
        IDs[Index] = SelectedIDs[Index];
    }

    CommandList.UpdateBuffer(SelectedIDsBuffer.Get(), FBufferRegion(0, sizeof(IDs)), IDs);
}

void FSelectionOutlinePass::RecordMask(FRHICommandList& CommandList, const FFrameResources& FrameResources, uint32 SelectedCount)
{
    GPU_TRACE_SCOPE(CommandList, "Selection Mask");

    const FSelectionOutlineConstantsHLSL Constants = CreateOutlineConstants(FrameResources, SelectedCount);

    SetOutlineViewport(CommandList, FrameResources);

    CommandList.SetGraphicsPipelineState(MaskPSO.Get());

    CommandList.SetShaderResourceView(MaskShader.Get(), FrameResources.EditorObjectID_NoJitter->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(MaskShader.Get(), SelectedIDsSRV.Get(), 1);
    CommandList.SetSamplerState(MaskShader.Get(), GetOutlineSampler(FrameResources), 0);

    constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(MaskShader.Get(), &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);
}

void FSelectionOutlinePass::RecordErode(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* Source, int32 DirectionX, int32 DirectionY, int32 Radius, uint32 SelectedCount)
{
    GPU_TRACE_SCOPE(CommandList, "Selection Erode");

    FSelectionOutlineConstantsHLSL Constants = CreateOutlineConstants(FrameResources, SelectedCount);
    Constants.Direction[0] = DirectionX;
    Constants.Direction[1] = DirectionY;
    Constants.Radius       = Radius;

    RecordFilterStep(CommandList, FrameResources, ErodePSO.Get(), ErodeShader.Get(), Source, Constants);
}

void FSelectionOutlinePass::RecordDilate(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* Source, int32 DirectionX, int32 DirectionY, int32 Radius, uint32 SelectedCount)
{
    GPU_TRACE_SCOPE(CommandList, "Selection Dilate");

    FSelectionOutlineConstantsHLSL Constants = CreateOutlineConstants(FrameResources, SelectedCount);
    Constants.Direction[0] = DirectionX;
    Constants.Direction[1] = DirectionY;
    Constants.Radius       = Radius;

    RecordFilterStep(CommandList, FrameResources, DilatePSO.Get(), DilateShader.Get(), Source, Constants);
}

void FSelectionOutlinePass::RecordRing(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* InnerMask, uint32 SelectedCount)
{
    GPU_TRACE_SCOPE(CommandList, "Selection Ring");

    const FSelectionOutlineConstantsHLSL Constants = CreateOutlineConstants(FrameResources, SelectedCount);

    SetOutlineViewport(CommandList, FrameResources);

    CommandList.SetGraphicsPipelineState(ResolvePSO.Get());

    CommandList.SetShaderResourceView(ResolveShader.Get(), InnerMask ? InnerMask->GetShaderResourceView() : nullptr, 2);
    CommandList.SetShaderResourceView(ResolveShader.Get(), DilatedMask->GetShaderResourceView(), 3);
    CommandList.SetSamplerState(ResolveShader.Get(), GetOutlineSampler(FrameResources), 0);

    constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(ResolveShader.Get(), &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);
}

void FSelectionOutlinePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    CHECK(Context.SelectedObjectIDs != nullptr);

    const bool bEnablePass = IsOutlineActive(*Context.FrameResources);

    const FSelectionOutlineSettings Settings = GetSelectionOutlineSettings();
    const int32 ThicknessPx = Math::Clamp<int32>(Settings.ThicknessPx, 1, 16);
    const int32 InnerRadius = ThicknessPx / 2;
    const int32 OuterRadius = ThicknessPx - InnerRadius;

    const bool  bEnableErosion  = bEnablePass && InnerRadius > 0;
    const uint32 SelectedCount  = Math::Min<uint32>(uint32(Context.SelectedObjectIDs->Size()), MaxSelectedIDs);

    GraphBuilder.AddPass("SelectionIDs", ERenderGraphPassFlags::Copy, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.WriteBuffer(Context.SelectedIDsBuffer, ERHIResourceState::CopyDest);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            RecordSelectedIDsUpload(PassCommandList, *Context.SelectedObjectIDs);
        });

    GraphBuilder.AddPass("SelectionMask", ERenderGraphPassFlags::Raster, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            // The outline is drawn by a fullscreen pixel shader, so the ID target has to be readable there
            PassBuilder.ReadTexture(Context.EditorObjectID_NoJitter, ERHIResourceState::PixelShaderResource);
            PassBuilder.ReadBuffer(Context.SelectedIDsBuffer, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionMask, EAttachmentLoadAction::Clear);
        },
        [this, Context, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordMask(PassCommandList, *Context.FrameResources, SelectedCount);
        });

    GraphBuilder.AddPass("SelectionErodeHorizontal", ERenderGraphPassFlags::Raster, bEnableErosion,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SelectionMask, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionErosionTemp, EAttachmentLoadAction::Clear);
        },
        [this, Context, InnerRadius, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            RecordErode(PassCommandList, *Context.FrameResources, SelectionMask.Get(), 1, 0, InnerRadius, SelectedCount);
        });

    GraphBuilder.AddPass("SelectionErodeVertical", ERenderGraphPassFlags::Raster, bEnableErosion,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SelectionErosionTemp, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionErodedMask, EAttachmentLoadAction::Clear);
        },
        [this, Context, InnerRadius, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            RecordErode(PassCommandList, *Context.FrameResources, ErosionTemp.Get(), 0, 1, InnerRadius, SelectedCount);
        });

    GraphBuilder.AddPass("SelectionDilateHorizontal", ERenderGraphPassFlags::Raster, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SelectionMask, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionDilationTemp, EAttachmentLoadAction::Clear);
        },
        [this, Context, OuterRadius, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            RecordDilate(PassCommandList, *Context.FrameResources, SelectionMask.Get(), 1, 0, OuterRadius, SelectedCount);
        });

    GraphBuilder.AddPass("SelectionDilateVertical", ERenderGraphPassFlags::Raster, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SelectionDilationTemp, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionDilatedMask, EAttachmentLoadAction::Clear);
        },
        [this, Context, OuterRadius, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            RecordDilate(PassCommandList, *Context.FrameResources, DilationTemp.Get(), 0, 1, OuterRadius, SelectedCount);
        });

    // Without erosion the ring is resolved against the unfiltered mask, which is what the chain leaves behind
    FRenderGraphTexture* InnerMaskTexture = bEnableErosion ? Context.SelectionErodedMask : Context.SelectionMask;

    GraphBuilder.AddPass("SelectionRing", ERenderGraphPassFlags::Raster, bEnablePass,
        [&Context, InnerMaskTexture](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(InnerMaskTexture, ERHIResourceState::PixelShaderResource);
            PassBuilder.ReadTexture(Context.SelectionDilatedMask, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.SelectionRing, EAttachmentLoadAction::Clear);
        },
        [this, Context, bEnableErosion, SelectedCount](FRHICommandList& PassCommandList, const FRenderGraphPassResources& /*PassResources*/)
        {
            FRHITexture* InnerMask = bEnableErosion ? ErodedMask.Get() : SelectionMask.Get();
            RecordRing(PassCommandList, *Context.FrameResources, InnerMask, SelectedCount);
        });
}

#endif // EDITOR_BUILD
