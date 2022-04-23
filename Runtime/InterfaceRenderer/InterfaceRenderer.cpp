#include "InterfaceRenderer.h"

#include "Core/Time/Timer.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Containers/Array.h"

#include "Engine/Engine.h"
#include "Engine/Resources/TextureFactory.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShaderCompiler.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRenderer

CInterfaceRenderer* CInterfaceRenderer::Make()
{
    return dbg_new CInterfaceRenderer();
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

    FontTexture = CTextureFactory::LoadFromMemory(Pixels, Width, Height, ETextureFactoryFlags::None, ERHIFormat::R8G8B8A8_Unorm);
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

    CRHIVertexShaderRef VShader = RHICreateVertexShader(ShaderCode);
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

    CRHIVertexInputLayoutInitializer InputLayoutInitializer =
    {
        { "POSITION", 0, ERHIFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, pos)), EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, ERHIFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, uv)),  EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, ERHIFormat::R8G8B8A8_Unorm, 0, static_cast<uint32>(IM_OFFSETOF(ImDrawVert, col)), EVertexInputClass::Vertex, 0 },
    };

    CRHIVertexInputLayoutRef InputLayout = RHICreateInputLayout(InputLayoutInitializer);
    if (!InputLayout)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIDepthStencilStateInitializer DepthStencilStateInitializer(EComparisonFunc::Less, false, EDepthWriteMask::Zero);

    CRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
    if (!DepthStencilState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIRasterizerStateInitializer RasterizerStateInitializer(EFillMode::Solid, ECullMode::None);

    CRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
    if (!RasterizerState)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIBlendStateInitializer BlendStateInitializer( { SRenderTargetBlendInfo( true
                                                                             , EBlendType::Src1Alpha
                                                                             , EBlendType::InvSrcAlpha
                                                                             , EBlendOp::Add
                                                                             , EBlendType::InvSrcAlpha) }
                                                   , false
                                                   , false);

    CRHIBlendStateRef BlendStateBlending = RHICreateBlendState(BlendStateInitializer);
    if (!BlendStateBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    BlendStateInitializer.RenderTargets[0].bBlendEnable = false;

    CRHIBlendStateRef BlendStateNoBlending = RHICreateBlendState(BlendStateInitializer);
    if (!BlendStateBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIGraphicsPipelineStateInitializer PSOInitializer( InputLayout
                                                       , DepthStencilState
                                                       , RasterizerState
                                                       , BlendStateBlending
                                                       , SGraphicsPipelineShaders(VShader.Get(), PShader.Get())
                                                       , SGraphicsPipelineFormats({ ERHIFormat::B8G8R8A8_Unorm }, 1));

    PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineState)
    {
        CDebug::DebugBreak();
        return false;
    }

    PSOInitializer.BlendState = BlendStateNoBlending.Get();

    PipelineStateNoBlending = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineStateNoBlending)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIVertexBufferInitializer VBInitializer = CRHIVertexBufferInitializer::CreateStructured<ImDrawVert>( EBufferUsageFlags::Default
                                                                                                         , 1024*1024
                                                                                                         , EResourceAccess::VertexAndConstantBuffer);
    VertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("ImGui VertexBuffer");
    }

    const EIndexFormat IndexFormat = (sizeof(ImDrawIdx) == 2) ? EIndexFormat::uint16 : EIndexFormat::uint32;

    CRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, IndexFormat, 1024*1024, EResourceAccess::Common);
    IndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName("ImGui IndexBuffer");
    }

    CRHISamplerStateInitializer SamplerInitializer = CRHISamplerStateInitializer::CreateSimple(ESamplerMode::Clamp, ESamplerFilter::MinMagMipPoint);

    PointSampler = CRHISamplerStateCache::Get().GetOrCreateSampler(SamplerInitializer);
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

    SSetShaderConstantsInfo ShaderConstants(&MVP, sizeof(MVP));
    CmdList.Set32BitShaderConstants(PShader.Get(), ShaderConstants);

    CmdList.SetVertexBuffers(&VertexBuffer, 1, 0);

    CmdList.SetIndexBuffer(IndexBuffer.Get());
    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetBlendFactor({ 0.0f, 0.0f, 0.0f, 0.0f });

    // TODO: Do not change to GenericRead, change to Vertex/Constant-Buffer
    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::GenericRead, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(IndexBuffer.Get() , EResourceAccess::GenericRead, EResourceAccess::CopyDest);

    uint32 VertexOffset = 0;
    uint32 IndexOffset  = 0;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[Index];

        const uint32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        CmdList.UpdateBuffer(VertexBuffer.Get(), ImCmdList->VtxBuffer.Data, VertexOffset, VertexSize);

        const uint32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        CmdList.UpdateBuffer(IndexBuffer.Get(), ImCmdList->IdxBuffer.Data, IndexOffset, IndexSize);

        VertexOffset += VertexSize;
        IndexOffset  += IndexSize;
    }

    CmdList.TransitionBuffer(VertexBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::GenericRead);
    CmdList.TransitionBuffer(IndexBuffer.Get() , EResourceAccess::CopyDest, EResourceAccess::GenericRead);

    CmdList.SetSamplerState(PShader.Get(), PointSampler.Get(), 0);

    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset  = 0;

    ImVec2 ClipOff = DrawData->DisplayPos;
    for (int32 Index = 0; Index < DrawData->CmdListsCount; ++Index)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[Index];
        for (int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; ++CmdIndex)
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
                CmdList.SetShaderResourceTexture(PShader.Get(), FontTexture.Get(), 0);
            }

            CmdList.SetScissorRect(Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y);

            CmdList.DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }

    for (SInterfaceImage* Image : RenderedImages)
    {
        Check(Image != nullptr);

        if (Image->AfterState != EResourceAccess::PixelShaderResource)
        {
            CmdList.TransitionTexture(Image->Image.Get(), EResourceAccess::PixelShaderResource, Image->AfterState);
        }
    }

    RenderedImages.Clear();
}
