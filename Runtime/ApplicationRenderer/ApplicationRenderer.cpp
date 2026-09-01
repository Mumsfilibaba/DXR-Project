#include "ApplicationRenderer/ApplicationRenderer.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Modules/ModuleManager.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Window.h"
#include "Application/Text/FontAtlas.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"

IMPLEMENT_ENGINE_MODULE(IModule, ApplicationRenderer);

// The geometry is grown in blocks so a window that gains a few glyphs does not reallocate every frame
static constexpr int32 GVertexGrowth = 4096;
static constexpr int32 GIndexGrowth  = 8192;

static TAutoConsoleVariable<int32> CVarDumpDrawData(
    "ApplicationRenderer.DumpDrawData",
    "Logs the commands, geometry and batches of every window for this many frames, counting itself back down to zero",
    0);

struct FApplicationUIConstants
{
    float ProjectionMatrix[4][4];
};

static String DescribeRectangle(const FRectangle& Rectangle)
{
    return String::Printf("(%d, %d, %d x %d)", Rectangle.Position.X, Rectangle.Position.Y, Rectangle.Width, Rectangle.Height);
}

static const CHAR* DescribeBatchTexture(const FUITextureHandle& Texture)
{
    if (Texture.Atlas)
    {
        return "atlas";
    }

    return Texture.Texture ? "image" : "white";
}

static void DumpWindowDrawData(const FWindow& Window, const FDrawCommandList& Commands, const FUIDrawData& DrawData)
{
    LOG_INFO("[FApplicationRenderer]: Draw data for '%s': %d commands (%d box, %d outline, %d text, %d line, %d polyline, %d polygon, %d image, %d clip push, %d clip pop), %d vertices, %d indices, %d batches",
        *Window.GetTitle(), Commands.Size(), Commands.CountCommandsOfType(EDrawCommandType::Box),
        Commands.CountCommandsOfType(EDrawCommandType::BoxOutline), Commands.CountCommandsOfType(EDrawCommandType::Text),
        Commands.CountCommandsOfType(EDrawCommandType::Line), Commands.CountCommandsOfType(EDrawCommandType::Polyline),
        Commands.CountCommandsOfType(EDrawCommandType::ConvexPolygon), Commands.CountCommandsOfType(EDrawCommandType::Image),
        Commands.CountCommandsOfType(EDrawCommandType::ClipPush), Commands.CountCommandsOfType(EDrawCommandType::ClipPop),
        DrawData.GetVertices().Size(), DrawData.GetIndices().Size(), DrawData.GetBatches().Size());

    if (DrawData.GetVertices().Size() >= FUIDrawData::MaxVertexCount)
    {
        LOG_WARNING("[FApplicationRenderer]: The vertex budget of %d is full, so geometry was dropped this frame", FUIDrawData::MaxVertexCount);
    }

    const TArray<FUIDrawBatch>& Batches = DrawData.GetBatches();
    for (int32 Index = 0; Index < Batches.Size(); ++Index)
    {
        const FUIDrawBatch& Batch = Batches[Index];
        LOG_INFO("[FApplicationRenderer]:   Batch %d: %s, scissor %s, indices %d to %d",
            Index, DescribeBatchTexture(Batch.Texture), Batch.bIsClipped ? *DescribeRectangle(Batch.ScissorRectangle) : "none",
            Batch.IndexOffset, Batch.IndexOffset + Batch.IndexCount);
    }
}

FApplicationRenderer::FApplicationRenderer()
    : WindowStates()
    , FrameCounter(0)
    , VShader(nullptr)
    , PShader(nullptr)
    , InputLayout(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , BlendState(nullptr)
    , LinearSampler(nullptr)
    , PipelineState(nullptr)
    , DefaultTexture(nullptr)
    , AtlasTextures()
    , PipelineStateFormat(EFormat::Unknown)
{
}

FApplicationRenderer::~FApplicationRenderer()
{
    ReleaseRHI();
}

bool FApplicationRenderer::InitializeRHI()
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the vertex shader");
        return false;
    }

    VShader = RHI::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the pixel shader");
        return false;
    }

    PShader = RHI::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        return false;
    }

    TArray<FRHIInputElementDesc> InputElements =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, Position)), 0, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, TexCoord)), 1, EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, Color)),    2, EVertexInputClass::Vertex, 0 },
    };

    InputLayout = RHI::CreateInputLayout(InputElements);
    if (!InputLayout)
    {
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    RasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.bIndependentBlendEnable        = false;
    BlendStateDesc.NumRenderTargets               = 1;
    BlendStateDesc.RenderTargets[0].bBlendEnable  = true;
    BlendStateDesc.RenderTargets[0].SrcBlend      = EBlendType::SrcAlpha;
    BlendStateDesc.RenderTargets[0].SrcBlendAlpha = EBlendType::InvSrcAlpha;
    BlendStateDesc.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
    BlendStateDesc.RenderTargets[0].DstBlendAlpha = EBlendType::Zero;
    BlendStateDesc.RenderTargets[0].BlendOp       = EBlendOp::Add;
    BlendStateDesc.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;

    BlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        return false;
    }

    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    LinearSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!LinearSampler)
    {
        return false;
    }

    const uint8 WhiteTexel[] = { 255, 255, 255, 255 };
    DefaultTexture = FTextureFactory::Get().LoadFromMemory(WhiteTexel, 1, 1, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);

    if (!DefaultTexture)
    {
        return false;
    }

    DefaultTexture->SetDebugName("ApplicationUI DefaultTexture");
    return true;
}

void FApplicationRenderer::ReleaseRHI()
{
    ReleaseWindowSurfaces();

    WindowStates.Clear();
    ReleaseRetiredBuffers(true);
    VShader.Reset();
    PShader.Reset();
    InputLayout.Reset();
    DepthStencilState.Reset();
    RasterizerState.Reset();
    BlendState.Reset();
    LinearSampler.Reset();
    PipelineState.Reset();
    DefaultTexture.Reset();
    AtlasTextures.Clear();

    PipelineStateFormat = EFormat::Unknown;
}

FDrawCommandList* FApplicationRenderer::BeginWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return nullptr;
    }

    FWindowDrawState* WindowState = FindOrAddWindowState(InWindow);
    if (!WindowState)
    {
        return nullptr;
    }

    WindowState->Commands.Reset();
    WindowState->DrawData.Reset();
    return &WindowState->Commands;
}

void FApplicationRenderer::EndWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return;
    }

    for (FWindowDrawState& WindowState : WindowStates)
    {
        if (WindowState.Window == InWindow)
        {
            WindowState.DrawData.BuildFromCommandList(WindowState.Commands);

            const int32 NumFramesToDump = CVarDumpDrawData.GetValue();
            if (NumFramesToDump > 0)
            {
                DumpWindowDrawData(*InWindow, WindowState.Commands, WindowState.DrawData);
                CVarDumpDrawData->SetAsInt(NumFramesToDump - 1, EConsoleVariableFlags::SetByCode);
            }

            return;
        }
    }
}

void FApplicationRenderer::OnWindowDestroyed(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return;
    }

    for (int32 Index = 0; Index < WindowStates.Size(); ++Index)
    {
        if (WindowStates[Index].Window == InWindow)
        {
            RetireWindowBuffers(WindowStates[Index]);
            WindowStates.RemoveAt(Index);
            return;
        }
    }
}

FWindowDrawState* FApplicationRenderer::FindWindowState(const TSharedPtr<FWindow>& InWindow)
{
    for (FWindowDrawState& WindowState : WindowStates)
    {
        if (WindowState.Window == InWindow)
        {
            return &WindowState;
        }
    }

    return nullptr;
}

FWindowDrawState* FApplicationRenderer::FindOrAddWindowState(const TSharedPtr<FWindow>& InWindow)
{
    for (int32 Index = WindowStates.Size() - 1; Index >= 0; --Index)
    {
        FWindowDrawState& WindowState = WindowStates[Index];
        if (WindowState.Window == InWindow)
        {
            return &WindowState;
        }

        if (!WindowState.Window.IsValid())
        {
            RetireWindowBuffers(WindowState);
            WindowStates.RemoveAt(Index);
        }
    }

    FWindowDrawState& NewState = WindowStates.Emplace();
    NewState.Window = InWindow;
    return &NewState;
}

bool FApplicationRenderer::PreparePipelineState(EFormat OutputFormat)
{
    if (PipelineState && PipelineStateFormat == OutputFormat)
    {
        return true;
    }

    FRHIGraphicsPipelineStateDesc PipelineStateDesc;
    PipelineStateDesc.VertexShader                                   = VShader.Get();
    PipelineStateDesc.PixelShader                                    = PShader.Get();
    PipelineStateDesc.InputLayout                                    = InputLayout.Get();
    PipelineStateDesc.DepthStencilState                              = DepthStencilState.Get();
    PipelineStateDesc.RasterizerState                                = RasterizerState.Get();
    PipelineStateDesc.BlendState                                     = BlendState.Get();
    PipelineStateDesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PipelineStateDesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
    PipelineStateDesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    FRHIGraphicsPipelineStateRef NewPipelineState = RHI::CreateGraphicsPipelineState(PipelineStateDesc);
    if (!NewPipelineState)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create the pipeline state");
        return false;
    }

    PipelineState       = NewPipelineState;
    PipelineStateFormat = OutputFormat;
    return true;
}

bool FApplicationRenderer::PrepareGeometry(FRHICommandList& CommandList, FWindowDrawState& WindowState)
{
    const FUIDrawData& DrawData = WindowState.DrawData;

    const int32 VertexCount = DrawData.GetVertices().Size();
    const int32 IndexCount  = DrawData.GetIndices().Size();

    if (VertexCount <= 0 || IndexCount <= 0)
    {
        return false;
    } 

    if (!WindowState.VertexBuffer || VertexCount > WindowState.VertexCapacity)
    {
        const int32 NewCapacity = VertexCount + GVertexGrowth;

        const FRHIBufferDesc VertexBufferDesc = FRHIBufferDesc::CreateVertexBuffer(
            sizeof(FUIVertex), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewVertexBuffer)
        {
            return false;
        }

        NewVertexBuffer->SetDebugName("ApplicationUI VertexBuffer");

        if (WindowState.VertexBuffer)
        {
            RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.VertexBuffer), FrameCounter });
        }

        WindowState.VertexBuffer   = NewVertexBuffer;
        WindowState.VertexCapacity = NewCapacity;
    }

    if (!WindowState.IndexBuffer || IndexCount > WindowState.IndexCapacity)
    {
        const int32 NewCapacity = IndexCount + GIndexGrowth;

        const FRHIBufferDesc IndexBufferDesc = FRHIBufferDesc::CreateIndexBuffer(
            sizeof(uint32), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewIndexBuffer)
        {
            return false;
        }

        NewIndexBuffer->SetDebugName("ApplicationUI IndexBuffer");

        if (WindowState.IndexBuffer)
        {
            RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.IndexBuffer), FrameCounter });
        }

        WindowState.IndexBuffer   = NewIndexBuffer;
        WindowState.IndexCapacity = NewCapacity;
    }

    const FRHITransitionBarrierDesc ToCopyDest[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(WindowState.VertexBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest),
        FRHITransitionBarrierDesc::CreateBuffer(WindowState.IndexBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest),
    };

    CommandList.TransitionBarrier(ToCopyDest);

    CommandList.UpdateBuffer(WindowState.VertexBuffer.Get(), 
        FBufferRegion(0, VertexCount * sizeof(FUIVertex)), DrawData.GetVertices().Data());
    CommandList.UpdateBuffer(WindowState.IndexBuffer.Get(), 
        FBufferRegion(0, IndexCount * sizeof(uint32)), DrawData.GetIndices().Data());

    const FRHITransitionBarrierDesc ToGenericRead[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(WindowState.VertexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead),
        FRHITransitionBarrierDesc::CreateBuffer(WindowState.IndexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead),
    };

    CommandList.TransitionBarrier(ToGenericRead);
    return true;
}

FRHIShaderResourceView* FApplicationRenderer::PrepareAtlasTexture(FRHICommandList& CommandList, const FFontAtlas* Atlas)
{
    if (!Atlas || !Atlas->IsValid())
    {
        return GetDefaultShaderResourceView();
    }

    FAtlasEntry* Entry = AtlasTextures.Find(Atlas);
    if (!Entry || Entry->Revision != Atlas->GetRevision())
    {
        FRHITextureRef NewAtlasTexture = FTextureFactory::Get().LoadFromMemory(Atlas->GetPixels(), 
            static_cast<uint32>(Atlas->GetWidth()), static_cast<uint32>(Atlas->GetHeight()),
            ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);

        if (!NewAtlasTexture)
        {
            return GetDefaultShaderResourceView();
        }

        NewAtlasTexture->SetDebugName("ApplicationUI FontAtlas");

        AtlasTextures.Add(Atlas, FAtlasEntry{ NewAtlasTexture, Atlas->GetRevision() });

        Entry = AtlasTextures.Find(Atlas);
        if (!Entry)
        {
            return GetDefaultShaderResourceView();
        }
    }

    FRHITexture* EntryTexture = Entry->Texture.Get();
    if (EntryTexture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Static)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(EntryTexture, ERHIResourceState::PixelShaderResource));
    }

    return EntryTexture->GetShaderResourceView();
}

FRHIShaderResourceView* FApplicationRenderer::PrepareBrushTexture(FRHICommandList& CommandList, FRHITexture* Texture)
{
    if (!Texture)
    {
        return GetDefaultShaderResourceView();
    }

    if (Texture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Static)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::PixelShaderResource));
    }

    FRHIShaderResourceView* TextureView = Texture->GetShaderResourceView();
    return TextureView ? TextureView : GetDefaultShaderResourceView();
}

void FApplicationRenderer::PrepareBatchTextures(FRHICommandList& CommandList, const FUIDrawData& DrawData)
{
    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        if (Batch.Texture.Atlas)
        {
            PrepareAtlasTexture(CommandList, Batch.Texture.Atlas);
        }
        else if (Batch.Texture.Texture)
        {
            PrepareBrushTexture(CommandList, Batch.Texture.Texture);
        }
    }
}

FRHIShaderResourceView* FApplicationRenderer::GetBatchShaderResourceView(const FUITextureHandle& Texture) const
{
    if (Texture.Atlas)
    {
        return GetAtlasShaderResourceView(Texture.Atlas);
    }

    if (Texture.Texture)
    {
        FRHIShaderResourceView* TextureView = Texture.Texture->GetShaderResourceView();
        if (TextureView)
        {
            return TextureView;
        }
    }

    return GetDefaultShaderResourceView();
}

void FApplicationRenderer::Render(FRHICommandList& CommandList, FRHISwapChain* SwapChain)
{
    if (!SwapChain || WindowStates.IsEmpty())
    {
        return;
    }

    TArray<FWindowDrawState*> DrawableStates;
    for (FWindowDrawState& WindowState : WindowStates)
    {
        if (WindowState.SwapChain)
        {
            continue;
        }

        if (WindowState.Window.IsValid() && !WindowState.DrawData.IsEmpty())
        {
            if (PrepareGeometry(CommandList, WindowState))
            {
                DrawableStates.Add(&WindowState);
            }
        }
    }

    if (DrawableStates.IsEmpty())
    {
        return;
    }

    if (!PreparePipelineState(SwapChain->GetDesc().ColorFormat))
    {
        return;
    }

    FRHIRenderTargetView* BackBufferView = SwapChain->GetRenderTargetView();

    for (const FWindowDrawState* WindowState : DrawableStates)
    {
        PrepareBatchTextures(CommandList, WindowState->DrawData);
    }

    FRHIBeginRenderPassDesc RenderPassDesc({ FRHIRenderTargetAttachment(BackBufferView, EAttachmentLoadAction::Load) }, 1);
    CommandList.BeginRenderPass(RenderPassDesc);

    for (const FWindowDrawState* WindowState : DrawableStates)
    {
        RenderWindow(CommandList, *WindowState);
    }

    CommandList.EndRenderPass();
}

void FApplicationRenderer::RegisterWindowSwapChain(const TSharedPtr<FWindow>& InWindow, FRHISwapChain* SwapChain)
{
    if (!InWindow)
    {
        return;
    }

    if (FWindowDrawState* WindowState = FindOrAddWindowState(InWindow))
    {
        WindowState->SwapChain = SwapChain;
    }
}

void FApplicationRenderer::RenderWindowToSwapChain(FRHICommandList& CommandList, const TSharedPtr<FWindow>& InWindow, EAttachmentLoadAction LoadAction)
{
    if (!InWindow)
    {
        return;
    }

    FWindowDrawState* WindowState = FindWindowState(InWindow);
    if (!WindowState || !WindowState->SwapChain)
    {
        return;
    }

    FRHISwapChain* SwapChain = WindowState->SwapChain;
    CommandList.AcquireNextBackBuffer(SwapChain);

    if (LoadAction == EAttachmentLoadAction::Clear)
    {
        FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::Undefined, ERHIResourceState::RenderTarget));
    }

    const bool bHasGeometry = !WindowState->DrawData.IsEmpty()
        && PrepareGeometry(CommandList, *WindowState)
        && PreparePipelineState(SwapChain->GetDesc().ColorFormat);

    if (bHasGeometry)
    {
        PrepareBatchTextures(CommandList, WindowState->DrawData);
    }

    FRHIBeginRenderPassDesc RenderPassDesc({ FRHIRenderTargetAttachment(SwapChain->GetRenderTargetView(), LoadAction) }, 1);
    CommandList.BeginRenderPass(RenderPassDesc);

    if (bHasGeometry)
    {
        RenderWindow(CommandList, *WindowState);
    }

    CommandList.EndRenderPass();
}

void FApplicationRenderer::SyncWindowSurfaces(const TSharedPtr<FWindow>& PrimaryWindow)
{
    ++FrameCounter;
    ReleaseRetiredBuffers(false);

    const TArray<TSharedPtr<FWindow>>& Windows = FApplication::Get().GetWindows();

    for (int32 Index = WindowSurfaces.Size() - 1; Index >= 0; --Index)
    {
        if (Windows.Contains(WindowSurfaces[Index].Window))
        {
            continue;
        }

        FRHICommandListExecutor::Get().WaitForGPU();
        WindowSurfaces.RemoveAt(Index);

        ReleaseRetiredBuffers(true);
    }

    for (const TSharedPtr<FWindow>& CurrentWindow : Windows)
    {
        if (CurrentWindow == PrimaryWindow)
        {
            continue;
        }

        const bool bHasSurface = WindowSurfaces.ContainsWithPredicate([&CurrentWindow](const FWindowSurface& Surface)
        {
            return Surface.Window == CurrentWindow;
        });

        if (bHasSurface)
        {
            continue;
        }

        const IntVector2 WindowSize = CurrentWindow->GetSize();

        FRHISwapChainDesc SwapChainDesc;
        SwapChainDesc.WindowHandle = CurrentWindow->GetPlatformWindow()->GetPlatformHandle();
        SwapChainDesc.Width        = static_cast<uint16>(Math::Max(WindowSize.X, 1));
        SwapChainDesc.Height       = static_cast<uint16>(Math::Max(WindowSize.Y, 1));
        SwapChainDesc.ColorFormat  = EFormat::Unknown;
        SwapChainDesc.ColorSpace   = EColorSpace::Unknown;
        SwapChainDesc.Usage        = ESwapChainUsageFlags::RenderTarget;
        SwapChainDesc.bFramePacing = false;

        FRHISwapChainRef SwapChain = RHI::CreateSwapChain(SwapChainDesc);
        if (!SwapChain)
        {
            LOG_ERROR("[FApplicationRenderer]: Failed to create a swap chain for a window");
            continue;
        }

        FWindowSurface Surface;
        Surface.Window            = CurrentWindow;
        Surface.SwapChain         = SwapChain;
        Surface.Size              = WindowSize;
        Surface.DeferredShowState = CurrentWindow->ShowOnCreate() ? EDeferredShowState::None : EDeferredShowState::AwaitingPresent;

        WindowSurfaces.Add(Surface);

        RegisterWindowSwapChain(CurrentWindow, SwapChain.Get());
    }
}

void FApplicationRenderer::RenderWindowSurfaces(FRHICommandList& CommandList)
{
    for (FWindowSurface& Surface : WindowSurfaces)
    {
        const IntVector2 WindowSize = Surface.Window->GetSize();
        if (WindowSize != Surface.Size && WindowSize.X > 0 && WindowSize.Y > 0)
        {
            Surface.Size = WindowSize;
            CommandList.ResizeSwapChain(Surface.SwapChain.Get(), static_cast<uint32>(WindowSize.X), static_cast<uint32>(WindowSize.Y));
        }

        RenderWindowToSwapChain(CommandList, Surface.Window, EAttachmentLoadAction::Clear);
    }
}

void FApplicationRenderer::PresentWindowSurfaces(FRHICommandList& CommandList)
{
    for (FWindowSurface& Surface : WindowSurfaces)
    {
        FRHITexture* BackBuffer = Surface.SwapChain->GetBackBuffer();
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present));

        CommandList.PresentSwapChain(Surface.SwapChain.Get(), false);

        switch (Surface.DeferredShowState)
        {
            case EDeferredShowState::AwaitingPresent:
            {
                Surface.DeferredShowState = EDeferredShowState::PresentRecorded;
                break;
            }

            case EDeferredShowState::PresentRecorded:
            {
                Surface.Window->Show(Surface.Window->ActivateOnShow());
                Surface.DeferredShowState = EDeferredShowState::None;
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void FApplicationRenderer::ReleaseWindowSurfaces()
{
    for (const FWindowSurface& Surface : WindowSurfaces)
    {
        RegisterWindowSwapChain(Surface.Window, nullptr);
    }

    WindowSurfaces.Clear();
}

void FApplicationRenderer::RetireWindowBuffers(FWindowDrawState& WindowState)
{
    if (WindowState.VertexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.VertexBuffer), FrameCounter });
    }

    if (WindowState.IndexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.IndexBuffer), FrameCounter });
    }

    WindowState.VertexCapacity = 0;
    WindowState.IndexCapacity  = 0;
}

void FApplicationRenderer::ReleaseRetiredBuffers(bool bReleaseEverything)
{
    if (bReleaseEverything)
    {
        RetiredBuffers.Clear();
        return;
    }

    for (int32 Index = RetiredBuffers.Size() - 1; Index >= 0; --Index)
    {
        if ((FrameCounter - RetiredBuffers[Index].Frame) >= NumRetiredFrames)
        {
            RetiredBuffers.RemoveAt(Index);
        }
    }
}

void FApplicationRenderer::RenderWindow(FRHICommandList& CommandList, const FWindowDrawState& WindowState)
{
    TWeakPtr<FWindow>   WeakWindow = WindowState.Window;
    TSharedPtr<FWindow> Window     = WeakWindow.ToSharedPtr();
    if (!Window)
    {
        return;
    }

    const IntVector2 WindowSize = Window->GetSize();
    if (WindowSize.X <= 0 || WindowSize.Y <= 0)
    {
        return;
    }

    const float DPIScale      = Math::Max(1.0f, Window->GetWindowDPIScale());
    const float LogicalWidth  = static_cast<float>(WindowSize.X);
    const float LogicalHeight = static_cast<float>(WindowSize.Y);
    const float FramebufferW  = LogicalWidth * DPIScale;
    const float FramebufferH  = LogicalHeight * DPIScale;

    const float Matrix[4][4] =
    {
        { 2.0f / LogicalWidth,  0.0f,                 0.0f, 0.0f },
        { 0.0f,                -2.0f / LogicalHeight, 0.0f, 0.0f },
        { 0.0f,                 0.0f,                 0.5f, 0.0f },
        { -1.0f,                1.0f,                 0.5f, 1.0f },
    };

    FApplicationUIConstants Constants;
    Memory::Memcpy(&Constants.ProjectionMatrix, Matrix, sizeof(Matrix));

    CommandList.SetGraphicsPipelineState(PipelineState.Get());
    CommandList.SetViewport(FViewportRegion(FramebufferW, FramebufferH, 0.0f, 0.0f, 0.0f, 1.0f));
    CommandList.SetVertexBuffers(MakeArrayView(&WindowState.VertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(WindowState.IndexBuffer.Get(), EIndexFormat::uint32);
    CommandList.SetBlendFactor(Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    CommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
    CommandList.SetShaderConstants(PShader.Get(), &Constants, 16);

    for (const FUIDrawBatch& Batch : WindowState.DrawData.GetBatches())
    {
        if (Batch.IndexCount <= 0)
        {
            continue;
        }

        float ScissorX      = 0.0f;
        float ScissorY      = 0.0f;
        float ScissorWidth  = FramebufferW;
        float ScissorHeight = FramebufferH;

        if (Batch.bIsClipped)
        {
            const float MinX = Math::Max(0.0f, static_cast<float>(Batch.ScissorRectangle.Position.X) * DPIScale);
            const float MinY = Math::Max(0.0f, static_cast<float>(Batch.ScissorRectangle.Position.Y) * DPIScale);
            const float MaxX = Math::Min(FramebufferW, static_cast<float>(Batch.ScissorRectangle.GetRight()) * DPIScale);
            const float MaxY = Math::Min(FramebufferH, static_cast<float>(Batch.ScissorRectangle.GetBottom()) * DPIScale);

            if (MaxX <= MinX || MaxY <= MinY)
            {
                continue;
            }

            ScissorX      = MinX;
            ScissorY      = MinY;
            ScissorWidth  = MaxX - MinX;
            ScissorHeight = MaxY - MinY;
        }

        FRHIShaderResourceView* TextureView = GetBatchShaderResourceView(Batch.Texture);
        if (!TextureView)
        {
            continue;
        }

        CommandList.SetShaderResourceView(PShader.Get(), TextureView, 0);
        CommandList.SetScissorRect(FScissorRegion(ScissorWidth, ScissorHeight, ScissorX, ScissorY));
        CommandList.DrawIndexedInstanced(static_cast<uint32>(Batch.IndexCount), 1, static_cast<uint32>(Batch.IndexOffset), 0, 0);
    }
}

FRHIShaderResourceView* FApplicationRenderer::GetDefaultShaderResourceView() const
{
    return DefaultTexture ? DefaultTexture->GetShaderResourceView() : nullptr;
}

FRHIShaderResourceView* FApplicationRenderer::GetAtlasShaderResourceView(const FFontAtlas* Atlas) const
{
    if (const FAtlasEntry* Entry = AtlasTextures.Find(Atlas))
    {
        return Entry->Texture->GetShaderResourceView();
    }

    return GetDefaultShaderResourceView();
}
