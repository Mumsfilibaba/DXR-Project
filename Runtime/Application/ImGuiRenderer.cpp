#include "ImGuiRenderer.h"
#include "Application.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"

#include <imgui.h>

struct FVertexConstantBuffer
{
    float ViewProjectionMatrix[4][4];
};

static void ImGuiCreateWindow(ImGuiViewport* InViewport)
{
    if (TSharedRef<FGenericWindow> Window = MakeSharedRef<FGenericWindow>(reinterpret_cast<FGenericWindow*>(InViewport->PlatformUserData)))
    {
        FRHIViewportDesc ViewportDesc;
        ViewportDesc.WindowHandle = Window->GetPlatformHandle();
        ViewportDesc.ColorFormat  = EFormat::B8G8R8A8_Unorm;
        ViewportDesc.Width        = static_cast<uint16>(InViewport->Size.x);
        ViewportDesc.Height       = static_cast<uint16>(InViewport->Size.y);
        
        FRHIViewportRef NewViewport = RHICreateViewport(ViewportDesc);
        if (NewViewport)
        {
            FViewportData* ViewportData = new FViewportData;
            ViewportData->Viewport = NewViewport;

            InViewport->RendererUserData = ViewportData;
        }
    }
}

static void ImGuiDestroyWindow(ImGuiViewport* Viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
    if (FViewportData* ViewportData = reinterpret_cast<FViewportData*>(Viewport->RendererUserData))
    {
        GRHICommandExecutor.WaitForGPU();
        delete ViewportData;
    }

    Viewport->RendererUserData = nullptr;
}

static void ImGuiSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    if (FViewportData* ViewportData = reinterpret_cast<FViewportData*>(Viewport->RendererUserData))
    {
        if (ViewportData->Viewport->GetWidth() != Size.x || ViewportData->Viewport->GetHeight() != Size.y)
        {
            ViewportData->bDidResize = true;
            ViewportData->Width  = static_cast<uint16>(Size.x);
            ViewportData->Height = static_cast<uint16>(Size.y);
        }
    }
}

static void ImGuiRenderWindow(ImGuiViewport* Viewport, void* CmdList)
{
    FRHICommandList* RHICmdList = reinterpret_cast<FRHICommandList*>(CmdList);
    if (!RHICmdList)
    {
        return;
    }

    FViewportData* ViewportData = reinterpret_cast<FViewportData*>(Viewport->RendererUserData);
    if (!ViewportData)
    {
        return;
    }

    if (FApplication::IsInitialized())
    {
        if (FImGuiRenderer* Renderer = FApplication::Get().GetRenderer())
        {
            if (ViewportData->bDidResize)
            {
                RHICmdList->ResizeViewport(ViewportData->Viewport.Get(), ViewportData->Width, ViewportData->Height);

                ViewportData->bDidResize = false;
                ViewportData->Width  = 0;
                ViewportData->Height = 0;
            }
            
            const bool bClear = (Viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0;
            Renderer->RenderViewport(*RHICmdList, Viewport->DrawData, *ViewportData, bClear);
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

    if (FViewportData* ViewportData = reinterpret_cast<FViewportData*>(Viewport->RendererUserData))
    {
        if (FRHICommandList* RHICmdList = reinterpret_cast<FRHICommandList*>(CmdList))
        {
            RHICmdList->PresentViewport(ViewportData->Viewport.Get(), bEnableVsync);
        }
    }
}

FImGuiRenderer::~FImGuiRenderer()
{
    // Release the MainViewport
    FImGui::SetMainViewport(nullptr);
}


bool FImGuiRenderer::Initialize()
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
    else
    {
        FontTexture->SetName("ImGui FontTexture");
    }

    const CHAR* VSSource =
        R"*(
        struct FShaderConstants
        {
            float4x4 ProjectionMatrix;
        };
        
        #if SHADER_LANG == SHADER_LANG_SPIRV
            [[vk::push_constant]]
            FShaderConstants Constants;
        #else
            ConstantBuffer<FShaderConstants> Constants : register(b0, space1);
        #endif

        struct FVSInput
        {
            float2 Position : POSITION;
            float2 TexCoord : TEXCOORD0;
            float4 Color    : COLOR0;
        };

        struct FPSInput
        {
            float4 Position : SV_POSITION;
            float4 Color    : COLOR0;
            float2 TexCoord : TEXCOORD0;
        };

        FPSInput Main(FVSInput Input)
        {
            FPSInput Output;
            Output.Position = mul(Constants.ProjectionMatrix, float4(Input.Position.xy, 0.0f, 1.0f));
            Output.Color    = Input.Color;
            Output.TexCoord = Input.TexCoord;
            return Output;
        })*";

    TArray<uint8> ShaderCode;

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
        if (!FShaderCompiler::Get().CompileFromSource(VSSource, CompileInfo, ShaderCode))
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
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FShaderCompiler::Get().CompileFromSource(PSSource, CompileInfo, ShaderCode))
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

    FRHIVertexInputLayoutInitializer InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  EVertexInputClass::Vertex, 0 },
    };

    FRHIVertexInputLayoutRef InputLayout = RHICreateVertexInputLayout(InputLayoutInfo);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilStateInfo;
    DepthStencilStateInfo.bDepthEnable      = false;
    DepthStencilStateInfo.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInitializer;
    RasterizerStateInitializer.CullMode               = ECullMode::None;
    RasterizerStateInitializer.bAntialiasedLineEnable = true;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.bIndependentBlendEnable        = false;
    BlendStateInitializer.NumRenderTargets               = 1;
    BlendStateInitializer.RenderTargets[0].bBlendEnable  = true;
    BlendStateInitializer.RenderTargets[0].SrcBlend      = EBlendType::SrcAlpha;
    BlendStateInitializer.RenderTargets[0].SrcBlendAlpha = EBlendType::InvSrcAlpha;
    BlendStateInitializer.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
    BlendStateInitializer.RenderTargets[0].DstBlendAlpha = EBlendType::Zero;
    BlendStateInitializer.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
    BlendStateInitializer.RenderTargets[0].BlendOp       = EBlendOp::Add;

    FRHIBlendStateRef BlendStateBlending = RHICreateBlendState(BlendStateInitializer);
    if (!BlendStateBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    BlendStateInitializer.RenderTargets[0].bBlendEnable = false;

    FRHIBlendStateRef BlendStateNoBlending = RHICreateBlendState(BlendStateInitializer);
    if (!BlendStateBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOProperties;
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.VertexInputLayout                      = InputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendStateBlending.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::B8G8R8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;

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

void FImGuiRenderer::Render(FRHICommandList& CmdList)
{
    TSharedPtr<FViewport> Viewport = FApplication::Get().GetMainViewport();
    if (!Viewport)
    {
        return;
    }

    // Render ImgGui draw data
    ImGui::Render();

    FRHIViewportRef RHIViewport = Viewport->GetRHIViewport();
    CHECK(RHIViewport != nullptr);

    // Prepare data before drawing
    ImDrawData* DrawData = ImGui::GetDrawData();
    PrepareDrawData(CmdList, DrawData);

    // Render to the main back buffer (Which we should just load)
    FRHIRenderPassDesc RenderPassDesc({ FRHIRenderTargetView(RHIViewport->GetBackBuffer(), EAttachmentLoadAction::Load) }, 1);
    CmdList.BeginRenderPass(RenderPassDesc);

    // Draw the ImGui data
    RenderDrawData(CmdList, DrawData);

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

void FImGuiRenderer::RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportData& ViewportData, bool bClear)
{
    FRHITexture* BackBuffer = ViewportData.Viewport->GetBackBuffer();
    CmdList.TransitionTexture(BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);

    // Prepare data before drawing
    PrepareDrawData(CmdList, DrawData);

    // Begin renderpass (All transfers has to be done before starting the renderpass)
    FRHIRenderPassDesc RenderPassDesc({ FRHIRenderTargetView(BackBuffer, bClear ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load) }, 1);
    CmdList.BeginRenderPass(RenderPassDesc);

    // Draw the data
    RenderDrawData(CmdList, DrawData);

    CmdList.EndRenderPass();
    CmdList.TransitionTexture(BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);
}

void FImGuiRenderer::PrepareDrawData(FRHICommandList& CmdList, ImDrawData* DrawData)
{
    // Avoid rendering when minimized
    if (DrawData->DisplaySize.x <= 0.0f || DrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    // Create and grow vertex/index buffers if needed
    FViewportData* ViewportData = reinterpret_cast<FViewportData*>(DrawData->OwnerViewport->RendererUserData);
    if (!ViewportData)
    {
        return;
    }

    if (!ViewportData->VertexBuffer || DrawData->TotalVtxCount > ViewportData->VertexCount)
    {
        if (ViewportData->VertexBuffer)
        {
            CmdList.DestroyResource(ViewportData->VertexBuffer.ReleaseOwnership());
        }

        const uint32 NewVertexCount = DrawData->TotalVtxCount + 50000;
        FRHIBufferDesc VBDesc(sizeof(ImDrawVert) * NewVertexCount, sizeof(ImDrawVert), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewVertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::GenericRead, nullptr);
        if (NewVertexBuffer)
        {
            NewVertexBuffer->SetName("ImGui VertexBuffer");
            ViewportData->VertexBuffer = NewVertexBuffer;
            ViewportData->VertexCount  = NewVertexCount;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!ViewportData->IndexBuffer || DrawData->TotalIdxCount > ViewportData->IndexCount)
    {
        if (ViewportData->IndexBuffer)
        {
            CmdList.DestroyResource(ViewportData->IndexBuffer.ReleaseOwnership());
        }

        const uint32 NewIndexCount = DrawData->TotalIdxCount + 100000;
        FRHIBufferDesc IBDesc(sizeof(ImDrawIdx) * NewIndexCount, sizeof(ImDrawIdx), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewIndexBuffer = RHICreateBuffer(IBDesc, EResourceAccess::GenericRead, nullptr);
        if (NewIndexBuffer)
        {
            NewIndexBuffer->SetName("ImGui IndexBuffer");
            ViewportData->IndexBuffer = NewIndexBuffer;
            ViewportData->IndexCount  = NewIndexCount;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    // TODO: Do not change to GenericRead, change to Vertex/Constant-Buffer
    CmdList.TransitionBuffer(ViewportData->VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(ViewportData->IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    // Upload vertex/index data into a single contiguous GPU buffer
    uint64 VertexOffset = 0;
    uint64 IndexOffset  = 0;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[Index];
        CmdList.UpdateBuffer(ViewportData->VertexBuffer.Get(), FBufferRegion(VertexOffset * sizeof(ImDrawVert), DrawCmdList->VtxBuffer.Size * sizeof(ImDrawVert)), DrawCmdList->VtxBuffer.Data);
        CmdList.UpdateBuffer(ViewportData->IndexBuffer.Get(), FBufferRegion(IndexOffset * sizeof(ImDrawIdx), DrawCmdList->IdxBuffer.Size * sizeof(ImDrawIdx)), DrawCmdList->IdxBuffer.Data);
        
        VertexOffset += DrawCmdList->VtxBuffer.Size;
        IndexOffset  += DrawCmdList->IdxBuffer.Size;
    }

    CmdList.TransitionBuffer(ViewportData->VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CmdList.TransitionBuffer(ViewportData->IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
}

void FImGuiRenderer::RenderDrawData(FRHICommandList& CmdList, ImDrawData* DrawData)
{
    // Avoid rendering when minimized
    if (DrawData->DisplaySize.x <= 0.0f || DrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    // Create and grow vertex/index buffers if needed
    FViewportData* ViewportData = reinterpret_cast<FViewportData*>(DrawData->OwnerViewport->RendererUserData);
    if (!ViewportData)
    {
        return;
    }

    // Setup desired render state
    SetupRenderState(CmdList, DrawData, *ViewportData);

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
            const ImDrawCmd* DrawCommand = &DrawCmdList->CmdBuffer[CmdIndex];
            if (DrawCommand->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (bResetRenderState || DrawCommand->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    SetupRenderState(CmdList, DrawData, *ViewportData);
                }
                else
                {
                    DrawCommand->UserCallback(DrawCmdList, DrawCommand);
                }
            }
            else
            {
                const ImTextureID TextureID = DrawCommand->GetTexID();
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
                    }
                    else
                    {
                        CmdList.SetGraphicsPipelineState(PipelineState.Get());
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
                    if (bResetRenderState)
                    {
                        SetupRenderState(CmdList, DrawData, *ViewportData);
                        bResetRenderState = false;
                    }

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
                    CmdList.SetGraphicsPipelineState(PipelineState.Get());
                }

                // Project Scissor/Clipping rectangles into Framebuffer space
                ImVec2 ClipMin(DrawCommand->ClipRect.x - ClipOffset.x, DrawCommand->ClipRect.y - ClipOffset.y);
                ImVec2 ClipMax(DrawCommand->ClipRect.z - ClipOffset.x, DrawCommand->ClipRect.w - ClipOffset.y);
                if (ClipMax.x <= ClipMin.x || ClipMax.y <= ClipMin.y)
                {
                    continue;
                }

                FRHIScissorRegion ScissorRegion(
                    DrawCommand->ClipRect.z - ClipOffset.x,
                    DrawCommand->ClipRect.w - ClipOffset.y,
                    DrawCommand->ClipRect.x - ClipOffset.x,
                    DrawCommand->ClipRect.y - ClipOffset.y);
                CmdList.SetScissorRect(ScissorRegion);

                CmdList.DrawIndexedInstanced(DrawCommand->ElemCount, 1, DrawCommand->IdxOffset + GlobalIndexOffset, DrawCommand->VtxOffset + GlobalVertexOffset, 0);
            }
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }
}

void FImGuiRenderer::SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FViewportData& Buffers)
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
    
    CmdList.SetBlendFactor(FVector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    CmdList.Set32BitShaderConstants(PShader.Get(), &VertexConstantBuffer, 16);
}
