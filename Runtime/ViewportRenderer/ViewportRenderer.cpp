#include "ViewportRenderer.h"

#include "Core/Time/Timer.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Containers/Array.h"

#include "Engine/Engine.h"
#include "Engine/Resources/TextureFactory.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShaderCompiler.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

FViewportRenderer* FViewportRenderer::Make()
{
    return dbg_new FViewportRenderer();;
}

bool FViewportRenderer::InitContext(InterfaceContext Context)
{
    INIT_CONTEXT(Context);

    // Build texture atlas
    uint8* Pixels = nullptr;
    int32  Width  = 0;
    int32  Height = 0;

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    FontTexture = FTextureFactory::LoadFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
    if (!FontTexture)
    {
        return false;
    }

    static const CHAR* VSSource =
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

    static const CHAR* PSSource =
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

    FRHIVertexInputLayoutInitializer InputLayoutInfo =
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

    FRHIDepthStencilStateInitializer DepthStencilStateInfo;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode               = ECullMode::None;
    RasterizerStateInfo.bAntialiasedLineEnable = true;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInfo;
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

    FRHIGraphicsPipelineStateInitializer PSOProperties;
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

    FRHIBufferDesc VBDesc(
        1024 * 1024,
        sizeof(ImDrawVert),
        EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
    
    VertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, nullptr);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("ImGui VertexBuffer");
    }

    FRHIBufferDesc IBDesc(
        1024 * 1024,
        sizeof(ImDrawIdx),
        EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
    
    IndexBuffer = RHICreateBuffer(IBDesc, EResourceAccess::IndexBuffer, nullptr);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName("ImGui IndexBuffer");
    }

    FRHISamplerStateInitializer SamplerInitializer;
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

void FViewportRenderer::BeginTick()
{
    // Begin new frame
    ImGui::NewFrame();
}

void FViewportRenderer::EndTick()
{
    // EndFrame
    ImGui::EndFrame();
}

void FViewportRenderer::Render(FRHICommandList& CmdList)
{
    // Render ImgGui draw data
    ImGui::Render();

    ImDrawData* DrawData = ImGui::GetDrawData();

    float L = DrawData->DisplayPos.x;
    float R = DrawData->DisplayPos.x + DrawData->DisplaySize.x;
    float T = DrawData->DisplayPos.y;
    float B = DrawData->DisplayPos.y + DrawData->DisplaySize.y;
    float MVP[4][4] =
    {
        { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
        { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
        { 0.0f,              0.0f,              0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };

    // Setup viewport
    CmdList.SetViewport(DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 1.0f, 0.0f, 0.0f);

    CmdList.Set32BitShaderConstants(PShader.Get(), &MVP, 16);

    CmdList.SetVertexBuffers(MakeArrayView(&VertexBuffer, 1), 0);
    CmdList.SetIndexBuffer(IndexBuffer.Get(), (sizeof(ImDrawIdx) == 2) ? EIndexFormat::uint16 : EIndexFormat::uint32);
    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetBlendFactor(FVector4{ 0.0f, 0.0f, 0.0f, 0.0f });

    // TODO: Do not change to GenericRead, change to Vertex/Constant-Buffer
    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(IndexBuffer.Get() , EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    uint32 VertexOffset = 0;
    uint32 IndexOffset  = 0;
    for (int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[i];

        const uint32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        CmdList.UpdateBuffer(VertexBuffer.Get(), VertexOffset, VertexSize, ImCmdList->VtxBuffer.Data);

        const uint32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        CmdList.UpdateBuffer(IndexBuffer.Get(), IndexOffset, IndexSize, ImCmdList->IdxBuffer.Data);

        VertexOffset += VertexSize;
        IndexOffset  += IndexSize;
    }

    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CmdList.TransitionBuffer(IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);

    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset  = 0;
    ImVec2 ClipOff = DrawData->DisplayPos;
    for (int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
        for (int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; CmdIndex++)
        {
            CmdList.SetGraphicsPipelineState(PipelineState.Get());

            const ImDrawCmd* Cmd = &DrawCmdList->CmdBuffer[CmdIndex];
            if (Cmd->TextureId)
            {
                FDrawableTexture* DrawableTexture = reinterpret_cast<FDrawableTexture*>(Cmd->TextureId);
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

            CmdList.SetScissorRect(Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y);

            CmdList.DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
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
