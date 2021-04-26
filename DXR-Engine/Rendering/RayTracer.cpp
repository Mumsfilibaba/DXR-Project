#include "RayTracer.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Resources/Material.h"
#include "Resources/Mesh.h"

#include "Math/Random.h"

Bool RayTracer::Init(FrameResources& Resources)
{
    if (!CreateRenderTargets(Resources))
    {
        return false;
    }

    if (!InitShadingImage(Resources))
    {
        return false;
    }

    TArray<ShaderDefine> Defines =
    {
        { "ENABLE_HALF_RES", std::to_string(ENABLE_HALF_RES) },
        { "ENABLE_VRS", std::to_string(VRS_IMAGE_ROUGHNESS) }
    };

    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RayGen.hlsl", "RayGen", &Defines, EShaderStage::RayGen, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    RayGenShader = CreateRayGenShader(Code);
    if (!RayGenShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RayGenShader->SetName("RayGenShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ClosestHit.hlsl", "ClosestHit", &Defines, EShaderStage::RayClosestHit, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    RayClosestHitShader = CreateRayClosestHitShader(Code);
    if (!RayClosestHitShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RayClosestHitShader->SetName("RayClosestHitShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Miss.hlsl", "Miss", &Defines, EShaderStage::RayMiss, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    RayMissShader = CreateRayMissShader(Code);
    if (!RayMissShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RayMissShader->SetName("RayMissShader");
    }

    {
        RayTracingPipelineStateCreateInfo CreateInfo;
        CreateInfo.RayGen                  = RayGenShader.Get();
        CreateInfo.ClosestHitShaders       = { RayClosestHitShader.Get() };
        CreateInfo.MissShaders             = { RayMissShader.Get() };
        CreateInfo.HitGroups               = { RayTracingHitGroup("HitGroup", nullptr, RayClosestHitShader.Get()) };
        CreateInfo.MaxRecursionDepth       = 1;
        CreateInfo.MaxAttributeSizeInBytes = sizeof(RayIntersectionAttributes);
        CreateInfo.MaxPayloadSizeInBytes   = sizeof(RayPayload);

        Pipeline = CreateRayTracingPipelineState(CreateInfo);
        if (!Pipeline)
        {
            Debug::DebugBreak();
            return false;
        }
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/FullscreenVS.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_5, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    FullscreenShader = CreateVertexShader(Code);
    if (!FullscreenShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FullscreenShader->SetName("FullscreenShader");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/InlineRayGen.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_5, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    InlineRayGen = CreatePixelShader(Code);
    if (!InlineRayGen)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        InlineRayGen->SetName("Inline RayGen");
    }

    GraphicsPipelineStateCreateInfo InlinePSODesc;
    InlinePSODesc.InputLayoutState       = nullptr;
    InlinePSODesc.BlendState             = nullptr;
    InlinePSODesc.DepthStencilState      = nullptr;
    InlinePSODesc.RasterizerState        = nullptr;
    InlinePSODesc.VertexShader           = FullscreenShader.Get();
    InlinePSODesc.PixelShader            = InlineRayGen.Get();
    InlinePSODesc.PrimitiveTopologyType  = EPrimitiveTopologyType::Triangle;
    InlinePSODesc.NumRenderTargets       = 2;
    InlinePSODesc.RenderTargetFormats[0] = EFormat::R16G16B16A16_Float;
    InlinePSODesc.RenderTargetFormats[1] = EFormat::R16G16B16A16_Float;
    InlinePSODesc.DepthStencilFormat     = EFormat::Unknown;

    InlineRTPipeline = CreateGraphicsPipelineState(InlinePSODesc);
    if (!InlineRTPipeline)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        InlineRTPipeline->SetName("InlineRT PipelineState");
    }

    RandomDataBuffer = CreateConstantBuffer<RandomData>(BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!RandomDataBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RandomDataBuffer->SetName("RandomDataBuffer");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RTSpatioTemporalFilter.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    RTSpatialShader = CreateComputeShader(Code);
    if (!RTSpatialShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTSpatialShader->SetName("RT Spatio-Temporal Shader");
    }

    ComputePipelineStateCreateInfo RTSpatialCreateInfo;
    RTSpatialCreateInfo.Shader = RTSpatialShader.Get();

    RTSpatialPSO = CreateComputePipelineState(RTSpatialCreateInfo);
    if (!RTSpatialPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTSpatialPSO->SetName("RT Spatial Pipeline");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadingImage.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    ShadingRateGenShader = CreateComputeShader(Code);
    if (!ShadingRateGenShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRateGenShader->SetName("ShadingRate Image Shader");
    }

    {
        ComputePipelineStateCreateInfo CreateInfo;
        CreateInfo.Shader = ShadingRateGenShader.Get();

        ShadingRateGenPSO = CreateComputePipelineState(CreateInfo);
        if (!ShadingRateGenPSO)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            ShadingRateGenPSO->SetName("ShadingRate Image Pipeline");
        }
    }

    Defines.Clear();
    Defines.EmplaceBack("HORIZONTAL_PASS", "1");

    // Load shader
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RTBilateralPass.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    BlurHorizontalShader = CreateComputeShader(Code);
    if (!BlurHorizontalShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurHorizontalShader->SetName("RT Horizontal Blur Shader");
    }

    ComputePipelineStateCreateInfo PSOProperties;
    PSOProperties.Shader = BlurHorizontalShader.Get();

    BlurHorizontalPSO = CreateComputePipelineState(PSOProperties);
    if (!BlurHorizontalPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurHorizontalPSO->SetName("RT Horizontal Blur PSO");
    }

    Defines.Clear();
    Defines.EmplaceBack("VERTICAL_PASS", "1");

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RTBilateralPass.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    BlurVerticalShader = CreateComputeShader(Code);
    if (!BlurVerticalShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurVerticalShader->SetName("RT Vertical Blur Shader");
    }

    PSOProperties.Shader = BlurVerticalShader.Get();

    BlurVerticalPSO = CreateComputePipelineState(PSOProperties);
    if (!BlurVerticalPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurVerticalPSO->SetName("RT Vertical Blur PSO");
    }

    RTMaterialBuffer = CreateStructuredBuffer(sizeof(RayTracingMaterial), MAX_RT_MATERIALS, BufferFlag_SRV, EResourceState::PixelShaderResource, nullptr);
    if (!RTMaterialBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTMaterialBuffer->SetName("RT MaterialBuffer");
    }

    RTMaterialBufferSRV = CreateShaderResourceView(RTMaterialBuffer.Get(), 0, MAX_RT_MATERIALS);
    if (!RTMaterialBufferSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTMaterialBufferSRV->SetName("RT MaterialBuffer SRV");
    }

    return true;
}

Bool RayTracer::InitShadingImage(FrameResources& Resources)
{
    UNREFERENCED_VARIABLE(Resources);

    ShadingRateSupport Support;
    CheckShadingRateSupport(Support);

    if (Support.Tier != EShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0)
    {
        return true;
    }

    UInt32 Width  = RTColorDepth->GetWidth() / Support.ShadingRateImageTileSize;
    UInt32 Height = RTColorDepth->GetHeight() / Support.ShadingRateImageTileSize;
    ShadingRateImage = CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::ShadingRateSource, nullptr);
    if (!ShadingRateImage)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRateImage->SetName("Shading Rate Image");
    }

    return true;
}

void RayTracer::Release()
{
    Pipeline.Reset();
    
    RTMaterialBuffer.Reset();
    RTMaterialBufferSRV.Reset();

    InlineRTPipeline.Reset();
    FullscreenShader.Reset();
    InlineRayGen.Reset();

    RandomDataBuffer.Reset();
    
    RTSpatialPSO.Reset();
    RTSpatialShader.Reset();
    
    RTColorDepth.Reset();

    RTHistory0.Reset();
    RTHistory1.Reset();
    
    RTReconstructed.Reset();
    
    ShadingRateImage.Reset();
    ShadingRateGenPSO.Reset();
    ShadingRateGenShader.Reset();

    BlurHorizontalPSO.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalPSO.Reset();
    BlurVerticalShader.Reset();
}

void RayTracer::Render(CommandList& CmdList, FrameResources& Resources, LightSetup& LightSetup, const Scene& Scene)
{
    static UInt32 FrameIndex = 0;
    FrameIndex++;
    FrameIndex = FrameIndex & 63;

    RandomData RndData;
    RndData.FrameIndex = FrameIndex;
    RndData.Seed       = Random::Integer();

    CmdList.TransitionBuffer(RandomDataBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.UpdateBuffer(RandomDataBuffer.Get(), 0, sizeof(RandomData), &RndData);
    CmdList.TransitionBuffer(RandomDataBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    UInt32 InstanceIndex  = 0;
    SamplerState* Sampler = nullptr;

    for (const MeshDrawCommand& Cmd : Scene.GetMeshDrawCommands())
    {
        Material* Mat = Cmd.Material;
        if (Cmd.Material->HasTransparency())
        {
            continue;
        }

        RayTracingMaterial MaterialData;
        MaterialData.AlbedoTexID   = Resources.RTMaterialTextureCache.Add(Mat->DiffuseMap->GetShaderResourceView());
        MaterialData.NormalTexID   = Resources.RTMaterialTextureCache.Add(Mat->NormalMap->GetShaderResourceView());
        MaterialData.SpecularTexID = Resources.RTMaterialTextureCache.Add(Mat->SpecularMap->GetShaderResourceView());
        
        if (Mat->EmissiveMap)
        {
            MaterialData.EmissiveTexID = Resources.RTMaterialTextureCache.Add(Mat->EmissiveMap->GetShaderResourceView());
        }

        MaterialData.Albedo    = Mat->GetMaterialProperties().Diffuse;
        MaterialData.Roughness = Mat->GetMaterialProperties().Roughness;
        MaterialData.AO        = Mat->GetMaterialProperties().AO;
        MaterialData.Metallic  = Mat->GetMaterialProperties().Metallic;
        Sampler = Mat->GetMaterialSampler();

        UInt32 MaterialIndex = 0;
        auto MaterialIndexPair = Resources.RTMaterialMap.find(Mat);
        if (MaterialIndexPair == Resources.RTMaterialMap.end())
        {
            MaterialIndex = Resources.RTMaterials.Size();
            Resources.RTMaterials.EmplaceBack(MaterialData);
            Resources.RTMaterialMap[Mat] = MaterialIndex;
        }
        else
        {
            MaterialIndex = MaterialIndexPair->second;
            Resources.RTMaterials[MaterialIndex] = MaterialData;
        }

        const XMFLOAT3X4 TinyTransform = Cmd.CurrentActor->GetTransform().GetTinyMatrix();
        UInt32 HitGroupIndex = 0;

        auto HitGroupIndexPair = Resources.RTMeshToHitGroupIndex.find(Cmd.Mesh);
        if (HitGroupIndexPair == Resources.RTMeshToHitGroupIndex.end())
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
            Resources.RTMeshToHitGroupIndex[Cmd.Mesh] = HitGroupIndex;

            RayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";
            if (Cmd.Mesh->VertexBufferSRV)
            {
                ShaderResourceView* SRV = Cmd.Mesh->VertexBufferSRV.Get();
                Resources.RTVertexBuffers.EmplaceBack(SRV);

                HitGroupResources.AddShaderResourceView(SRV);
            }
            if (Cmd.Mesh->IndexBufferSRV)
            {
                ShaderResourceView* SRV = Cmd.Mesh->IndexBufferSRV.Get();
                Resources.RTIndexBuffers.EmplaceBack(SRV);

                HitGroupResources.AddShaderResourceView(SRV);
            }

            Resources.RTHitGroupResources.EmplaceBack(HitGroupResources);
        }
        else
        {
            HitGroupIndex = HitGroupIndexPair->second;
        }

        RayTracingGeometryInstance Instance;
        Instance.Instance      = MakeSharedRef<RayTracingGeometry>(Cmd.Geometry);
        Instance.Flags         = RayTracingInstanceFlags_None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = MaterialIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.EmplaceBack(Instance);
    }

    Assert(Resources.RTMaterials.Size() <= MAX_RT_MATERIALS);

    CmdList.TransitionBuffer(RTMaterialBuffer.Get(), EResourceState::PixelShaderResource, EResourceState::CopyDest);
    CmdList.UpdateBuffer(RTMaterialBuffer.Get(), 0, Resources.RTMaterials.SizeInBytes(), Resources.RTMaterials.Data());
    CmdList.TransitionBuffer(RTMaterialBuffer.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);

    Resources.RTVertexBuffers.ShrinkToFit();
    Resources.RTIndexBuffers.ShrinkToFit();
    Resources.RTGeometryInstances.ShrinkToFit();

    if (!Resources.RTScene)
    {
        Resources.RTScene = CreateRayTracingScene(RayTracingStructureBuildFlag_None, Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size());
        if (Resources.RTScene)
        {
            Resources.RTScene->SetName("RayTracingScene");
        }
    }
    else
    {
        GPU_TRACE_SCOPE(CmdList, "RT Build Scene");
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size(), false);
    }

    constexpr float episolon = 0x1.fffffep-1;

    UInt32 Width  = Resources.RTReflections->GetWidth();
    UInt32 Height = Resources.RTReflections->GetHeight();
#if ENABLE_HALF_RES
    const UInt32 TraceWidth  = Width / 2;
    const UInt32 TraceHeight = Height / 2;
#else
    const UInt32 TraceWidth  = Width;
    const UInt32 TraceHeight = Height;
#endif

#if ENABLE_INLINE_RAY_GEN
    CmdList.TransitionTexture(RTColorDepth.Get(), EResourceState::UnorderedAccess, EResourceState::RenderTarget);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::UnorderedAccess, EResourceState::RenderTarget);

#if ENABLE_VRS
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin VRS Image");

        GPU_TRACE_SCOPE(CmdList, "Generate VRS Image");

        CmdList.SetShadingRate(EShadingRate::VRS_1x1);
        CmdList.TransitionTexture(ShadingRateImage.Get(), EResourceState::ShadingRateSource, EResourceState::UnorderedAccess);

        CmdList.SetComputePipelineState(ShadingRateGenPSO.Get());
        
#if VRS_IMAGE_ROUGHNESS
        CmdList.SetShaderResourceView(ShadingRateGenShader.Get(), Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 0);
#else
        CmdList.SetShaderResourceView(ShadingRateGenShader.Get(), Resources.GBuffer[GBUFFER_GEOM_NORMAL_INDEX]->GetShaderResourceView(), 0);
        CmdList.SetShaderResourceView(ShadingRateGenShader.Get(), Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 1);

        CmdList.SetConstantBuffer(ShadingRateGenShader.Get(), Resources.CameraBuffer.Get(), 0);
#endif
        CmdList.SetUnorderedAccessView(ShadingRateGenShader.Get(), ShadingRateImage->GetUnorderedAccessView(), 0);

        XMUINT3 Threads = ShadingRateGenShader->GetThreadGroupXYZ();
        UInt32 ThreadsX = Math::DivideByMultiple(ShadingRateImage->GetWidth(), Threads.x);
        UInt32 ThreadsY = Math::DivideByMultiple(ShadingRateImage->GetHeight(), Threads.y);
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(ShadingRateImage.Get(), EResourceState::UnorderedAccess, EResourceState::ShadingRateSource);

        CmdList.SetShadingRateImage(ShadingRateImage.Get());

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End VRS Image");
    }
#endif

    CmdList.SetConstantBuffer(InlineRayGen.Get(), Resources.CameraBuffer.Get(), 0);
    CmdList.SetConstantBuffer(InlineRayGen.Get(), RandomDataBuffer.Get(), 1);
    CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.LightInfoBuffer.Get(), 2);
    CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.PointLightsBuffer.Get(), 3);
    CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 4);
    //CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 5);
    //CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 6);
    CmdList.SetConstantBuffer(InlineRayGen.Get(), LightSetup.DirectionalLightsBuffer.Get(), 5);

    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.RTScene->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.Skybox->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.GBuffer[GBUFFER_GEOM_NORMAL_INDEX]->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 4);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), LightSetup.DirLightShadowMaps->GetShaderResourceView(), 5);

#if ENABLE_HALF_RES
    CmdList.SetShaderResourceView(InlineRayGen.Get(), Resources.BlueNoise->GetShaderResourceView(), 6);
    CmdList.SetShaderResourceView(InlineRayGen.Get(), RTMaterialBufferSRV.Get(), 7);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTMaterialTextureCache.Data(), Resources.RTMaterialTextureCache.Size(), 8);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTVertexBuffers.Data(), Resources.RTVertexBuffers.Size(), 9);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTIndexBuffers.Data(), Resources.RTIndexBuffers.Size(), 10);
#else
    CmdList.SetShaderResourceView(InlineRayGen.Get(), RTMaterialBufferSRV.Get(), 6);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTMaterialTextureCache.Data(), Resources.RTMaterialTextureCache.Size(), 7);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTVertexBuffers.Data(), Resources.RTVertexBuffers.Size(), 8);
    CmdList.SetShaderResourceViews(InlineRayGen.Get(), Resources.RTIndexBuffers.Data(), Resources.RTIndexBuffers.Size(), 9);
#endif

    CmdList.SetSamplerState(InlineRayGen.Get(), Resources.IrradianceSampler.Get(), 0);
    CmdList.SetSamplerState(InlineRayGen.Get(), Sampler, 1);
    CmdList.SetSamplerState(InlineRayGen.Get(), Resources.DirectionalShadowSampler.Get(), 2);

    CmdList.SetGraphicsPipelineState(InlineRTPipeline.Get());

    CmdList.SetViewport((Float)TraceWidth, (Float)TraceHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect((Float)TraceWidth, (Float)TraceHeight, 0.0f, 0.0f);

    RenderTargetView* RTVs[2] =
    {
        RTColorDepth->GetRenderTargetView(),
        Resources.RTRayPDF->GetRenderTargetView()
    };
    CmdList.SetRenderTargets(RTVs, 2, nullptr);

    VertexBuffer* VBuffer = nullptr;
    CmdList.SetVertexBuffers(&VBuffer, 1, 0);
    CmdList.SetIndexBuffer(nullptr);

    {
        GPU_TRACE_SCOPE(CmdList, "Inline RayGen");
        CmdList.Draw(3, 0);
    }

    CmdList.SetShadingRate(EShadingRate::VRS_1x1);
#if ENABLE_VRS
    CmdList.SetShadingRateImage(nullptr);
#endif

    CmdList.TransitionTexture(RTColorDepth.Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource);

    CmdList.SetViewport((Float)Width, (Float)Height, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect((Float)Width, (Float)Height, 0.0f, 0.0f);
#else
    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(RTColorDepth->GetUnorderedAccessView());
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTRayPDF->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.LightInfoBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(RandomDataBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.PointLightsBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.PointLightsPosRadBuffer.Get());
    //Resources.GlobalResources.AddConstantBuffer(LightSetup.ShadowCastingPointLightsBuffer.Get());
    //Resources.GlobalResources.AddConstantBuffer(LightSetup.ShadowCastingPointLightsPosRadBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.DirectionalLightsBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.SkyboxSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddSamplerState(Resources.DirectionalShadowSampler.Get());
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_GEOM_NORMAL_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView());
#if ENABLE_HALF_RES
    Resources.GlobalResources.AddShaderResourceView(Resources.BlueNoise->GetShaderResourceView());
#endif
    Resources.GlobalResources.AddShaderResourceView(RTMaterialBufferSRV.Get());
    Resources.GlobalResources.AddShaderResourceView(LightSetup.DirLightShadowMaps->GetShaderResourceView());

    for (UInt32 i = 0; i < Resources.RTMaterialTextureCache.Size(); i++)
    {
        Resources.GlobalResources.AddShaderResourceView(Resources.RTMaterialTextureCache.Get(i));
    }

    Resources.RayGenLocalResources.Reset();
    Resources.RayGenLocalResources.Identifier = "RayGen";
    
    Resources.MissLocalResources.Reset();
    Resources.MissLocalResources.Identifier = "Miss";

    // TODO: NO MORE BINDINGS CAN BE BOUND BEFORE DISPATCH RAYS, FIX THIS
    CmdList.SetRayTracingBindings(
        Resources.RTScene.Get(), 
        Pipeline.Get(), 
        &Resources.GlobalResources, 
        &Resources.RayGenLocalResources, 
        &Resources.MissLocalResources,
        Resources.RTHitGroupResources.Data(), 
        Resources.RTHitGroupResources.Size());

    {
        GPU_TRACE_SCOPE(CmdList, "Dispatch Rays");
        CmdList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), TraceWidth, TraceHeight, 1);
    }

    CmdList.UnorderedAccessTextureBarrier(RTColorDepth.Get());
    CmdList.UnorderedAccessTextureBarrier(Resources.RTRayPDF.Get());

    CmdList.TransitionTexture(RTColorDepth.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource);
#endif

    CmdList.SetComputePipelineState(RTSpatialPSO.Get());
    
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), RTColorDepth->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.RTRayPDF->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_GEOM_NORMAL_INDEX]->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 4);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_VELOCITY_INDEX]->GetShaderResourceView(), 5);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 6);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.PrevGeomNormals->GetShaderResourceView(), 7);

    CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RTReconstructed->GetUnorderedAccessView(), 0);

    if (FrameIndex & 1)
    {
        CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RTHistory0->GetUnorderedAccessView(), 1);
        CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RTHistory1->GetUnorderedAccessView(), 2);
    }
    else
    {
        CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RTHistory1->GetUnorderedAccessView(), 1);
        CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RTHistory0->GetUnorderedAccessView(), 2);
    }
    
    CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), Resources.RTReflections->GetUnorderedAccessView(), 3);
    
    CmdList.SetConstantBuffer(RTSpatialShader.Get(), Resources.CameraBuffer.Get(), 0);

    XMUINT3 ThreadGroup = RTSpatialShader->GetThreadGroupXYZ();
    Width  = Math::DivideByMultiple(RTReconstructed->GetWidth(), ThreadGroup.x);
    Height = Math::DivideByMultiple(RTReconstructed->GetHeight(), ThreadGroup.y);

    {
        GPU_TRACE_SCOPE(CmdList, "RT Reconstruction Filter");
        CmdList.Dispatch(Width, Height, ThreadGroup.z);
    }

    CmdList.TransitionTexture(RTColorDepth.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess);

    CmdList.UnorderedAccessTextureBarrier(RTReconstructed.Get());
    CmdList.UnorderedAccessTextureBarrier(RTHistory0.Get());
    CmdList.UnorderedAccessTextureBarrier(RTHistory1.Get());
    CmdList.UnorderedAccessTextureBarrier(Resources.RTReflections.Get());

    {
        GPU_TRACE_SCOPE(CmdList, "RT Bilateral Filter");
        CmdList.SetComputePipelineState(BlurHorizontalPSO.Get());

        XMFLOAT2 ScreenSize = XMFLOAT2(Resources.RTReflections->GetWidth(), Resources.RTReflections->GetHeight());
        CmdList.Set32BitShaderConstants(BlurHorizontalShader.Get(), &ScreenSize, 2);

        CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), Resources.RTReflections->GetUnorderedAccessView(), 0);
        CmdList.Dispatch(Width, Height, 1);

        CmdList.UnorderedAccessTextureBarrier(Resources.RTReflections.Get());

        CmdList.SetComputePipelineState(BlurVerticalPSO.Get());

        CmdList.Set32BitShaderConstants(BlurVerticalShader.Get(), &ScreenSize, 2);

        CmdList.Dispatch(Width, Height, 1);

        CmdList.UnorderedAccessTextureBarrier(Resources.RTReflections.Get());
    }

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RTColorDepth->GetShaderResourceView()),
        RTColorDepth,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.RTRayPDF->GetShaderResourceView()),
        Resources.RTRayPDF,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(ShadingRateImage->GetShaderResourceView()),
        ShadingRateImage,
        EResourceState::ShadingRateSource,
        EResourceState::ShadingRateSource);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RTReconstructed->GetShaderResourceView()),
        RTReconstructed,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RTHistory0->GetShaderResourceView()),
        RTHistory0,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RTHistory1->GetShaderResourceView()),
        RTHistory1,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.RTReflections->GetShaderResourceView()),
        Resources.RTReflections,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);
}

Bool RayTracer::OnResize(FrameResources& Resources)
{
    return CreateRenderTargets(Resources);
}

Bool RayTracer::CreateRenderTargets(FrameResources& Resources)
{
    UInt32 Width  = Resources.MainWindowViewport->GetWidth();
    UInt32 Height = Resources.MainWindowViewport->GetHeight();
    Resources.RTReflections = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!Resources.RTReflections)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.RTReflections->SetName("RT Reflections");
    }

    RTHistory0 = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!RTHistory0)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTHistory0->SetName("RT History0");
    }

    RTHistory1 = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!RTHistory1)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTHistory1->SetName("RT History1");
    }

    RTReconstructed = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!RTReconstructed)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTReconstructed->SetName("RT Reconstructed");
    }

    // Trace at half resolution
#if ENABLE_HALF_RES
    Width  = Width / 2;
    Height = Height / 2;
#endif

    const UInt32 TextureFlags = TextureFlags_RWTexture | TextureFlag_RTV;

    RTColorDepth = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags, EResourceState::UnorderedAccess, nullptr);
    if (!RTColorDepth)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RTColorDepth->SetName("RayTracing Color Depth");
    }

    Resources.RTRayPDF = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags, EResourceState::UnorderedAccess, nullptr);
    if (!Resources.RTRayPDF)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.RTRayPDF->SetName("RayTracing Ray PDF");
    }

    return true;
}
