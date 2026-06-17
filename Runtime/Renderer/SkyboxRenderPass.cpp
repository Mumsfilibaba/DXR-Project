#include "Core/Misc/Debug.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Assets/AssetManager.h"
#include "RendererCore/TextureFactory.h"
#include "Renderer/SkyboxRenderPass.h"
#include "Renderer/Scene/Scene.h"

static bool GClearBeforeSkyboxEnabled = false;
static FAutoConsoleVariableRef CVarClearBeforeSkyboxEnabled(
    "Renderer.Skybox.ClearBeforeSkybox",
    "Clear the final target before rendering the Skybox (Used for debugging)",
    GClearBeforeSkyboxEnabled);

FSkyboxRenderPass::FSkyboxRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , SkyboxIndexCount(0)
    , SkyboxIndexFormat(EIndexFormat::Unknown)
    , CachedReadOnlyDepthTarget(nullptr)
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
    TArray<Vector3> SkyboxVertices;
    TArray<uint16>  SkyboxIndicies16;
    TArray<uint32>  SkyboxIndicies32;

    // Create a sphere used for the Skybox
    {
        FMeshCreateInfo SkyboxMesh = MeshFactory::CreateSphere(0);
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
        for (const FVertex& Vertex : SkyboxMesh.Vertices)
        {
            SkyboxVertices.Emplace(Vertex.Position);
        }
    }

    // VertexBuffer
    FRHIBufferDesc VertexBufferDesc;
    VertexBufferDesc.Size   = SkyboxVertices.SizeInBytes();
    VertexBufferDesc.Stride = SkyboxVertices.Stride();
    VertexBufferDesc.Flags  = EBufferFlags::Default | EBufferFlags::VertexBuffer;

    SkyboxVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, EResourceAccess::VertexBuffer, SkyboxVertices.Data());
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetDebugName("Skybox VertexBuffer");
    }

    // IndexBuffers
    FRHIBufferDesc IndexBufferDesc;
    IndexBufferDesc.Stride = GetStrideFromIndexFormat(SkyboxIndexFormat);
    IndexBufferDesc.Size   = SkyboxIndexCount * IndexBufferDesc.Stride;
    IndexBufferDesc.Flags  = EBufferFlags::Default | EBufferFlags::IndexBuffer;

    SkyboxIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, EResourceAccess::IndexBuffer, (SkyboxIndexFormat == EIndexFormat::uint16) ?
        reinterpret_cast<void*>(SkyboxIndicies16.Data()) :
        reinterpret_cast<void*>(SkyboxIndicies32.Data()));

    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetDebugName("Skybox IndexBuffer");
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

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    SkyboxVertexShader = RHI::CreateVertexShader(ShaderCode);
    if (!SkyboxVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    SkyboxPixelShader = RHI::CreatePixelShader(ShaderCode);
    if (!SkyboxPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }
    
    // Initialize standard input layout
    TArray<FRHIInputElementDesc> InputElements =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(Vector3), 0, 0, 0, EVertexInputClass::Vertex, 0 }
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
        PipelineState->SetDebugName("SkyboxPSO PipelineState");
    }

    return true;
}

void FSkyboxRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    RHI_EVENT_SCOPE(CommandList, "Skybox");

    GPU_TRACE_SCOPE(CommandList, "Skybox");

    TRACE_SCOPE("Render Skybox");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    const FFloatColor ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    const EAttachmentLoadAction LoadAction = GClearBeforeSkyboxEnabled ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;
    
    FRHITexture*          DepthTarget      = FrameResources.GBuffer[EGBufferIndex::Depth].Get();
    FRHIRenderTargetView* RenderTargetView = FrameResources.SceneTarget->GetRenderTargetView();

    if (CachedReadOnlyDepthTarget != DepthTarget)
    {
        CachedReadOnlyDepthDSV.Reset();
        CachedReadOnlyDepthTarget = DepthTarget;

        if (DepthTarget)
        {
            const FRHIDepthStencilViewDesc DSVDesc = FRHIDepthStencilViewDesc::CreateTexture2D(DepthTarget->GetDesc().Format, 0, EDepthStencilViewFlags::ReadOnlyDepth);
            CachedReadOnlyDepthDSV = RHI::CreateDepthStencilView(DepthTarget, DSVDesc);
        }
    }

    FRHIDepthStencilView* DepthStencilView = CachedReadOnlyDepthDSV.Get();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, LoadAction, EAttachmentStoreAction::Store, ClearColor);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetVertexBuffers(MakeArrayView(&SkyboxVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SkyboxIndexBuffer.Get(), SkyboxIndexFormat);
    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    struct FSimpleCameraBufferHLSL
    {
        Matrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene->Camera->GetViewProjectionWitoutTranslateMatrix();
    SimpleCamera.Matrix = SimpleCamera.Matrix.GetTranspose();

    constexpr uint32 NumConstants = sizeof(FSimpleCameraBufferHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, NumConstants);

    FRHIShaderResourceView* SkyboxSRV = nullptr;
    if (Scene->Skybox)
    {
        SkyboxSRV = Scene->Skybox->GetCubeMap()->GetShaderResourceView();
    }

    CommandList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CommandList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CommandList.DrawIndexedInstanced(SkyboxIndexCount, 1, 0, 0, 0);

    CommandList.EndRenderPass();
}
