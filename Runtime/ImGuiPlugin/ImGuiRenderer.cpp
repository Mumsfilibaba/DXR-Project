#include "ImGuiRenderer.h"
#include "ImGuiExtensions.h"
#include "ImGuiPlugin.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Application/Widgets/Window.h"
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"
#include <imgui.h>

struct FVertexConstantBuffer
{
    float ViewProjectionMatrix[4][4];
};

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
}

FImGuiRenderer::~FImGuiRenderer()
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendRendererUserData = nullptr;
}

bool FImGuiRenderer::Initialize()
{
    ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
    if (ImGuiExtensions::IsMultiViewportEnabled())
    {
        PlatformState.Renderer_CreateWindow  = &FImGuiRenderer::StaticCreateWindow;
        PlatformState.Renderer_DestroyWindow = &FImGuiRenderer::StaticDestroyWindow;
        PlatformState.Renderer_SetWindowSize = &FImGuiRenderer::StaticSetWindowSize;
        PlatformState.Renderer_RenderWindow  = &FImGuiRenderer::StaticRenderWindow;
        PlatformState.Renderer_SwapBuffers   = &FImGuiRenderer::StaticSwapBuffers;
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

    FontTexture = FTextureFactory::LoadFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
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

void FImGuiRenderer::Render(FRHICommandList& CmdList)
{
    if (ImGuiViewport* MainViewport = ImGui::GetMainViewport())
    {
        FImGuiViewport* MainViewportData = reinterpret_cast<FImGuiViewport*>(MainViewport->RendererUserData);
        CHECK(MainViewportData != nullptr);

        ImGui::Render();

        FRHIViewportRef RHIViewport = MainViewportData->Viewport;
        CHECK(RHIViewport != nullptr);

        ImDrawData* DrawData = ImGui::GetDrawData();
        PrepareDrawData(CmdList, DrawData);

        // Render to the main Viewport
        FRHIBeginRenderPassInfo RenderPassDesc({ FRHIRenderTargetView(RHIViewport->GetBackBuffer(), EAttachmentLoadAction::Load) }, 1);
        CmdList.BeginRenderPass(RenderPassDesc);
        RenderDrawData(CmdList, DrawData);
        CmdList.EndRenderPass();

        ImGuiIO& IOState = ImGui::GetIO();
        if (IOState.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, reinterpret_cast<void*>(&CmdList));
        }

        for (FImGuiTexture* Image : RenderedImages)
        {
            CHECK(Image != nullptr);

            if (Image->AfterState != EResourceAccess::PixelShaderResource)
            {
                CmdList.TransitionTexture(Image->Texture.Get(), EResourceAccess::PixelShaderResource, Image->AfterState);
            }
        }

        RenderedImages.Clear();
    }
}

void FImGuiRenderer::RenderViewport(FRHICommandList& CmdList, ImDrawData* DrawData, FImGuiViewport& ViewportData, bool bClear)
{
    FRHITexture* BackBuffer = ViewportData.Viewport->GetBackBuffer();
    CmdList.TransitionTexture(BackBuffer, EResourceAccess::Present, EResourceAccess::RenderTarget);

    PrepareDrawData(CmdList, DrawData);

    FRHIBeginRenderPassInfo RenderPassDesc({ FRHIRenderTargetView(BackBuffer, bClear ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load) }, 1);
    CmdList.BeginRenderPass(RenderPassDesc);
    RenderDrawData(CmdList, DrawData);
    CmdList.EndRenderPass();

    CmdList.TransitionTexture(BackBuffer, EResourceAccess::RenderTarget, EResourceAccess::Present);
}

void FImGuiRenderer::PrepareDrawData(FRHICommandList& CmdList, ImDrawData* DrawData)
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

    CmdList.TransitionBuffer(ViewportData->VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(ViewportData->IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

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
    if (DrawData->DisplaySize.x <= 0.0f || DrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(DrawData->OwnerViewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    SetupRenderState(CmdList, DrawData, *ViewportData);

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
                    FImGuiTexture* DrawableTexture = reinterpret_cast<FImGuiTexture*>(TextureID);
                    RenderedImages.Emplace(DrawableTexture);

                    if (DrawableTexture->BeforeState != EResourceAccess::PixelShaderResource)
                    {
                        CmdList.TransitionTexture(DrawableTexture->Texture.Get(), DrawableTexture->BeforeState, EResourceAccess::PixelShaderResource);
                        // TODO: Another way to do this? May break somewhere?
                        DrawableTexture->BeforeState = EResourceAccess::PixelShaderResource;
                    }

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

                    CmdList.SetShaderResourceView(PShader.Get(), DrawableTexture->View.Get(), 0);
                }
                else
                {
                    if (bResetRenderState)
                    {
                        SetupRenderState(CmdList, DrawData, *ViewportData);
                        bResetRenderState = false;
                    }

                    CmdList.SetGraphicsPipelineState(PipelineState.Get());

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
                ImVec2 ClipMin(DrawCommand->ClipRect.x - ClipOffset.x, DrawCommand->ClipRect.y - ClipOffset.y);
                ImVec2 ClipMax(DrawCommand->ClipRect.z - ClipOffset.x, DrawCommand->ClipRect.w - ClipOffset.y);
                if (ClipMax.x <= ClipMin.x || ClipMax.y <= ClipMin.y)
                {
                    continue;
                }

                const FScissorRegion ScissorRegion(
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

void FImGuiRenderer::SetupRenderState(FRHICommandList& CmdList, ImDrawData* DrawData, FImGuiViewport& Buffers)
{
    // Setup Orthographic Projection matrix into our Constant-Buffer
    // The visible ImGui space lies from DrawData->DisplayPos (top left) to DrawData->DisplayPos+DrawData->DisplaySize (bottom right).
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

    FVertexConstantBuffer VertexConstantBuffer;
    FMemory::Memcpy(&VertexConstantBuffer.ViewProjectionMatrix, Matrix, sizeof(Matrix));

    FViewportRegion ViewportRegion(DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.SetViewport(ViewportRegion);

    const EIndexFormat IndexFormat = sizeof(ImDrawIdx) == 2 ? EIndexFormat::uint16 : EIndexFormat::uint32;
    CmdList.SetIndexBuffer(Buffers.IndexBuffer.Get(), IndexFormat);
    CmdList.SetVertexBuffers(MakeArrayView(&Buffers.VertexBuffer, 1), 0);
    
    CmdList.SetBlendFactor(FVector4{ 0.0f, 0.0f, 0.0f, 0.0f });

    CmdList.Set32BitShaderConstants(PShader.Get(), &VertexConstantBuffer, 16);
}

void FImGuiRenderer::StaticCreateWindow(ImGuiViewport* Viewport)
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
        Viewport->RendererUserData = Viewport->PlatformUserData;
    }
}

void FImGuiRenderer::StaticDestroyWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);
    Viewport->RendererUserData = nullptr;
}

void FImGuiRenderer::StaticSetWindowSize(ImGuiViewport* Viewport, ImVec2 Size)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    if (ViewportData->Viewport->GetWidth() != Size.x || ViewportData->Viewport->GetHeight() != Size.y)
    {
        ViewportData->bDidResize = true;
        ViewportData->Width      = static_cast<uint16>(Size.x);
        ViewportData->Height     = static_cast<uint16>(Size.y);
    }
}

void FImGuiRenderer::StaticRenderWindow(ImGuiViewport* Viewport, void* CommandList)
{
    FRHICommandList* RHICommandList = reinterpret_cast<FRHICommandList*>(CommandList);
    CHECK(RHICommandList != nullptr);

    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->RendererUserData);
    CHECK(ViewportData != nullptr);

    if (ViewportData->bDidResize)
    {
        RHICommandList->ResizeViewport(ViewportData->Viewport.Get(), ViewportData->Width, ViewportData->Height);

        ViewportData->bDidResize = false;
        ViewportData->Width      = 0;
        ViewportData->Height     = 0;
    }

    const bool bClear = (Viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0;
    GImGuiPlugin->Renderer->RenderViewport(*RHICommandList, Viewport->DrawData, *ViewportData, bClear);
}

void FImGuiRenderer::StaticSwapBuffers(ImGuiViewport* Viewport, void* CommandList)
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