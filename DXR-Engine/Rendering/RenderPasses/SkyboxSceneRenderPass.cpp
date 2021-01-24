#include "SkyboxSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"

#include "Rendering/TextureFactory.h"

Bool SkyboxSceneRenderPass::Init(SharedRenderPassResources& FrameResource)
{
    SkyboxMesh = MeshFactory::CreateSphere(1);

    ResourceData VertexData = ResourceData(SkyboxMesh.Vertices.Data());
    SkyboxVertexBuffer = RenderLayer::CreateVertexBuffer<Vertex>(
        &VertexData,
        SkyboxMesh.Vertices.Size(),
        BufferUsage_Dynamic);
    if (!SkyboxVertexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxVertexBuffer->SetName("Skybox VertexBuffer");
    }

    ResourceData IndexData = ResourceData(SkyboxMesh.Indices.Data());
    SkyboxIndexBuffer = RenderLayer::CreateIndexBuffer(
        &IndexData,
        SkyboxMesh.Indices.SizeInBytes(),
        EIndexFormat::IndexFormat_UInt32,
        BufferUsage_Dynamic);
    if (!SkyboxIndexBuffer)
    {
        return false;
    }
    else
    {
        SkyboxIndexBuffer->SetName("Skybox IndexBuffer");
    }

    // Create Texture Cube
    const std::string PanoramaSourceFilename = "../Assets/Textures/arches.hdr";
    SampledTexture2D Panorama = TextureFactory::LoadSampledTextureFromFile(
        PanoramaSourceFilename,
        0,
        EFormat::Format_R32G32B32A32_Float);
    if (!Panorama)
    {
        return false;
    }
    else
    {
        Panorama.SetName(PanoramaSourceFilename);
    }

    FrameResource.Skybox = TextureFactory::CreateTextureCubeFromPanorma(
        Panorama,
        1024,
        TextureFactoryFlag_GenerateMips,
        EFormat::Format_R16G16B16A16_Float);
    if (!FrameResource.Skybox)
    {
        return false;
    }
    else
    {
        FrameResource.Skybox->SetName("Skybox");
    }

    FrameResource.SkyboxSRV = RenderLayer::CreateShaderResourceView(
        FrameResource.Skybox.Get(),
        EFormat::Format_R16G16B16A16_Float,
        0,
        FrameResource.Skybox->GetMipLevels());
    if (!FrameResource.SkyboxSRV)
    {
        return false;
    }
    else
    {
        FrameResource.SkyboxSRV->SetName("Skybox SRV");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressV = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressW = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipLinear;
    CreateInfo.MinLOD   = 0.0f;
    CreateInfo.MaxLOD   = 0.0f;

    SkyboxSampler = RenderLayer::CreateSamplerState(CreateInfo);
    if (!SkyboxSampler)
    {
        return false;
    }

    return true;
}

void SkyboxSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResource)
{
}
