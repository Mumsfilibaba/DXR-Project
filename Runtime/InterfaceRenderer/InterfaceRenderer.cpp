#include "InterfaceRenderer.h"

#include "Core/Time/Timer.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Containers/Array.h"

#include "Engine/Engine.h"
#include "Engine/Resources/TextureFactory.h"

#include "RHI/RHICoreInstance.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShaderCompiler.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRenderer

CInterfaceRenderer* CInterfaceRenderer::Make()
{
    return dbg_new CInterfaceRenderer();;
}

bool CInterfaceRenderer::InitContext(InterfaceContext Context)
{
    INIT_CONTEXT(Context);

    // Build texture atlas
    uint8* Pixels = nullptr;
    int32  Width  = 0;
    int32  Height = 0;

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    FontTexture = CTextureFactory::LoadFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
    if (!FontTexture)
    {
        return false;
    }

    static const char* VSSource =
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
    if (!CRHIShaderCompiler::CompileShader(VSSource, "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHIVertexShader> VShader = RHICreateVertexShader(ShaderCode);
    if (!VShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    static const char* PSSource =
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

    if (!CRHIShaderCompiler::CompileShader(PSSource, "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        CDebug::DebugBreak();
        return false;
    }

    PShader = RHICreatePixelShader(ShaderCode);
    if (!PShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    SRHIVertexInputLayoutInitializer InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), EVertexInputClass::Vertex, 0 },
    };

    TSharedRef<CRHIVertexInputLayout> InputLayout = RHICreateInputLayout(InputLayoutInfo);
    if (!InputLayout)
    {
        CDebug::DebugBreak();
        return false;
    }

    SRHIDepthStencilStateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.bDepthEnable   = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }

    SRHIRasterizerStateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }

    SRHIBlendStateInfo BlendStateInfo;
    BlendStateInfo.bIndependentBlendEnable        = false;
    BlendStateInfo.RenderTarget[0].bBlendEnable   = true;
    BlendStateInfo.RenderTarget[0].SrcBlend       = EBlendType ::SrcAlpha;
    BlendStateInfo.RenderTarget[0].SrcBlendAlpha  = EBlendType ::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlend      = EBlendType ::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlendAlpha = EBlendType ::Zero;
    BlendStateInfo.RenderTarget[0].BlendOpAlpha   = EBlendOp::Add;
    BlendStateInfo.RenderTarget[0].BlendOp        = EBlendOp::Add;

    TSharedRef<CRHIBlendState> BlendStateBlending = RHICreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    BlendStateInfo.RenderTarget[0].bBlendEnable = false;

    TSharedRef<CRHIBlendState> BlendStateNoBlending = RHICreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    SRHIGraphicsPipelineStateInfo PSOProperties;
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.InputLayoutState                       = InputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendStateBlending.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    PipelineState = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PipelineState)
    {
        CDebug::DebugBreak();
        return false;
    }

    PSOProperties.BlendState = BlendStateNoBlending.Get();

    PipelineStateNoBlending = RHICreateGraphicsPipelineState(PSOProperties);
    if (!PipelineStateNoBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    VertexBuffer = RHICreateVertexBuffer<ImDrawVert>(1024 * 1024, EBufferUsageFlags::Default, EResourceAccess::VertexAndConstantBuffer, nullptr);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("ImGui VertexBuffer");
    }

    const EIndexFormat IndexFormat = (sizeof(ImDrawIdx) == 2) ? EIndexFormat::uint16 : EIndexFormat::uint32;
    IndexBuffer = RHICreateIndexBuffer(IndexFormat, 1024 * 1024, EBufferUsageFlags::Default, EResourceAccess::Common, nullptr);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName("ImGui IndexBuffer");
    }

    SRHISamplerStateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipPoint;

    PointSampler = RHICreateSamplerState(CreateInfo);
    if (!PointSampler)
    {
        return false;
    }

    return true;
}

void CInterfaceRenderer::BeginTick()
{
    // Begin new frame
    ImGui::NewFrame();
}

void CInterfaceRenderer::EndTick()
{
    // EndFrame
    ImGui::EndFrame();
}

void CInterfaceRenderer::Render(CRHICommandList& CmdList)
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

    CmdList.SetVertexBuffers(&VertexBuffer, 1, 0);
    CmdList.SetIndexBuffer(IndexBuffer.Get());
    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetBlendFactor({ 0.0f, 0.0f, 0.0f, 0.0f });

    // TODO: Do not change to GenericRead, change to vertex/constantbuffer
    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(IndexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    uint32 VertexOffset = 0;
    uint32 IndexOffset = 0;
    for (int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[i];

        const uint32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        CmdList.UpdateBuffer(VertexBuffer.Get(), VertexOffset, VertexSize, ImCmdList->VtxBuffer.Data);

        const uint32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        CmdList.UpdateBuffer(IndexBuffer.Get(), IndexOffset, IndexSize, ImCmdList->IdxBuffer.Data);

        VertexOffset += VertexSize;
        IndexOffset += IndexSize;
    }

    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CmdList.TransitionBuffer(IndexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);

    CmdList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);

    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset = 0;
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
                SInterfaceImage* Image = reinterpret_cast<SInterfaceImage*>(Cmd->TextureId);
                RenderedImages.Emplace(Image);

                if (Image->BeforeState != EResourceAccess::PixelShaderResource)
                {
                    CmdList.TransitionTexture(Image->Image.Get(), Image->BeforeState, EResourceAccess::PixelShaderResource);

                    // TODO: Another way to do this? May break somewhere?
                    Image->BeforeState = EResourceAccess::PixelShaderResource;
                }

                CmdList.SetShaderResourceView(PShader.Get(), Image->ImageView.Get(), 0);

                if (!Image->bAllowBlending)
                {
                    CmdList.SetGraphicsPipelineState(PipelineStateNoBlending.Get());
                }
            }
            else
            {
                CRHIShaderResourceView* View = FontTexture->GetDefaultShaderResourceView();
                CmdList.SetShaderResourceView(PShader.Get(), View, 0);
            }

            CmdList.SetScissorRect(Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y);

            CmdList.DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
        }

        GlobalIndexOffset += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }

    for (SInterfaceImage* Image : RenderedImages)
    {
        Assert(Image != nullptr);

        if (Image->AfterState != EResourceAccess::PixelShaderResource)
        {
            CmdList.TransitionTexture(Image->Image.Get(), EResourceAccess::PixelShaderResource, Image->AfterState);
        }
    }

    RenderedImages.Clear();
}
