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

static TAutoConsoleVariable<bool> CVarClearBeforeSkyboxEnabled(
    "Renderer.Skybox.ClearBeforeSkybox",
    "Clear the final target before rendering the Skybox (Used for debugging)",
    false);

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
        FMeshCreateInfo SkyboxMesh = FMeshFactory::CreateSphere(0);
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
    FRHIBufferInfo VBInfo;
    VBInfo.Size   = SkyboxVertices.SizeInBytes();
    VBInfo.Stride = SkyboxVertices.Stride();
    VBInfo.Flags  = EBufferFlags::Default | EBufferFlags::VertexBuffer;

    SkyboxVertexBuffer = FRHI::Get()->CreateBuffer(VBInfo, EResourceAccess::VertexBuffer, SkyboxVertices.Data());
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetDebugName("Skybox VertexBuffer");
    }

    // IndexBuffers
    FRHIBufferInfo IBInfo;
    IBInfo.Stride = GetStrideFromIndexFormat(SkyboxIndexFormat);
    IBInfo.Size   = SkyboxIndexCount * IBInfo.Stride;
    IBInfo.Flags  = EBufferFlags::Default | EBufferFlags::IndexBuffer;

    SkyboxIndexBuffer = FRHI::Get()->CreateBuffer(IBInfo, EResourceAccess::IndexBuffer, (SkyboxIndexFormat == EIndexFormat::uint16) ?
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

    FRHISamplerStateInfo Initializer;
    Initializer.AddressU = ESamplerMode::Wrap;
    Initializer.AddressV = ESamplerMode::Wrap;
    Initializer.AddressW = ESamplerMode::Wrap;
    Initializer.Filter   = ESamplerFilter::MinMagMipLinear;
    Initializer.MinLOD   = 0.0f;
    Initializer.MaxLOD   = 0.0f;

    SkyboxSampler = FRHI::Get()->CreateSamplerState(Initializer);
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

    SkyboxVertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);
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

    SkyboxPixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!SkyboxPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }
    
    // Initialize standard input layout
    FRHIVertexLayoutInitializerList VertexElementList =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 }
    };

    FRHIVertexLayoutRef InputLayout = FRHI::Get()->CreateVertexLayout(VertexElementList);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInfo DepthStencilStateInitializer;
    DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateInitializer.bDepthEnable      = true;
    DepthStencilStateInitializer.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.VertexInputLayout                      = InputLayout.Get();
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = SkyboxVertexShader.Get();
    PSOInitializer.ShaderState.PixelShader                = SkyboxPixelShader.Get();
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

    PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
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
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Skybox");

    GPU_TRACE_SCOPE(CommandList, "Skybox");

    TRACE_SCOPE("Render Skybox");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    const FFloatColor ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    const EAttachmentLoadAction LoadAction = CVarClearBeforeSkyboxEnabled.GetValue() ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;
    
    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), LoadAction, EAttachmentStoreAction::Store, ClearColor);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetVertexBuffers(MakeArrayView(&SkyboxVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SkyboxIndexBuffer.Get(), SkyboxIndexFormat);
    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    struct FSimpleCameraBufferHLSL
    {
        FMatrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene->Camera->GetViewProjectionWitoutTranslateMatrix();
    SimpleCamera.Matrix = SimpleCamera.Matrix.GetTranspose();

    constexpr uint32 NumConstants = sizeof(FSimpleCameraBufferHLSL) / sizeof(uint32);
    CommandList.Set32BitShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, NumConstants);

    FRHIShaderResourceView* SkyboxSRV = nullptr;
    if (Scene->Skybox)
    {
        SkyboxSRV = Scene->Skybox->GetCubeMap()->GetShaderResourceView();
    }

    CommandList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CommandList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CommandList.DrawIndexedInstanced(SkyboxIndexCount, 1, 0, 0, 0);

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Skybox");
}
