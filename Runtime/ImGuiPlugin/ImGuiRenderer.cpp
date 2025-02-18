#include "ImGuiRenderer.h"
#include "ImGuiExtensions.h"
#include "ImGuiPlugin.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Application/Widgets/WindowWidget.h"
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"
#include <imgui.h>

struct FVertexConstantBuffer
{
    float ViewProjectionMatrix[4][4];
};

FImGuiRenderer* GImGuiRenderer = nullptr;

FImGuiRenderer::FImGuiRenderer()
    : RenderedImages()
    , FontTexture(nullptr)
    , PipelineState(nullptr)
    , PipelineStateNoBlending(nullptr)
    , PShader(nullptr)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , LinearSampler(nullptr)
    , PointSampler(nullptr)
{
    CHECK(GImGuiRenderer == nullptr);
    GImGuiRenderer = this;
}

FImGuiRenderer::~FImGuiRenderer()
{
    CHECK(GImGuiRenderer == this);
    GImGuiRenderer = nullptr;
}

bool FImGuiRenderer::InitializeRHI()
{
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    if (ImGuiExtensions::IsMultiViewportEnabled())
    {
        PlatformState.Renderer_CreateWindow = [](ImGuiViewport* Viewport)
        {
            CHECK(GImGuiRenderer != nullptr);
            GImGuiRenderer->OnCreateWindow(Viewport);
        };

        PlatformState.Renderer_DestroyWindow = [](ImGuiViewport* Viewport)
        {
            CHECK(GImGuiRenderer != nullptr);
            GImGuiRenderer->OnDestroyWindow(Viewport);
        };

        PlatformState.Renderer_SetWindowSize = [](ImGuiViewport* Viewport, ImVec2 Size)
        {
            CHECK(GImGuiRenderer != nullptr);
            GImGuiRenderer->OnSetWindowSize(Viewport, Size);
        };

        PlatformState.Renderer_RenderWindow = [](ImGuiViewport* Viewport, void* CommandList)
        {
            CHECK(GImGuiRenderer != nullptr);
            GImGuiRenderer->OnRenderWindow(Viewport, CommandList);
        };

        PlatformState.Renderer_SwapBuffers = [](ImGuiViewport* Viewport, void* CommandList)
        {
            CHECK(GImGuiRenderer != nullptr);
            GImGuiRenderer->OnSwapBuffers(Viewport, CommandList);
        };
    }
    else
    {
        PlatformState.Renderer_CreateWindow  = nullptr;
        PlatformState.Renderer_DestroyWindow = nullptr;
        PlatformState.Renderer_SetWindowSize = nullptr;
        PlatformState.Renderer_RenderWindow  = nullptr;
        PlatformState.Renderer_SwapBuffers   = nullptr;
    }

    // Build texture atlas
    uint8* Pixels = nullptr;
    int32  Width  = 0;
    int32  Height = 0;

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendRendererUserData = this;
    UIState.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    FontTexture = FTextureFactory::Get().LoadFromMemory(Pixels, Width, Height, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!FontTexture)
    {
        return false;
    }
    else
    {
        FontTexture->SetDebugName("ImGui FontTexture");
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

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromSource(VSSource, CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
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

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromSource(PSSource, CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    PShader = RHICreatePixelShader(ShaderCode);
    if (!PShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexLayoutInitializerList VertexElementList =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), 0, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  1, EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), 2, EVertexInputClass::Vertex, 0 },
    };

    FRHIVertexLayoutRef InputLayout = RHICreateVertexLayout(VertexElementList);
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

    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Clamp;
    SamplerInfo.AddressV = ESamplerMode::Clamp;
    SamplerInfo.AddressW = ESamplerMode::Clamp;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    LinearSampler = RHICreateSamplerState(SamplerInfo);
    if (!LinearSampler)
    {
        return false;
    }

    SamplerInfo.Filter = ESamplerFilter::MinMagMipPoint;

    PointSampler = RHICreateSamplerState(SamplerInfo);
    if (!PointSampler)
    {
        return false;
    }

    return true;
}

void FImGuiRenderer::ReleaseRHI()
{
    // Release all RHI textures
    FontTexture.Reset();
    PipelineState.Reset();
    PipelineStateNoBlending.Reset();
    PShader.Reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    LinearSampler.Reset();
    PointSampler.Reset();
}

void FImGuiRenderer::Render(FRHICommandList& CommandList)
{
    if (ImGuiViewport* MainViewport = ImGui::GetMainViewport())
    {
        FImGuiViewport* MainViewportData = reinterpret_cast<FImGuiViewport*>(MainViewport->RendererUserData);
        CHECK(MainViewportData != nullptr);

        FRHIViewportRef RHIViewport = MainViewportData->Viewport;
        CHECK(RHIViewport != nullptr);

        // Render
        ImGui::Render();
        
        ImDrawData* DrawData = ImGui::GetDrawData();
        PrepareDrawData(CommandList, DrawData);

        // Render to the main Viewport
        FRHIBeginRenderPassInfo RenderPassDesc({ FRHIRenderTargetView(RHIViewport->GetBackBuffer(), EAttachmentLoadAction::Load) }, 1);
        CommandList.BeginRenderPass(RenderPassDesc);
        RenderDrawData(CommandList, DrawData);
        CommandList.EndRenderPass();

        ImGuiIO& IOState = ImGui::GetIO();
        if (IOState.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, reinterpret_cast<void*>(&CommandList));
        }

        for (FImGuiTexture* Image : RenderedImages)
        {
            CHECK(Image != nullptr);
            if (Image->AfterState != EResourceAccess::PixelShaderResource)
            {
                CommandList.TransitionTexture(Image->Texture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, Image->AfterState));
            }
        }

        RenderedImages.Clear();
    }
}

void FImGuiRenderer::RenderViewport(FRHICommandList& CommandList, ImDrawData* DrawData, FImGuiViewport& ViewportData, bool bClear)
{
    FRHITexture* BackBuffer = ViewportData.Viewport->GetBackBuffer();
    CommandList.TransitionTexture(BackBuffer, FRHITextureTransition::Make(EResourceAccess::Present, EResourceAccess::RenderTarget));

    PrepareDrawData(CommandList, DrawData);

    FRHIBeginRenderPassInfo RenderPassDesc({ FRHIRenderTargetView(BackBuffer, bClear ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load) }, 1);
    CommandList.BeginRenderPass(RenderPassDesc);
    
    RenderDrawData(CommandList, DrawData);
    
    CommandList.EndRenderPass();

    CommandList.TransitionTexture(BackBuffer, FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::Present));
}

void FImGuiRenderer::PrepareDrawData(FRHICommandList& CommandList, ImDrawData* DrawData)
{
    if (DrawData->DisplaySize.x <= 0.0f || DrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(DrawData->OwnerViewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    if (!ViewportData->VertexBuffer || DrawData->TotalVtxCount > ViewportData->VertexCount)
    {
        const uint32 NewVertexCount = DrawData->TotalVtxCount + 50000;
        FRHIBufferInfo VBInfo(sizeof(ImDrawVert) * NewVertexCount, sizeof(ImDrawVert), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::GenericRead, nullptr);
        if (NewVertexBuffer)
        {
            NewVertexBuffer->SetDebugName("ImGui VertexBuffer");
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
        const uint32 NewIndexCount = DrawData->TotalIdxCount + 100000;
        FRHIBufferInfo IBInfo(sizeof(ImDrawIdx) * NewIndexCount, sizeof(ImDrawIdx), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);

        TSharedRef<FRHIBuffer> NewIndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::GenericRead, nullptr);
        if (NewIndexBuffer)
        {
            NewIndexBuffer->SetDebugName("ImGui IndexBuffer");
            ViewportData->IndexBuffer = NewIndexBuffer;
            ViewportData->IndexCount  = NewIndexCount;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    CommandList.TransitionBuffer(ViewportData->VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ViewportData->IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    uint64 VertexOffset = 0;
    uint64 IndexOffset  = 0;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[Index];
        CommandList.UpdateBuffer(ViewportData->VertexBuffer.Get(), FBufferRegion(VertexOffset * sizeof(ImDrawVert), DrawCmdList->VtxBuffer.Size * sizeof(ImDrawVert)), DrawCmdList->VtxBuffer.Data);
        CommandList.UpdateBuffer(ViewportData->IndexBuffer.Get(), FBufferRegion(IndexOffset * sizeof(ImDrawIdx), DrawCmdList->IdxBuffer.Size * sizeof(ImDrawIdx)), DrawCmdList->IdxBuffer.Data);
        
        VertexOffset += DrawCmdList->VtxBuffer.Size;
        IndexOffset  += DrawCmdList->IdxBuffer.Size;
    }

    CommandList.TransitionBuffer(ViewportData->VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CommandList.TransitionBuffer(ViewportData->IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
}

void FImGuiRenderer::RenderDrawData(FRHICommandList& CommandList, ImDrawData* DrawData)
{
    int32 FramebufferWidth  = static_cast<int32>(DrawData->DisplaySize.x * DrawData->FramebufferScale.x);
    int32 FramebufferHeight = static_cast<int32>(DrawData->DisplaySize.y * DrawData->FramebufferScale.y);
    
    if (FramebufferWidth <= 0 || FramebufferHeight <= 0)
    {
        return;
    }

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(DrawData->OwnerViewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    SetupRenderState(CommandList, DrawData, *ViewportData);

    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int32 GlobalVertexOffset = 0;
    int32 GlobalIndexOffset  = 0;

    ImVec2 ClipOffset = DrawData->DisplayPos;
    ImVec2 ClipScale  = DrawData->FramebufferScale;
    
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
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state)
                // User callback, registered via ImDrawList::AddCallback()
                if (bResetRenderState || DrawCommand->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    SetupRenderState(CommandList, DrawData, *ViewportData);
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
                    FImGuiTexture* DrawableTexture = reinterpret_cast<FImGuiTexture*>(TextureID);
                    RenderedImages.Emplace(DrawableTexture);

                    if (DrawableTexture->BeforeState != EResourceAccess::PixelShaderResource)
                    {
                        CommandList.TransitionTexture(DrawableTexture->Texture.Get(), FRHITextureTransition::Make(DrawableTexture->BeforeState, EResourceAccess::PixelShaderResource));
                        
                        // TODO: Another way to do this? Maybe breaks somewhere?
                        DrawableTexture->BeforeState = EResourceAccess::PixelShaderResource;
                    }

                    if (!DrawableTexture->bAllowBlending)
                    {
                        CommandList.SetGraphicsPipelineState(PipelineStateNoBlending.Get());
                    }
                    else
                    {
                        CommandList.SetGraphicsPipelineState(PipelineState.Get());
                    }

                    if (DrawableTexture->bSamplerLinear)
                    {
                        CommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                    }
                    else
                    {
                        CommandList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);
                    }

                    CommandList.SetShaderResourceView(PShader.Get(), DrawableTexture->View.Get(), 0);
                }
                else
                {
                    if (bResetRenderState)
                    {
                        SetupRenderState(CommandList, DrawData, *ViewportData);
                        bResetRenderState = false;
                    }

                    CommandList.SetGraphicsPipelineState(PipelineState.Get());

                    if (DrawCmdList->Flags & ImDrawListFlags_AntiAliasedLinesUseTex)
                    {
                        CommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                    }
                    else
                    {
                        CommandList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);
                    }

                    FRHIShaderResourceView* View = FontTexture->GetShaderResourceView();
                    CommandList.SetShaderResourceView(PShader.Get(), View, 0);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 ClipMin((DrawCommand->ClipRect.x - ClipOffset.x), (DrawCommand->ClipRect.y - ClipOffset.y));
                ImVec2 ClipMax((DrawCommand->ClipRect.z - ClipOffset.x), (DrawCommand->ClipRect.w - ClipOffset.y));

                if (ClipMin.x < 0.0f)
                {
                    ClipMin.x = 0.0f;
                }
                if (ClipMin.y < 0.0f)
                {
                    ClipMin.y = 0.0f;
                }
                
                if (ClipMax.x > FramebufferWidth)
                {
                    ClipMax.x = static_cast<float>(FramebufferWidth);
                }
                if (ClipMax.y > FramebufferHeight)
                {
                    ClipMax.y = static_cast<float>(FramebufferHeight);
                }
                
                if (ClipMax.x <= ClipMin.x || ClipMax.y <= ClipMin.y)
                {
                    continue;
                }
                
                const FScissorRegion ScissorRegion(ClipMax.x - ClipMin.x, ClipMax.y - ClipMin.y, ClipMin.x, ClipMin.y);
                CommandList.SetScissorRect(ScissorRegion);

                CommandList.DrawIndexedInstanced(DrawCommand->ElemCount, 1, DrawCommand->IdxOffset + GlobalIndexOffset, DrawCommand->VtxOffset + GlobalVertexOffset, 0);
            }
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }
}

void FImGuiRenderer::SetupRenderState(FRHICommandList& CommandList, ImDrawData* DrawData, FImGuiViewport& Buffers)
{
    int32 FramebufferWidth  = static_cast<int32>(DrawData->DisplaySize.x * DrawData->FramebufferScale.x);
    int32 FramebufferHeight = static_cast<int32>(DrawData->DisplaySize.y * DrawData->FramebufferScale.y);
    
    // Setup Orthographic Projection matrix into our Constant-Buffer
    // The visible ImGui space lies from DrawData->DisplayPos (top left)
    // to DrawData->DisplayPos+DrawData->DisplaySize (bottom right).
    float L = DrawData->DisplayPos.x;
    float R = DrawData->DisplayPos.x + FramebufferWidth;
    float T = DrawData->DisplayPos.y;
    float B = DrawData->DisplayPos.y + FramebufferHeight;

    float Matrix[4][4] =
    {
        { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
        { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
        { 0.0f,              0.0f,              0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };

    FVertexConstantBuffer VertexConstantBuffer;
    FMemory::Memcpy(&VertexConstantBuffer.ViewProjectionMatrix, Matrix, sizeof(Matrix));

    FViewportRegion ViewportRegion(static_cast<float>(FramebufferWidth), static_cast<float>(FramebufferHeight), 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    const EIndexFormat IndexFormat = sizeof(ImDrawIdx) == 2 ? EIndexFormat::uint16 : EIndexFormat::uint32;
    CommandList.SetIndexBuffer(Buffers.IndexBuffer.Get(), IndexFormat);
    CommandList.SetVertexBuffers(MakeArrayView(&Buffers.VertexBuffer, 1), 0);
    
    CommandList.SetBlendFactor(FVector4{ 0.0f, 0.0f, 0.0f, 0.0f });

    CommandList.Set32BitShaderConstants(PShader.Get(), &VertexConstantBuffer, 16);
}

void FImGuiRenderer::OnCreateWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    FRHIViewportInfo ViewportInfo;
    ViewportInfo.WindowHandle = PlatformWindow->GetPlatformHandle();
    ViewportInfo.ColorFormat  = EFormat::B8G8R8A8_Unorm;
    ViewportInfo.Width        = static_cast<uint16>(Viewport->Size.x);
    ViewportInfo.Height       = static_cast<uint16>(Viewport->Size.y);
        
    ViewportData->Viewport = RHICreateViewport(ViewportInfo);
    if (ViewportData->Viewport)
    {
        ViewportData->Width  = ViewportInfo.Width;
        ViewportData->Height = ViewportInfo.Height;
        Viewport->RendererUserData = Viewport->PlatformUserData;
    }
}

void FImGuiRenderer::OnDestroyWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    Viewport->RendererUserData = nullptr;
}

void FImGuiRenderer::OnSetWindowSize(ImGuiViewport*, ImVec2)
{
}

void FImGuiRenderer::OnRenderWindow(ImGuiViewport* Viewport, void* CommandList)
{
    FRHICommandList* RHICommandList = reinterpret_cast<FRHICommandList*>(CommandList);
    CHECK(RHICommandList != nullptr);

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    const ImVec2 ViewportSize = Viewport->Size;
    if (static_cast<uint16>(ViewportSize.x) != ViewportData->Width || static_cast<uint16>(ViewportSize.y) != ViewportData->Height)
    {
        ViewportData->Width  = static_cast<uint16>(ViewportSize.x);
        ViewportData->Height = static_cast<uint16>(ViewportSize.y);

        FRHIViewport* RHIViewport = ViewportData->Viewport.Get();
        RHICommandList->ResizeViewport(RHIViewport, ViewportData->Width, ViewportData->Height);
    }
    
    const bool bClear = (Viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0;
    RenderViewport(*RHICommandList, Viewport->DrawData, *ViewportData, bClear);
}

void FImGuiRenderer::OnSwapBuffers(ImGuiViewport* Viewport, void* CommandList)
{
    FRHICommandList* RHICommandList = reinterpret_cast<FRHICommandList*>(CommandList);
    CHECK(RHICommandList != nullptr);

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    bool bEnableVsync = false;
    if (IConsoleVariable* CVarVSyncEnabled = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.VerticalSync"))
    {
        bEnableVsync = CVarVSyncEnabled->GetBool();
    }

    RHICommandList->PresentViewport(ViewportData->Viewport.Get(), bEnableVsync);
}
