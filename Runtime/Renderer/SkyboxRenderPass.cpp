#include "SkyboxRenderPass.h"
#include "Core/Misc/Debug.h"
#include "Core/Misc/FrameProfiler.h"
#include "Renderer/Debug/GPUProfiler.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Assets/AssetManager.h"
#include "RendererCore/TextureFactory.h"

bool FSkyboxRenderPass::Initialize(FFrameResources& FrameResources)
{
    if (!TextureCompressor.Initialize())
    {
        return false;
    }

    FMeshData SkyboxMesh = FMeshFactory::CreateSphere(0);
    SkyboxIndexCount = SkyboxMesh.Indices.Size();

    TArray<FVector3> NewVertices;
    NewVertices.Reserve(SkyboxMesh.Vertices.Size());
    for (const FVertex& Vertex : SkyboxMesh.Vertices)
    {
        NewVertices.Emplace(Vertex.Position);
    }
    
    FRHIBufferDesc VBDesc(NewVertices.SizeInBytes(), NewVertices.Stride(), EBufferUsageFlags::Default | EBufferUsageFlags::VertexBuffer);
    SkyboxVertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, SkyboxMesh.Vertices.Data());
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;
    const void* InitialIndicies = nullptr;

    SkyboxIndexFormat = SkyboxIndexCount < TNumericLimits<uint16>::Max() ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (SkyboxIndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(SkyboxMesh.Indices.Size());
        for (uint32 Index : SkyboxMesh.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.Data();
    }
    else
    {
        InitialIndicies = SkyboxMesh.Indices.Data();
    }

    FRHIBufferDesc IBDesc(SkyboxIndexCount * GetStrideFromIndexFormat(SkyboxIndexFormat), GetStrideFromIndexFormat(SkyboxIndexFormat), EBufferUsageFlags::Default | EBufferUsageFlags::IndexBuffer);
    SkyboxIndexBuffer = RHICreateBuffer(IBDesc, EResourceAccess::IndexBuffer, InitialIndicies);
    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetName("Skybox IndexBuffer");
    }

    // Create Texture Cube
    {
        const FString PanoramaSourceFilename = ENGINE_LOCATION"/Assets/Textures/arches.hdr";
        FTextureResource2DRef Panorama = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(PanoramaSourceFilename, false));
        if (!Panorama)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            Panorama->SetName(PanoramaSourceFilename);
        }

        // Compress the Panorama
        FRHITextureRef PanoramaRHI = Panorama->GetRHITexture();

        FRHITextureRef Skybox = FTextureFactory::CreateTextureCubeFromPanorma(PanoramaRHI.Get(), 1024, TextureFactoryFlag_GenerateMips, EFormat::R16G16B16A16_Float);
        if (!Skybox)
        {
            return false;
        }
        else
        {
            Skybox->SetName("Skybox Uncompressed");
        }

        // Compress the CubeMap
        TextureCompressor.CompressCubeMapBC6(Skybox, FrameResources.Skybox);
        if (FrameResources.Skybox)
        {
            FrameResources.Skybox->SetName("Skybox Compressed");
        }
    }

    FRHISamplerStateDesc Initializer;
    Initializer.AddressU = ESamplerMode::Wrap;
    Initializer.AddressV = ESamplerMode::Wrap;
    Initializer.AddressW = ESamplerMode::Wrap;
    Initializer.Filter   = ESamplerFilter::MinMagMipLinear;
    Initializer.MinLOD   = 0.0f;
    Initializer.MaxLOD   = 0.0f;

    SkyboxSampler = RHICreateSamplerState(Initializer);
    if (!SkyboxSampler)
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    
    {
        FRHIShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    SkyboxVertexShader = RHICreateVertexShader(ShaderCode);
    if (!SkyboxVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHIShaderCompileInfo CompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Skybox.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    SkyboxPixelShader = RHICreatePixelShader(ShaderCode);
    if (!SkyboxPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }
    
    // Initialize standard input layout
    FRHIVertexInputLayoutInitializer InputLayoutInitializer =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
    };

    FRHIVertexInputLayoutRef InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
    if (!InputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
    DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateInitializer.bDepthEnable      = true;
    DepthStencilStateInitializer.bDepthWriteEnable = true;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
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
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

    PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetName("SkyboxPSO PipelineState");
    }

    return true;
}

void FSkyboxRenderPass::Render(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FScene& Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Skybox");

    GPU_TRACE_SCOPE(CommandList, "Skybox");

    TRACE_SCOPE("Render Skybox");

    const float RenderWidth  = float(FrameResources.FinalTarget->GetWidth());
    const float RenderHeight = float(FrameResources.FinalTarget->GetHeight());

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    FRHIViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FRHIScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetVertexBuffers(MakeArrayView(&SkyboxVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SkyboxIndexBuffer.Get(), SkyboxIndexFormat);
    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    struct FSimpleCameraBuffer
    {
        FMatrix4 Matrix;
    } SimpleCamera;

    SimpleCamera.Matrix = Scene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

    CommandList.Set32BitShaderConstants(SkyboxVertexShader.Get(), &SimpleCamera, 16);

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CommandList.SetShaderResourceView(SkyboxPixelShader.Get(), SkyboxSRV, 0);

    CommandList.SetSamplerState(SkyboxPixelShader.Get(), SkyboxSampler.Get(), 0);

    CommandList.DrawIndexedInstanced(SkyboxIndexCount, 1, 0, 0, 0);

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Skybox");
}

void FSkyboxRenderPass::Release()
{
    PipelineState.Reset();
    SkyboxVertexBuffer.Reset();
    SkyboxIndexBuffer.Reset();
    SkyboxSampler.Reset();
    SkyboxVertexShader.Reset();
    SkyboxPixelShader.Reset();
}
