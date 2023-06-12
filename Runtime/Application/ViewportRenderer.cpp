#include "ViewportRenderer.h"
#include "Application.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "RHI/RHIInterface.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShaderCompiler.h"
#include "RendererCore/TextureFactory.h"

#include <imgui.h>

struct FVertexConstantBuffer
{
    float ViewProjectionMatrix[4][4];
};

bool FViewport::InitializeRHI(const FViewportInitializer& Initializer)
{
    TSharedRef<FRHIViewport> NewViewport = RHICreateViewport(FRHIViewportDesc()
        .SetWindowHandle(Initializer.Window->GetPlatformHandle())
        .SetWidth(Initializer.Width)
        .SetHeight(Initializer.Height)
        .SetColorFormat(EFormat::R8G8B8A8_Unorm));

    if (NewViewport)
    {
        RHIViewport = NewViewport;
        Window = MakeSharedRef<FGenericWindow>(Initializer.Window);
    }
    else
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FViewport::ReleaseRHI()
{
    RHIViewport.Reset();
}


static void ImGuiCreateWindow(ImGuiViewport* InViewport)
{
    if (TSharedRef<FGenericWindow> Window = MakeSharedRef<FGenericWindow>(reinterpret_cast<FGenericWindow*>(InViewport->PlatformUserData)))
    {
        TSharedPtr<FViewport> Viewport = FApplication::Get().CreateViewport(FViewportInitializer()
            .SetWindow(Window.Get())
            .SetWidth(static_cast<int32>(InViewport->Size.x))
            .SetHeight(static_cast<int32>(InViewport->Size.y)));

        if (Viewport)
        {
            InViewport->RendererUserData = Viewport.Get();
        }
    }
}

static void ImGuiDestroyWindow(ImGuiViewport* Viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
    if (FViewport* ViewportData = reinterpret_cast<FViewport*>(Viewport->RendererUserData))
    {
        GRHICommandExecutor.WaitForGPU();
        // TODO: Destroy the viewport via the application
    }

    Viewport->RendererUserData = nullptr;
}

static void ImGuiSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    if (FViewport* ViewportData = reinterpret_cast<FViewport*>(Viewport->RendererUserData))
    {
        GRHICommandExecutor.WaitForGPU();
        ViewportData->GetRHIViewport()->Resize(static_cast<uint32>(Size.x), static_cast<uint32>(Size.y));
    }
}

static void ImGuiRenderWindow(ImGuiViewport* Viewport, void* CmdList)
{
    FRHICommandList* RHICmdList = reinterpret_cast<FRHICommandList*>(CmdList);
    if (!RHICmdList)
    {
        return;
    }

    FViewport* ViewportData = reinterpret_cast<FViewport*>(Viewport->RendererUserData);
    if (!ViewportData)
    {
        return;
    }

    if (FApplication::IsInitialized())
    {
        if (FViewportRenderer* Renderer = FApplication::Get().GetRenderer())
        {
            const bool bClear = (Viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0;
            Renderer->RenderViewport(*RHICmdList, Viewport->DrawData, ViewportData, bClear);
        }
    }
}

static void ImGuiSwapBuffers(ImGuiViewport* Viewport, void* CmdList)
{
    bool bEnableVsync = false;
    if (IConsoleVariable* CVarVSyncEnabled = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.VerticalSync"))
    {
        bEnableVsync = CVarVSyncEnabled->GetBool();
    }

    if (FViewport* ViewportData = reinterpret_cast<FViewport*>(Viewport->RendererUserData))
    {
        if (FRHICommandList* RHICmdList = reinterpret_cast<FRHICommandList*>(CmdList))
        {
            RHICmdList->PresentViewport(ViewportData->GetRHIViewport(), bEnableVsync);
        }
    }
}


bool FViewportRenderer::Initialize()
{
    // Start by initializing the functions for handling Viewports
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    PlatformState.Renderer_CreateWindow  = ImGuiCreateWindow;
    PlatformState.Renderer_DestroyWindow = ImGuiDestroyWindow;
    PlatformState.Renderer_SetWindowSize = ImGuiSetWindowSize;
    PlatformState.Renderer_RenderWindow  = ImGuiRenderWindow;
    PlatformState.Renderer_SwapBuffers   = ImGuiSwapBuffers;

    // Build texture atlas
    uint8* Pixels = nullptr;
    int32  Width  = 0;
    int32  Height = 0;

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendRendererUserData = this;
    UIState.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    FontTexture = FTextureFactory::LoadFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
    if (!FontTexture)
    {
        return false;
    }

    const CHAR* VSSource =
        R"*(
        cbuffer VertexBuffer : register(b0, space1)
        {
            float4x4 ProjectionMatrix;
        };

        struct VS_INPUT
        {
            float2 Position : POSITION;
            float4 Color    : COLOR0;
            float2 TexCoord : TEXCOORD0;
        };

        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float4 Color    : COLOR0;
            float2 TexCoord : TEXCOORD0;
        };

        PS_INPUT Main(VS_INPUT Input)
        {
            PS_INPUT Output;
            Output.Position = mul(ProjectionMatrix, float4(Input.Position.xy, 0.0f, 1.0f));
            Output.Color    = Input.Color;
            Output.TexCoord = Input.TexCoord;
            return Output;
        })*";

    TArray<uint8> ShaderCode;

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromSource(VSSource, CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    FRHIVertexShaderRef VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }

    const CHAR* PSSource =
        R"*(
        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float4 Color    : COLOR0;
            float2 TexCoord : TEXCOORD0;
        };

        SamplerState Sampler0 : register(s0);
        Texture2D    Texture0 : register(t0);

        float4 Main(PS_INPUT Input) : SV_Target
        {
            float4 OutColor = Input.Color * Texture0.Sample(Sampler0, Input.TexCoord);
            return OutColor;
        })*";

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromSource(PSSource, CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    PShader = RHICreatePixelShader(ShaderCode);
    if (!PShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexInputLayoutDesc InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), EVertexInputClass::Vertex, 0 },
    };

    FRHIVertexInputLayoutRef InputLayout = RHICreateVertexInputLayout(InputLayoutInfo);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateInfo;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateInfo;
    RasterizerStateInfo.CullMode               = ECullMode::None;
    RasterizerStateInfo.bAntialiasedLineEnable = true;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateInfo;
    BlendStateInfo.bIndependentBlendEnable        = false;
    BlendStateInfo.RenderTargets[0].bBlendEnable  = true;
    BlendStateInfo.RenderTargets[0].SrcBlend      = EBlendType ::SrcAlpha;
    BlendStateInfo.RenderTargets[0].SrcBlendAlpha = EBlendType ::InvSrcAlpha;
    BlendStateInfo.RenderTargets[0].DstBlend      = EBlendType ::InvSrcAlpha;
    BlendStateInfo.RenderTargets[0].DstBlendAlpha = EBlendType ::Zero;
    BlendStateInfo.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
    BlendStateInfo.RenderTargets[0].BlendOp       = EBlendOp::Add;

    FRHIBlendStateRef BlendStateBlending = RHICreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    BlendStateInfo.RenderTargets[0].bBlendEnable = false;

    FRHIBlendStateRef BlendStateNoBlending = RHICreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateDesc PSOProperties;
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.VertexInputLayout                      = InputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendStateBlending.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    PipelineState = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }

    PSOProperties.BlendState = BlendStateNoBlending.Get();

    PipelineStateNoBlending = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PipelineStateNoBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHISamplerStateDesc SamplerInitializer;
    SamplerInitializer.AddressU = ESamplerMode::Clamp;
    SamplerInitializer.AddressV = ESamplerMode::Clamp;
    SamplerInitializer.AddressW = ESamplerMode::Clamp;
    SamplerInitializer.Filter   = ESamplerFilter::MinMagMipLinear;

    LinearSampler = RHICreateSamplerState(SamplerInitializer);
    if (!LinearSampler)
    {
        return false;
    }

    SamplerInitializer.Filter = ESamplerFilter::MinMagMipPoint;

    PointSampler = RHICreateSamplerState(SamplerInitializer);
    if (!PointSampler)
    {
        return false;
    }

    return true;
}

void FViewportRenderer::Render(FRHICommandList& CmdList)
{
    TSharedPtr<FViewport> Viewport = FApplication::Get().GetMainViewport();
    if (!Viewport)
    {
        return;
    }

    // Render ImgGui draw data
    ImGui::Render();

    FRHIViewport* RHIViewport = Viewport->GetRHIViewport();
    CHECK(RHIViewport != nullptr);

    // Render to the main back buffer (Which we should just load)
    FRHIRenderPassDesc RenderPassDesc({ FRHIRenderTargetView(RHIViewport->GetBackBuffer(), EAttachmentLoadAction::Load) }, 1);
    CmdList.BeginRenderPass(RenderPassDesc);

    RenderDrawData(CmdList, ImGui::GetDrawData());

    CmdList.EndRenderPass();

    // Update and Render additional Platform Windows
    ImGuiIO& IOState = ImGui::GetIO();
    if (IOState.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, reinterpret_cast<void*>(&CmdList));
    }

    for (FDrawableTexture* Image : RenderedImages)
    {
        CHECK(Image != nullptr);

        if (Image->AfterState != EResourceAccess::PixelShaderResource)
        {
            CmdList.TransitionTexture(Image->Texture.Get(), EResourceAccess::PixelShaderResource, Image->AfterState);
        }
    }

    RenderedImages.Clear();
}

void FViewportRenderer::RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FViewport* InViewport, bool bClear)
{
    FRHITexture* BackBuffer = InViewport->GetRHIViewport()->GetBackBuffer();
    CmdList.TransitionTexture(BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);

    FRHIRenderPassDesc RenderPassDesc({ FRHIRenderTargetView(BackBuffer, bClear ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load) }, 1);
    CmdList.BeginRenderPass(RenderPassDesc);

    RenderDrawData(CmdList, DrawData);

    CmdList.EndRenderPass();
    CmdList.TransitionTexture(BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);
}

void FViewportRenderer::RenderDrawData(FRHICommandList& CmdList, ImDrawData* DrawData)
{
    // Avoid rendering when minimized
    if (DrawData->DisplaySize.x <= 0.0f || DrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    // Create and grow vertex/index buffers if needed
    FViewport* Viewport = reinterpret_cast<FViewport*>(DrawData->OwnerViewport->RendererUserData);
    if (!Viewport)
    {
        return;
    }

    FViewportBuffers& Buffers = ViewportBuffers[Viewport];
    if (!Buffers.VertexBuffer || DrawData->TotalVtxCount > Buffers.VertexCount)
    {
        if (Buffers.VertexBuffer)
        {
            CmdList.DestroyResource(Buffers.VertexBuffer.ReleaseOwnership());
        }

        const uint32 NewVertexCount = DrawData->TotalVtxCount + 50000;
        FRHIBufferDesc VBDesc(sizeof(ImDrawVert) * NewVertexCount, sizeof(ImDrawVert), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewVertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (NewVertexBuffer)
        {
            NewVertexBuffer->SetName("ImGui VertexBuffer");
            Buffers.VertexBuffer = NewVertexBuffer;
            Buffers.VertexCount  = NewVertexCount;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (Buffers.IndexBuffer == nullptr || DrawData->TotalIdxCount > Buffers.IndexCount)
    {
        if (Buffers.IndexBuffer)
        {
            CmdList.DestroyResource(Buffers.IndexBuffer.ReleaseOwnership());
        }

        const uint32 NewIndexCount = DrawData->TotalIdxCount + 100000;
        FRHIBufferDesc IBDesc(sizeof(ImDrawIdx) * NewIndexCount, sizeof(ImDrawIdx), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewIndexBuffer = RHICreateBuffer(IBDesc, EResourceAccess::IndexBuffer, nullptr);
        if (NewIndexBuffer)
        {
            NewIndexBuffer->SetName("ImGui IndexBuffer");
            Buffers.IndexBuffer = NewIndexBuffer;
            Buffers.IndexCount  = NewIndexCount;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    // TODO: Do not change to GenericRead, change to Vertex/Constant-Buffer
    CmdList.TransitionBuffer(Buffers.VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(Buffers.IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    // Upload vertex/index data into a single contiguous GPU buffer
    uint64 VertexOffset = 0;
    uint64 IndexOffset  = 0;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[Index];
        CmdList.UpdateBuffer(Buffers.VertexBuffer.Get(), FBufferRegion(VertexOffset * sizeof(ImDrawVert), DrawCmdList->VtxBuffer.Size * sizeof(ImDrawVert)), DrawCmdList->VtxBuffer.Data);
        CmdList.UpdateBuffer(Buffers.IndexBuffer.Get(), FBufferRegion(IndexOffset * sizeof(ImDrawIdx), DrawCmdList->IdxBuffer.Size * sizeof(ImDrawIdx)), DrawCmdList->IdxBuffer.Data);
        
        VertexOffset += DrawCmdList->VtxBuffer.Size;
        IndexOffset  += DrawCmdList->IdxBuffer.Size;
    }

    CmdList.TransitionBuffer(Buffers.VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CmdList.TransitionBuffer(Buffers.IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);

    // Setup desired DX state
    SetupRenderState(CmdList, DrawData, Buffers);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int32 GlobalVertexOffset = 0;
    int32 GlobalIndexOffset  = 0;

    ImVec2 ClipOffset = DrawData->DisplayPos;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        // TODO: This should probably be handled differently
        bool bResetRenderState = false;

        const ImDrawList* DrawCmdList = DrawData->CmdLists[Index];
        for (int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; ++CmdIndex)
        {
            const ImDrawCmd* pDrawCommand = &DrawCmdList->CmdBuffer[CmdIndex];
            if (pDrawCommand->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pDrawCommand->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    SetupRenderState(CmdList, DrawData, Buffers);
                }
                else
                {
                    pDrawCommand->UserCallback(DrawCmdList, pDrawCommand);
                }
            }
            else
            {
                const ImTextureID TextureID = pDrawCommand->GetTexID();
                if (TextureID)
                {
                    // TODO: Change this so that the same code can be used for font texture and images
                    FDrawableTexture* DrawableTexture = reinterpret_cast<FDrawableTexture*>(TextureID);
                    RenderedImages.Emplace(DrawableTexture);

                    if (DrawableTexture->BeforeState != EResourceAccess::PixelShaderResource)
                    {
                        CmdList.TransitionTexture(DrawableTexture->Texture.Get(), DrawableTexture->BeforeState, EResourceAccess::PixelShaderResource);

                        // TODO: Another way to do this? May break somewhere?
                        DrawableTexture->BeforeState = EResourceAccess::PixelShaderResource;
                    }

                    CmdList.SetShaderResourceView(PShader.Get(), DrawableTexture->View.Get(), 0);

                    if (!DrawableTexture->bAllowBlending)
                    {
                        CmdList.SetGraphicsPipelineState(PipelineStateNoBlending.Get());
                        bResetRenderState = true;
                    }

                    if (DrawableTexture->bSamplerLinear)
                    {
                        CmdList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                    }
                    else
                    {
                        CmdList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);
                    }
                }
                else
                {
                    if (DrawCmdList->Flags & ImDrawListFlags_AntiAliasedLinesUseTex)
                    {
                        CmdList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                    }
                    else
                    {
                        CmdList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);
                    }

                    FRHIShaderResourceView* View = FontTexture->GetShaderResourceView();
                    CmdList.SetShaderResourceView(PShader.Get(), View, 0);
                }

                // Project Scissor/Clipping rectangles into Framebuffer space
                ImVec2 ClipMin(pDrawCommand->ClipRect.x - ClipOffset.x, pDrawCommand->ClipRect.y - ClipOffset.y);
                ImVec2 ClipMax(pDrawCommand->ClipRect.z - ClipOffset.x, pDrawCommand->ClipRect.w - ClipOffset.y);
                if (ClipMax.x <= ClipMin.x || ClipMax.y <= ClipMin.y)
                {
                    continue;
                }

                FRHIScissorRegion ScissorRegion(
                    pDrawCommand->ClipRect.z - ClipOffset.x,
                    pDrawCommand->ClipRect.w - ClipOffset.y,
                    pDrawCommand->ClipRect.x - ClipOffset.x,
                    pDrawCommand->ClipRect.y - ClipOffset.y);
                CmdList.SetScissorRect(ScissorRegion);

                CmdList.DrawIndexedInstanced(pDrawCommand->ElemCount, 1, pDrawCommand->IdxOffset + GlobalIndexOffset, pDrawCommand->VtxOffset + GlobalVertexOffset, 0);
            }
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }
}

void FViewportRenderer::SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportBuffers& Buffers)
{
    // Setup Orthographic Projection matrix into our Constant-Buffer
    // The visible ImGui space lies from DrawData->DisplayPos (top left) to DrawData->DisplayPos+DrawData->DisplaySize (bottom right).
    FVertexConstantBuffer VertexConstantBuffer;
    {
        float L = DrawData->DisplayPos.x;
        float R = DrawData->DisplayPos.x + DrawData->DisplaySize.x;
        float T = DrawData->DisplayPos.y;
        float B = DrawData->DisplayPos.y + DrawData->DisplaySize.y;

        float Matrix[4][4] =
        {
            { 2.0f / (R - L)   , 0.0f             , 0.0f, 0.0f },
            { 0.0f             , 2.0f / (T - B)   , 0.0f, 0.0f },
            { 0.0f             , 0.0f             , 0.5f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
        };

        FMemory::Memcpy(&VertexConstantBuffer.ViewProjectionMatrix, Matrix, sizeof(Matrix));
    }

    // Setup viewport
    FRHIViewportRegion ViewportRegion(DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.SetViewport(ViewportRegion);

    // Bind shader and vertex buffers
    const EIndexFormat IndexFormat = sizeof(ImDrawIdx) == 2 ? EIndexFormat::uint16 : EIndexFormat::uint32;
    CmdList.SetIndexBuffer(Buffers.IndexBuffer.Get(), IndexFormat);
    CmdList.SetVertexBuffers(MakeArrayView(&Buffers.VertexBuffer, 1), 0);
    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    
    CmdList.SetBlendFactor(FVector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    CmdList.Set32BitShaderConstants(PShader.Get(), &VertexConstantBuffer, 16);
}