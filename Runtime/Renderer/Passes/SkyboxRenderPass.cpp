#include "Core/Misc/Debug.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/MeshFactory.h"
#include "RendererCore/TextureFactory.h"
#include "Renderer/Passes/SkyboxRenderPass.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

bool GClearBeforeSkyboxEnabled = false;
static FAutoConsoleVariableRef CVarClearBeforeSkyboxEnabled(
    TEXT("Renderer.Skybox.ClearBeforeSkybox"),
    TEXT("Clear the final target before rendering the Skybox (Used for debugging)"),
    GClearBeforeSkyboxEnabled);

struct FSkyboxConstantsHLSL
{
    FMatrix4 Matrix;
    float    Intensity;
    float    Rotation;
    float    Padding0;
    float    Padding1;
};

class FSkyboxVS
{
    DECLARE_SHADER_TYPE(FSkyboxVS, EShaderStage::Vertex);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSkyboxVS, TEXT("Shaders/Skybox.hlsl"), TEXT("VSMain"), EShaderModel::SM_6_2);

class FSkyboxPS
{
    DECLARE_SHADER_TYPE(FSkyboxPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSkyboxPS, TEXT("Shaders/Skybox.hlsl"), TEXT("PSMain"), EShaderModel::SM_6_2);

FSkyboxRenderPass::FSkyboxRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , SkyboxIndexCount(0)
    , SkyboxIndexFormat(EIndexFormat::Unknown)
{
}

FSkyboxRenderPass::~FSkyboxRenderPass()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
    SkyboxVertexShader.Reset();
    SkyboxPixelShader.Reset();
}

bool FSkyboxRenderPass::Initialize(FFrameResources& /* FrameResources */)
{
    // Sphere-data
    TArray<FVector3> SkyboxVertices;
    TArray<uint16>   SkyboxIndicies16;
    TArray<uint32>   SkyboxIndicies32;

    // Create a sphere used for the Skybox
    {
        FMeshData SkyboxMesh = MeshFactory::CreateSphere(0);
        SkyboxIndexCount = SkyboxMesh.Indices.Size();

        // Indices
        SkyboxIndexFormat = SkyboxIndexCount < TNumericLimits<uint16>::Max() ? EIndexFormat::uint16 : EIndexFormat::uint32;
        if (SkyboxIndexFormat == EIndexFormat::uint16)
        {
            SkyboxIndicies16 = SkyboxMesh.GetSmallIndices();
        }
        else
        {
            SkyboxIndicies32 = Move(SkyboxMesh.Indices);
        }

        // Vertices
        SkyboxVertices.Reserve(SkyboxMesh.Vertices.Size());
        for (const FSourceVertex& Vertex : SkyboxMesh.Vertices)
        {
            SkyboxVertices.Emplace(Vertex.Position);
        }
    }

    // VertexBuffer
    const FRHIBufferDesc VertexBufferDesc = FRHIBufferDesc::CreateVertexBuffer(SkyboxVertices.Stride(), SkyboxVertices.Size());
    SkyboxVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, ERHIResourceState::VertexBuffer, SkyboxVertices.Data());
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetDebugName(TEXT("Skybox VertexBuffer"));
    }

    // IndexBuffers
    const FRHIBufferDesc IndexBufferDesc = FRHIBufferDesc::CreateIndexBuffer(SkyboxIndexFormat, SkyboxIndexCount);
    SkyboxIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::IndexBuffer, (SkyboxIndexFormat == EIndexFormat::uint16) ? reinterpret_cast<void*>(SkyboxIndicies16.Data()) :
        reinterpret_cast<void*>(SkyboxIndicies32.Data()));

    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetDebugName(TEXT("Skybox IndexBuffer"));
    }

    FRHISamplerStateDesc SamplerStateDesc;
    SamplerStateDesc.AddressU = ESamplerMode::Wrap;
    SamplerStateDesc.AddressV = ESamplerMode::Wrap;
    SamplerStateDesc.AddressW = ESamplerMode::Wrap;
    SamplerStateDesc.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerStateDesc.MinLOD   = 0.0f;
    SamplerStateDesc.MaxLOD   = 0.0f;

    SkyboxSampler = RHI::CreateSamplerState(SamplerStateDesc);
    if (!SkyboxSampler)
    {
        return false;
    }

    SkyboxVertexShader = FShaderCache::Get().GetShader<FSkyboxVS>();
    if (!SkyboxVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    SkyboxPixelShader = FShaderCache::Get().GetShader<FSkyboxPS>();
    if (!SkyboxPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    // Initialize standard input layout
    TArray<FRHIInputElementDesc> InputElements =
    {
        { TEXT("POSITION"), 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 }
    };

    FRHIInputLayoutRef InputLayout = RHI::CreateInputLayout(InputElements);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = InputLayout.Get();
    PSODesc.BlendState                                     = BlendState.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.VertexShader                                   = SkyboxVertexShader.Get();
    PSODesc.PixelShader                                    = SkyboxPixelShader.Get();
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RendererTextureFormats::SceneTargetFormat;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

    PipelineState = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetDebugName(TEXT("SkyboxPSO PipelineState"));
    }

    return true;
}

void FSkyboxRenderPass::Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, TEXT("Skybox"));

    TRACE_SCOPE(TEXT("Render Skybox"));

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetVertexBuffers(MakeArrayView(&SkyboxVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SkyboxIndexBuffer.Get(), SkyboxIndexFormat);
    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    FSkyboxConstantsHLSL SkyboxConstants;
    SkyboxConstants.Matrix    = Scene->GetCamera()->Snapshot.ViewProjectionNoTranslation.GetTranspose();
    SkyboxConstants.Intensity = 1.0f;
    SkyboxConstants.Rotation  = 0.0f;
    SkyboxConstants.Padding0  = 0.0f;
    SkyboxConstants.Padding1  = 0.0f;

    FRHIShaderResourceView* SkyboxSRV = nullptr;
    if (FSceneSkybox* Skybox = Scene->GetSkybox())
    {
        SkyboxSRV                 = Skybox->CubeMap->GetShaderResourceView();
        SkyboxConstants.Intensity = Skybox->Intensity;
        SkyboxConstants.Rotation  = Skybox->Rotation;
    }

    constexpr uint32 NumConstants = sizeof(FSkyboxConstantsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(SkyboxVertexShader.Get(), &SkyboxConstants, NumConstants);

    CommandList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CommandList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CommandList.DrawIndexedInstanced(SkyboxIndexCount, 1, 0, 0, 0);
}

void FSkyboxRenderPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass(TEXT("Skybox"), ERenderGraphPassFlags::Raster, GSkyboxEnabled,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            const EAttachmentLoadAction LoadAction = GClearBeforeSkyboxEnabled ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;
            PassBuilder.SetRenderTarget(0, Context.SceneTargetRenderTargetView, LoadAction);

            if (Context.GBufferDepthReadOnlyDSV)
            {
                PassBuilder.SetDepthStencil(Context.GBufferDepthReadOnlyDSV, EAttachmentLoadAction::Load);
            }

            if (Context.SkyboxVertexBuffer)
            {
                PassBuilder.ReadBuffer(Context.SkyboxVertexBuffer, ERHIResourceState::VertexBuffer);
            }

            if (Context.SkyboxIndexBuffer)
            {
                PassBuilder.ReadBuffer(Context.SkyboxIndexBuffer, ERHIResourceState::IndexBuffer);
            }
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources, Context.Scene);
        });
}
