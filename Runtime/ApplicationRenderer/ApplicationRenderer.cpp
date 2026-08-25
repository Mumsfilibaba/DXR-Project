#include "ApplicationRenderer/ApplicationRenderer.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Modules/ModuleManager.h"
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

static void DumpWindowDrawData(const FWindow& Window, const FDrawCommandList& Commands, const FUIDrawData& DrawData)
{
    LOG_INFO("[FApplicationRenderer]: Draw data for '%s': %d commands (%d box, %d text, %d line, %d clip push, %d clip pop), %d vertices, %d indices, %d batches",
        *Window.GetTitle(),
        Commands.Size(),
        Commands.CountCommandsOfType(EDrawCommandType::Box),
        Commands.CountCommandsOfType(EDrawCommandType::Text),
        Commands.CountCommandsOfType(EDrawCommandType::Line),
        Commands.CountCommandsOfType(EDrawCommandType::ClipPush),
        Commands.CountCommandsOfType(EDrawCommandType::ClipPop),
        DrawData.GetVertices().Size(),
        DrawData.GetIndices().Size(),
        DrawData.GetBatches().Size());

    if (DrawData.GetVertices().Size() >= FUIDrawData::MaxVertexCount)
    {
        LOG_WARNING("[FApplicationRenderer]: The vertex budget of %d is full, so geometry was dropped this frame", FUIDrawData::MaxVertexCount);
    }

    const TArray<FUIDrawBatch>& Batches = DrawData.GetBatches();
    for (int32 Index = 0; Index < Batches.Size(); ++Index)
    {
        const FUIDrawBatch& Batch = Batches[Index];
        LOG_INFO("[FApplicationRenderer]:   Batch %d: %s, scissor %s, indices %d to %d",
            Index,
            Batch.Atlas ? "atlas" : "white",
            Batch.bIsClipped ? *DescribeRectangle(Batch.ScissorRectangle) : "none",
            Batch.IndexOffset,
            Batch.IndexOffset + Batch.IndexCount);
    }
}

FApplicationRenderer::FApplicationRenderer()
    : WindowStates()
    , VShader(nullptr)
    , PShader(nullptr)
    , InputLayout(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , BlendState(nullptr)
    , LinearSampler(nullptr)
    , PipelineState(nullptr)
    , WhiteTexture(nullptr)
    , AtlasTexture(nullptr)
    , UploadedAtlas(nullptr)
    , UploadedAtlasRevision(0)
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

    // Boxes and glyphs share one shader, so a box samples a single white texel instead of branching
    const uint8 WhiteTexel[] = { 255, 255, 255, 255 };

    WhiteTexture = FTextureFactory::Get().LoadFromMemory(WhiteTexel, 1, 1, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!WhiteTexture)
    {
        return false;
    }

    WhiteTexture->SetDebugName("ApplicationUI WhiteTexture");
    return true;
}

void FApplicationRenderer::ReleaseRHI()
{
    WindowStates.Clear();
    VShader.Reset();
    PShader.Reset();
    InputLayout.Reset();
    DepthStencilState.Reset();
    RasterizerState.Reset();
    BlendState.Reset();
    LinearSampler.Reset();
    PipelineState.Reset();
    WhiteTexture.Reset();
    AtlasTexture.Reset();

    UploadedAtlas         = nullptr;
    UploadedAtlasRevision = 0;
    PipelineStateFormat   = EFormat::Unknown;
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
            WindowStates.RemoveAt(Index);
            return;
        }
    }
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

        WindowState.VertexBuffer   = NewVertexBuffer;
        WindowState.VertexCapacity = NewCapacity;
    }

    if (!WindowState.IndexBuffer || IndexCount > WindowState.IndexCapacity)
    {
        const int32 NewCapacity = IndexCount + GIndexGrowth;

        const FRHIBufferDesc IndexBufferDesc = FRHIBufferDesc::CreateIndexBuffer(
            sizeof(uint16), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewIndexBuffer)
        {
            return false;
        }

        NewIndexBuffer->SetDebugName("ApplicationUI IndexBuffer");
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
        FBufferRegion(0, IndexCount * sizeof(uint16)), DrawData.GetIndices().Data());

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
        return WhiteTexture ? WhiteTexture->GetShaderResourceView() : nullptr;
    }

    if (!AtlasTexture || UploadedAtlas != Atlas || UploadedAtlasRevision != Atlas->GetRevision())
    {
        FRHITextureRef NewAtlasTexture = FTextureFactory::Get().LoadFromMemory(Atlas->GetPixels(), 
            static_cast<uint32>(Atlas->GetWidth()), static_cast<uint32>(Atlas->GetHeight()),
            ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);

        if (!NewAtlasTexture)
        {
            return WhiteTexture ? WhiteTexture->GetShaderResourceView() : nullptr;
        }

        NewAtlasTexture->SetDebugName("ApplicationUI FontAtlas");

        AtlasTexture          = NewAtlasTexture;
        UploadedAtlas         = Atlas;
        UploadedAtlasRevision = Atlas->GetRevision();
    }

    if (AtlasTexture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Static)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(AtlasTexture.Get(), ERHIResourceState::PixelShaderResource));
    }

    return AtlasTexture->GetShaderResourceView();
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
        for (const FUIDrawBatch& Batch : WindowState->DrawData.GetBatches())
        {
            PrepareAtlasTexture(CommandList, Batch.Atlas);
        }
    }

    FRHIBeginRenderPassDesc RenderPassDesc({ FRHIRenderTargetAttachment(BackBufferView, EAttachmentLoadAction::Load) }, 1);
    CommandList.BeginRenderPass(RenderPassDesc);

    for (const FWindowDrawState* WindowState : DrawableStates)
    {
        RenderWindow(CommandList, *WindowState);
    }

    CommandList.EndRenderPass();
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

    const float DPIScale       = Math::Max(1.0f, Window->GetWindowDPIScale());
    const float LogicalWidth   = static_cast<float>(WindowSize.X);
    const float LogicalHeight  = static_cast<float>(WindowSize.Y);
    const float FramebufferW   = LogicalWidth * DPIScale;
    const float FramebufferH   = LogicalHeight * DPIScale;

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
    CommandList.SetIndexBuffer(WindowState.IndexBuffer.Get(), EIndexFormat::uint16);
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

        FRHIShaderResourceView* TextureView = Batch.Atlas ? GetAtlasShaderResourceView(Batch.Atlas) : GetWhiteShaderResourceView();
        if (!TextureView)
        {
            continue;
        }

        CommandList.SetShaderResourceView(PShader.Get(), TextureView, 0);
        CommandList.SetScissorRect(FScissorRegion(ScissorWidth, ScissorHeight, ScissorX, ScissorY));
        CommandList.DrawIndexedInstanced(static_cast<uint32>(Batch.IndexCount), 1, static_cast<uint32>(Batch.IndexOffset), 0, 0);
    }
}

FRHIShaderResourceView* FApplicationRenderer::GetWhiteShaderResourceView() const
{
    return WhiteTexture ? WhiteTexture->GetShaderResourceView() : nullptr;
}

FRHIShaderResourceView* FApplicationRenderer::GetAtlasShaderResourceView(const FFontAtlas* Atlas) const
{
    if (AtlasTexture && UploadedAtlas == Atlas)
    {
        return AtlasTexture->GetShaderResourceView();
    }

    return GetWhiteShaderResourceView();
}
