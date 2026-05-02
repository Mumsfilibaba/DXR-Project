#include "ImGuiRenderer.h"
#include "ImGuiExtensions.h"
#include "ImGuiPlugin.h"
#include "Core/Time/ElapsedTime.h"
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
    : RenderedTextures()
    , FontAtlas(nullptr)
    , PipelineState(nullptr)
    , PipelineStateNoBlending(nullptr)
    , PipelineStateFormat(EFormat::Unknown)
    , VShader(nullptr)
    , PShader(nullptr)
    , InputLayout(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , BlendStateBlending(nullptr)
    , BlendStateNoBlending(nullptr)
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

#ifdef EDITOR_BUILD
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
#else
    PlatformState.Renderer_CreateWindow  = nullptr;
    PlatformState.Renderer_DestroyWindow = nullptr;
    PlatformState.Renderer_SetWindowSize = nullptr;
    PlatformState.Renderer_RenderWindow  = nullptr;
    PlatformState.Renderer_SwapBuffers   = nullptr;
#endif

    ImGuiIO& State = ImGui::GetIO();
    State.BackendRendererUserData = this;

    // Create initial font atlas
    UpdateFontAtlas();

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ImGui.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    VShader = FRHI::Get()->CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }
    
    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/ImGui.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    PShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FRHIInputElementDesc> InputElements =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), 0, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  1, EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(ImDrawVert), 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), 2, EVertexInputClass::Vertex, 0 },
    };

    InputLayout = FRHI::Get()->CreateInputLayout(InputElements);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode               = ECullMode::None;
    RasterizerStateDesc.bAntialiasedLineEnable = true;

    RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
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
    BlendStateDesc.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
    BlendStateDesc.RenderTargets[0].BlendOp       = EBlendOp::Add;

    BlendStateBlending = FRHI::Get()->CreateBlendState(BlendStateDesc);
    if (!BlendStateBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    BlendStateDesc.RenderTargets[0].bBlendEnable = false;

    BlendStateNoBlending = FRHI::Get()->CreateBlendState(BlendStateDesc);
    if (!BlendStateNoBlending)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    LinearSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!LinearSampler)
    {
        return false;
    }

    SamplerDesc.Filter = ESamplerFilter::MinMagMipPoint;

    PointSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!PointSampler)
    {
        return false;
    }

    return true;
}

void FImGuiRenderer::ReleaseRHI()
{
    // Release all RHI textures
    FontAtlas.Reset();
    PipelineState.Reset();
    PipelineStateNoBlending.Reset();
    PipelineStateFormat = EFormat::Unknown;
    VShader.Reset();
    PShader.Reset();
    InputLayout.Reset();
    DepthStencilState.Reset();
    RasterizerState.Reset();
    BlendStateBlending.Reset();
    BlendStateNoBlending.Reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    LinearSampler.Reset();
    PointSampler.Reset();
}

bool FImGuiRenderer::UpdateFontAtlas()
{
    if (FontAtlas)
    {
        FontAtlas.Reset();
    }

	// Build texture atlas
	uint8* Pixels = nullptr;
	int32  Width  = 0;
	int32  Height = 0;

	// Ensure the default font is in the atlas
    ImGuiIO& State = ImGui::GetIO();
	State.Fonts->Build();
	State.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

	FontAtlas = FTextureFactory::Get().LoadFromMemory(Pixels, Width, Height, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
	if (!FontAtlas)
	{
		return false;
	}
	else
	{
		FontAtlas->SetDebugName("ImGui FontTexture");
	}

    // TODO: We need to uncomment below, but this requires changes to the renderer loop so keep avoiding this for now. 
    // State.Fonts->SetTexID((ImTextureID)FontAtlas.Get());
    
    return true;
}

void FImGuiRenderer::PreparePipelineState(EFormat OutputFormat)
{
    if (PipelineState && PipelineStateNoBlending && PipelineStateFormat == OutputFormat)
    {
        return;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.VertexShader                                   = VShader.Get();
    PSODesc.PixelShader                                    = PShader.Get();
    PSODesc.InputLayout                                    = InputLayout.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.BlendState                                     = BlendStateBlending.Get();
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    FRHIGraphicsPipelineStateRef NewBlending = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!NewBlending)
    {
        DEBUG_BREAK();
        return;
    }

    PSODesc.BlendState = BlendStateNoBlending.Get();

    FRHIGraphicsPipelineStateRef NewNoBlending = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!NewNoBlending)
    {
        DEBUG_BREAK();
        return;
    }

    PipelineState           = NewBlending;
    PipelineStateNoBlending = NewNoBlending;
    PipelineStateFormat     = OutputFormat;
}

void FImGuiRenderer::Render(FRHICommandList& CommandList)
{
    if (ImGuiViewport* MainViewport = ImGui::GetMainViewport())
    {
        FImGuiViewport* MainViewportData = reinterpret_cast<FImGuiViewport*>(MainViewport->RendererUserData);
        CHECK(MainViewportData != nullptr);

        FRHISwapChainRef RHISwapChain = MainViewportData->SwapChain;
        CHECK(RHISwapChain != nullptr);

        PreparePipelineState(RHISwapChain->GetColorFormat());

        // Render
        ImGui::Render();
        
        ImDrawData* DrawData = ImGui::GetDrawData();
        PrepareDrawData(CommandList, DrawData);
        PrepareTexturesForShaderResourceUsage(CommandList, DrawData);

        FRHIRenderTargetView* BackBufferRTV = RHISwapChain->GetBackBufferRenderTargetView();
        FRHIBeginRenderPassDesc RenderPassDesc({ FRHIRenderPassAttachment(BackBufferRTV, EAttachmentLoadAction::Load) }, 1);
        CommandList.BeginRenderPass(RenderPassDesc);
        
        RenderDrawData(CommandList, DrawData);

        CommandList.EndRenderPass();

        ImGuiIO& IOState = ImGui::GetIO();
        if (IOState.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, reinterpret_cast<void*>(&CommandList));
        }

        RenderedTextures.Clear();
    }
}

void FImGuiRenderer::RenderViewport(FRHICommandList& CommandList, ImDrawData* DrawData, FImGuiViewport& ViewportData, bool bClear)
{
    FRHITexture* BackBuffer = ViewportData.SwapChain->GetBackBuffer();
    CommandList.TransitionTextureState(BackBuffer, FRHITextureTransition::Make(EResourceAccess::Present, EResourceAccess::RenderTarget));

    PreparePipelineState(ViewportData.SwapChain->GetColorFormat());
    PrepareDrawData(CommandList, DrawData);

    FRHIRenderTargetView* BackBufferRTV = ViewportData.SwapChain->GetBackBufferRenderTargetView();
    FRHIBeginRenderPassDesc RenderPassDesc({ FRHIRenderPassAttachment(BackBufferRTV, bClear ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load) }, 1);
    
    CommandList.BeginRenderPass(RenderPassDesc);
    RenderDrawData(CommandList, DrawData);
    CommandList.EndRenderPass();
    
    CommandList.TransitionTextureState(BackBuffer, FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::Present));
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

        FRHIBufferDesc VertexBufferDesc;
        VertexBufferDesc.Stride = sizeof(ImDrawVert);
        VertexBufferDesc.Size   = VertexBufferDesc.Stride * NewVertexCount;
        VertexBufferDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

        TSharedRef<FRHIBuffer> NewVertexBuffer = FRHI::Get()->CreateBuffer(VertexBufferDesc, EResourceAccess::GenericRead, nullptr);
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

        FRHIBufferDesc IndexBufferDesc;
        IndexBufferDesc.Stride = sizeof(ImDrawIdx);
        IndexBufferDesc.Size   = IndexBufferDesc.Stride * NewIndexCount;
        IndexBufferDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

        TSharedRef<FRHIBuffer> NewIndexBuffer = FRHI::Get()->CreateBuffer(IndexBufferDesc, EResourceAccess::GenericRead, nullptr);
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

    CommandList.TransitionBufferState(ViewportData->VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CommandList.TransitionBufferState(ViewportData->IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    uint64 VertexOffset = 0;
    uint64 IndexOffset  = 0;

    for (int32 i = 0; i < DrawData->CmdListsCount; ++i)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
        CommandList.UpdateBuffer(ViewportData->VertexBuffer.Get(), FBufferRegion(VertexOffset * sizeof(ImDrawVert), DrawCmdList->VtxBuffer.Size * sizeof(ImDrawVert)), DrawCmdList->VtxBuffer.Data);
        CommandList.UpdateBuffer(ViewportData->IndexBuffer.Get(), FBufferRegion(IndexOffset * sizeof(ImDrawIdx), DrawCmdList->IdxBuffer.Size * sizeof(ImDrawIdx)), DrawCmdList->IdxBuffer.Data);
        
        VertexOffset += DrawCmdList->VtxBuffer.Size;
        IndexOffset  += DrawCmdList->IdxBuffer.Size;
    }

    CommandList.TransitionBufferState(ViewportData->VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CommandList.TransitionBufferState(ViewportData->IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
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
    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset  = 0;
    ImVec2 ClipOffset = DrawData->DisplayPos;
    ImVec2 ClipScale  = DrawData->FramebufferScale;
    
    for (int32 i = 0; i < DrawData->CmdListsCount; ++i)
    {
        // TODO: This should probably be handled differently
        bool bResetRenderState = false;

        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
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
                    const FImGuiTexture* DrawableTexture = reinterpret_cast<const FImGuiTexture*>(TextureID);
                    if (!DrawableTexture->bEnableBlending)
                    {
                        CommandList.SetGraphicsPipelineState(PipelineStateNoBlending.Get());
                    }
                    else
                    {
                        CommandList.SetGraphicsPipelineState(PipelineState.Get());
                    }

                    if (DrawableTexture->bEnableLinearSampler)
                    {
                        CommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                    }
                    else
                    {
                        CommandList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);
                    }

                    CommandList.SetShaderResourceView(PShader.Get(), DrawableTexture->ShaderResourceView.Get(), 0);
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

                    FRHIShaderResourceView* View = FontAtlas->GetShaderResourceView();
                    CommandList.SetShaderResourceView(PShader.Get(), View, 0);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 ClipMin = ImVec2((DrawCommand->ClipRect.x - ClipOffset.x), (DrawCommand->ClipRect.y - ClipOffset.y));
                ImVec2 ClipMax = ImVec2((DrawCommand->ClipRect.z - ClipOffset.x), (DrawCommand->ClipRect.w - ClipOffset.y));

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

    CommandList.SetShaderConstants(PShader.Get(), &VertexConstantBuffer, 16);
}

void FImGuiRenderer::PrepareTexturesForShaderResourceUsage(FRHICommandList& CommandList, ImDrawData* DrawData)
{
	for (int32 i = 0; i < DrawData->CmdListsCount; ++i)
	{
		const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
		for (int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; ++CmdIndex)
		{
			const ImDrawCmd* DrawCommand = &DrawCmdList->CmdBuffer[CmdIndex];
			if (const ImTextureID TextureID = DrawCommand->GetTexID())
			{
				const FImGuiTexture* DrawableTexture = reinterpret_cast<const FImGuiTexture*>(TextureID);
				PrepareTextureForShaderResourceUsage(CommandList, DrawableTexture);
			}
		}
	}
}

void FImGuiRenderer::PrepareTextureForShaderResourceUsage(FRHICommandList& CommandList, const FImGuiTexture* InTexture)
{
    if (!InTexture)
    {
        return;
    }

    FRHITexture* Texture = InTexture->GetTexture();
    if (!Texture)
    {
        return;
    }

    if (RenderedTextures.Contains(Texture))
    {
        return;
    }

    CommandList.RequireTextureState(Texture, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    RenderedTextures.Emplace(Texture);
}

void FImGuiRenderer::OnCreateWindow(ImGuiViewport* Viewport)
{
    FImGuiViewport* ViewportData = reinterpret_cast<FImGuiViewport*>(Viewport->PlatformUserData);
    CHECK(ViewportData != nullptr);

    TSharedRef<FGenericWindow> PlatformWindow = ViewportData->Window->GetPlatformWindow();
    CHECK(PlatformWindow != nullptr);

    FRHISwapChainDesc SwapChainDesc;
    SwapChainDesc.WindowHandle = PlatformWindow->GetPlatformHandle();
    SwapChainDesc.ColorFormat  = EFormat::Unknown;
    SwapChainDesc.Width        = static_cast<uint16>(Viewport->Size.x);
    SwapChainDesc.Height       = static_cast<uint16>(Viewport->Size.y);
        
    ViewportData->SwapChain = FRHI::Get()->CreateSwapChain(SwapChainDesc);
    if (ViewportData->SwapChain)
    {
        ViewportData->Width  = SwapChainDesc.Width;
        ViewportData->Height = SwapChainDesc.Height;
        
        Viewport->RendererUserData = Viewport->PlatformUserData;
    }
}

void FImGuiRenderer::OnDestroyWindow(ImGuiViewport* Viewport)
{
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
    if (uint16(ViewportSize.x) != ViewportData->Width || uint16(ViewportSize.y) != ViewportData->Height)
    {
        ViewportData->Width  = uint16(ViewportSize.x);
        ViewportData->Height = uint16(ViewportSize.y);

        FRHISwapChain* RHISwapChain = ViewportData->SwapChain.Get();
        RHICommandList->ResizeSwapChain(RHISwapChain, ViewportData->Width, ViewportData->Height);
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

    RHICommandList->PresentSwapChain(ViewportData->SwapChain.Get(), false);
}
