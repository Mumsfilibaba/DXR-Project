#include "Renderer/SelectionOutlinePass.h"
#if EDITOR_BUILD
    #include "RHI/RHI.h"
    #include "RHI/ShaderCompiler.h"
    #include "Core/Math/Math.h"
    #include "Core/Misc/FrameProfiler.h"
    #include "Renderer/Performance/GPUProfiler.h"
    #include "Renderer/SceneRenderer.h"
    #include "Renderer/SelectionOutlineSettings.h"

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

    SelectionMask = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!SelectionMask)
    {
        return false;
    }

    SelectionMask->SetDebugName("SelectionMask");

    DilationTemp = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!DilationTemp)
    {
        return false;
    }

    DilationTemp->SetDebugName("SelectionMask Dilate Temp");

    DilatedMask = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!DilatedMask)
    {
        return false;
    }

    DilatedMask->SetDebugName("SelectionMask Dilated");

    ErosionTemp = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!ErosionTemp)
    {
        return false;
    }

    ErosionTemp->SetDebugName("SelectionMask Erode Temp");

    ErodedMask = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!ErodedMask)
    {
        return false;
    }

    ErodedMask->SetDebugName("SelectionMask Eroded");

    RingMask = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!RingMask)
    {
        return false;
    }

    RingMask->SetDebugName("SelectionRing");

    return true;
}

bool FSelectionOutlinePass::CreateSelectedIDsBuffer()
{
    FRHIBufferDesc BufferDesc;
    BufferDesc.Stride = sizeof(uint32);
    BufferDesc.Size   = uint64(BufferDesc.Stride) * MaxSelectedIDs;
    BufferDesc.Flags  = EBufferFlags::ShaderResourceBuffer | EBufferFlags::CopyDest | EBufferFlags::Default;

    SelectedIDsBuffer = RHI::CreateBuffer(BufferDesc, EResourceAccess::PixelShaderResource, nullptr);
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
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        return false;
    }

    FRHIVertexShaderRef FullscreenVS = RHI::CreateVertexShader(ShaderCode);
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
        CompileInfo = FShaderCompileInfo("SelectionMaskPS", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/SelectionOutline.hlsl", CompileInfo, ShaderCode))
        {
            return false;
        }

        MaskShader = RHI::CreatePixelShader(ShaderCode);
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
        CompileInfo = FShaderCompileInfo("DilateMaxPS", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/SelectionOutline.hlsl", CompileInfo, ShaderCode))
        {
            return false;
        }

        DilateShader = RHI::CreatePixelShader(ShaderCode);
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
        CompileInfo = FShaderCompileInfo("ErodeMinPS", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/SelectionOutline.hlsl", CompileInfo, ShaderCode))
        {
            return false;
        }

        ErodeShader = RHI::CreatePixelShader(ShaderCode);
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
        CompileInfo = FShaderCompileInfo("SelectionRingPS", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/SelectionOutline.hlsl", CompileInfo, ShaderCode))
        {
            return false;
        }

        ResolveShader = RHI::CreatePixelShader(ShaderCode);
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

void FSelectionOutlinePass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, TArrayView<const uint32> SelectedIDs)
{
    RHI_EVENT_SCOPE(CommandList, "Selection Outline");

    TRACE_SCOPE("SelectionOutline");

    GPU_TRACE_SCOPE(CommandList, "Selection Outline");

    const FSelectionOutlineSettings Settings = GetSelectionOutlineSettings();
    if (!Settings.bEnabled)
    {
        return;
    }

    if (!SelectionMask || !DilationTemp || !DilatedMask || !ErosionTemp || !ErodedMask || !RingMask || !SelectedIDsBuffer || !SelectedIDsSRV || !MaskPSO || !DilatePSO || !ErodePSO || !ResolvePSO)
    {
        return;
    }

    FRHISamplerState* LinearClampSampler = FrameResources.FXAASampler ? FrameResources.FXAASampler.Get() : FrameResources.GBufferSampler.Get();
    if (!LinearClampSampler)
    {
        return;
    }

#if EDITOR_BUILD
    FRHITexture* ObjectIDTexture = FrameResources.EditorObjectID_NoJitter.Get();
#else
    FRHITexture* ObjectIDTexture = nullptr;
#endif

    if (!ObjectIDTexture)
    {
        return;
    }

    const uint32 RenderWidth  = FrameResources.CurrentRenderWidth;
    const uint32 RenderHeight = FrameResources.CurrentRenderHeight;
    if (RenderWidth == 0 || RenderHeight == 0)
    {
        return;
    }

    // Upload selected IDs
    uint32 IDs[MaxSelectedIDs] = { 0 };
    const uint32 SelectedCount = Math::Min<uint32>(uint32(SelectedIDs.Size()), MaxSelectedIDs);
    for (uint32 Index = 0; Index < SelectedCount; ++Index)
    {
        IDs[Index] = SelectedIDs[Index];
    }

    CommandList.TransitionBufferState(SelectedIDsBuffer.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(SelectedIDsBuffer.Get(), FBufferRegion(0, sizeof(IDs)), IDs);
    CommandList.TransitionBufferState(SelectedIDsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

    struct FSelectionOutlineConstantsHLSL
    {
        int32 ScreenSize[2];
        int32 Direction[2];
        int32 Radius;
        uint32 SelectedCount;
        float InvViewport[2];
        float Smoothness;
        float Padding0;
    } Constants;

    Constants.ScreenSize[0]   = int32(RenderWidth);
    Constants.ScreenSize[1]   = int32(RenderHeight);
    Constants.SelectedCount   = SelectedCount;
    Constants.InvViewport[0]  = 1.0f / float(RenderWidth);
    Constants.InvViewport[1]  = 1.0f / float(RenderHeight);
    Constants.Smoothness      = Settings.Smoothness;
    Constants.Padding0        = 0;

    const int32 ThicknessPx = Math::Clamp<int32>(Settings.ThicknessPx, 1, 16);
    const int32 InnerRadius = ThicknessPx / 2;
    const int32 OuterRadius = ThicknessPx - InnerRadius;

    FRHITexture* InnerMaskTexture = SelectionMask.Get();

    FViewportRegion ViewportRegion(float(RenderWidth), float(RenderHeight), 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(float(RenderWidth), float(RenderHeight), 0.0f, 0.0f);
    CommandList.SetScissorRect(ScissorRegion);

    // ---------------------------------------------------------------------------
    // Pass 1: SelectedMask (ObjectID -> 0/1)
    // ---------------------------------------------------------------------------

    CommandList.TransitionTextureState(SelectionMask.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

    {
        FRHIRenderTargetView* RenderTargetView = SelectionMask->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

        CommandList.BeginRenderPass(RenderPassDesc);

        CommandList.SetGraphicsPipelineState(MaskPSO.Get());

        CommandList.SetShaderResourceView(MaskShader.Get(), ObjectIDTexture->GetShaderResourceView(), 0);
        CommandList.SetShaderResourceView(MaskShader.Get(), SelectedIDsSRV.Get(), 1);
        CommandList.SetSamplerState(MaskShader.Get(), LinearClampSampler, 0);

        Constants.Direction[0] = 0;
        Constants.Direction[1] = 0;

        constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(MaskShader.Get(), &Constants, NumConstants);

        CommandList.DrawInstanced(3, 1, 0, 0);

        CommandList.EndRenderPass();
    }

    CommandList.TransitionTextureState(SelectionMask.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));

    // ---------------------------------------------------------------------------
    // Pass 2: Erosion (min filter) to pull outline closer to the object
    // ---------------------------------------------------------------------------

    if (InnerRadius > 0)
    {
        // Horizontal erosion
        CommandList.TransitionTextureState(ErosionTemp.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

        {
        FRHIRenderTargetView* RenderTargetView = ErosionTemp->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

            CommandList.BeginRenderPass(RenderPassDesc);

            CommandList.SetGraphicsPipelineState(ErodePSO.Get());

            CommandList.SetShaderResourceView(ErodeShader.Get(), SelectionMask->GetShaderResourceView(), 2);
            CommandList.SetSamplerState(ErodeShader.Get(), LinearClampSampler, 0);

            Constants.Direction[0] = 1;
            Constants.Direction[1] = 0;
            Constants.Radius       = InnerRadius;

            constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
            CommandList.SetShaderConstants(ErodeShader.Get(), &Constants, NumConstants);

            CommandList.DrawInstanced(3, 1, 0, 0);

            CommandList.EndRenderPass();
        }

        CommandList.TransitionTextureState(ErosionTemp.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));

        // Vertical erosion
        CommandList.TransitionTextureState(ErodedMask.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

        {
        FRHIRenderTargetView* RenderTargetView = ErodedMask->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

            CommandList.BeginRenderPass(RenderPassDesc);

            CommandList.SetGraphicsPipelineState(ErodePSO.Get());

            CommandList.SetShaderResourceView(ErodeShader.Get(), ErosionTemp->GetShaderResourceView(), 2);
            CommandList.SetSamplerState(ErodeShader.Get(), LinearClampSampler, 0);

            Constants.Direction[0] = 0;
            Constants.Direction[1] = 1;
            Constants.Radius       = InnerRadius;

            constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
            CommandList.SetShaderConstants(ErodeShader.Get(), &Constants, NumConstants);

            CommandList.DrawInstanced(3, 1, 0, 0);

            CommandList.EndRenderPass();
        }

        CommandList.TransitionTextureState(ErodedMask.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));
        InnerMaskTexture = ErodedMask.Get();
    }

    // ---------------------------------------------------------------------------
    // Pass 3: Horizontal dilation
    // ---------------------------------------------------------------------------

    CommandList.TransitionTextureState(DilationTemp.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

    {
        FRHIRenderTargetView* RenderTargetView = DilationTemp->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

        CommandList.BeginRenderPass(RenderPassDesc);

        CommandList.SetGraphicsPipelineState(DilatePSO.Get());

        CommandList.SetShaderResourceView(DilateShader.Get(), SelectionMask->GetShaderResourceView(), 2);
        CommandList.SetSamplerState(DilateShader.Get(), LinearClampSampler, 0);

        Constants.Direction[0] = 1;
        Constants.Direction[1] = 0;
        Constants.Radius       = OuterRadius;

        constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(DilateShader.Get(), &Constants, NumConstants);

        CommandList.DrawInstanced(3, 1, 0, 0);

        CommandList.EndRenderPass();
    }

    CommandList.TransitionTextureState(DilationTemp.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));

    // ---------------------------------------------------------------------------
    // Pass 4: Vertical dilation
    // ---------------------------------------------------------------------------

    CommandList.TransitionTextureState(DilatedMask.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

    {
        FRHIRenderTargetView* RenderTargetView = DilatedMask->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

        CommandList.BeginRenderPass(RenderPassDesc);

        CommandList.SetGraphicsPipelineState(DilatePSO.Get());

        CommandList.SetShaderResourceView(DilateShader.Get(), DilationTemp->GetShaderResourceView(), 2);
        CommandList.SetSamplerState(DilateShader.Get(), LinearClampSampler, 0);

        Constants.Direction[0] = 0;
        Constants.Direction[1] = 1;
        Constants.Radius       = OuterRadius;

        constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(DilateShader.Get(), &Constants, NumConstants);

        CommandList.DrawInstanced(3, 1, 0, 0);

        CommandList.EndRenderPass();
    }

    CommandList.TransitionTextureState(DilatedMask.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));

    // ---------------------------------------------------------------------------
    // Pass 5: Resolve ring mask (Outer - Inner) with optional smoothing
    // ---------------------------------------------------------------------------

    CommandList.TransitionTextureState(RingMask.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

    {
        FRHIRenderTargetView* RenderTargetView = RingMask->GetRenderTargetView();

        FRHIBeginRenderPassDesc RenderPassDesc;
        RenderPassDesc.NumRenderTargets = 1;
        RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

        CommandList.BeginRenderPass(RenderPassDesc);

        CommandList.SetGraphicsPipelineState(ResolvePSO.Get());

        CommandList.SetShaderResourceView(ResolveShader.Get(), InnerMaskTexture ? InnerMaskTexture->GetShaderResourceView() : nullptr, 2);
        CommandList.SetShaderResourceView(ResolveShader.Get(), DilatedMask->GetShaderResourceView(), 3);
        CommandList.SetSamplerState(ResolveShader.Get(), LinearClampSampler, 0);

        Constants.Direction[0] = 0;
        Constants.Direction[1] = 0;
        Constants.Radius       = 0;

        constexpr uint32 NumConstants = sizeof(FSelectionOutlineConstantsHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(ResolveShader.Get(), &Constants, NumConstants);

        CommandList.DrawInstanced(3, 1, 0, 0);

        CommandList.EndRenderPass();
    }

    CommandList.TransitionTextureState(RingMask.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));
}

#endif // EDITOR_BUILD
