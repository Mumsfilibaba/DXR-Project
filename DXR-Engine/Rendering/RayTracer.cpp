#include "RayTracer.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Resources/Material.h"
#include "Resources/Mesh.h"

#include "Math/Random.h"

Bool RayTracer::Init(FrameResources& Resources)
{
    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RayGen.hlsl", "RayGen", nullptr, EShaderStage::RayGen, EShaderModel::SM_6_3, Code))
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

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ClosestHit.hlsl", "ClosestHit", nullptr, EShaderStage::RayClosestHit, EShaderModel::SM_6_3, Code))
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

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Miss.hlsl", "Miss", nullptr, EShaderStage::RayMiss, EShaderModel::SM_6_3, Code))
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

    RayTracingPipelineStateCreateInfo CreateInfo;
    CreateInfo.RayGen                  = RayGenShader.Get();
    CreateInfo.ClosestHitShaders       = { RayClosestHitShader.Get() };
    CreateInfo.MissShaders             = { RayMissShader.Get() };
    CreateInfo.HitGroups               = { RayTracingHitGroup("HitGroup", nullptr, RayClosestHitShader.Get()) };
    CreateInfo.MaxRecursionDepth       = 4;
    CreateInfo.MaxAttributeSizeInBytes = sizeof(RayIntersectionAttributes);
    CreateInfo.MaxPayloadSizeInBytes   = sizeof(RayPayload);

    Pipeline = CreateRayTracingPipelineState(CreateInfo);
    if (!Pipeline)
    {
        Debug::DebugBreak();
        return false;
    }

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

    RT_History = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!RT_History)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RT_History->SetName("RT History");
    }

    RT_Color_Depth = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!RT_Color_Depth)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RT_Color_Depth->SetName("RayTracing Color Depth");
    }

    Resources.RTRayPDF = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!Resources.RTRayPDF)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.RTRayPDF->SetName("RayTracing Ray PDF");
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

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RTSpatialFilter.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
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
        RTSpatialShader->SetName("RT Spatial Shader");
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

    TArray<ShaderDefine> Defines;
    Defines.EmplaceBack("HORIZONTAL_PASS", "1");

    // Load shader
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Blur.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
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
        BlurHorizontalShader->SetName("SSAO Horizontal Blur Shader");
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
        BlurHorizontalPSO->SetName("SSAO Horizontal Blur PSO");
    }

    Defines.Clear();
    Defines.EmplaceBack("VERTICAL_PASS", "1");

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Blur.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
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
        BlurVerticalShader->SetName("SSAO Vertcial Blur Shader");
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
        BlurVerticalPSO->SetName("SSAO Vertical Blur PSO");
    }

    return true;
}

void RayTracer::Release()
{
    Pipeline.Reset();
    RandomDataBuffer.Reset();
    RTSpatialPSO.Reset();
    RTSpatialShader.Reset();
    RT_Color_Depth.Reset();
    RT_History.Reset();
    BlurHorizontalPSO.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalPSO.Reset();
    BlurVerticalShader.Reset();
}

void RayTracer::Render(CommandList& CmdList, FrameResources& Resources, LightSetup& LightSetup, const Scene& Scene)
{
    static UInt32 FrameIndex = 0;

    FrameIndex++;
    if (FrameIndex >= 64)
    {
        FrameIndex = 0;
    }

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
        if (Cmd.Material->HasAlphaMask())
        {
            continue;
        }

        UInt32 AlbedoIndex = Resources.RTMaterialTextureCache.Add(Mat->AlbedoMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->NormalMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->RoughnessMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->HeightMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->MetallicMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->AOMap->GetShaderResourceView());
        Sampler = Mat->GetMaterialSampler();

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
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->VertexBufferSRV.Get());
            }
            if (Cmd.Mesh->IndexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->IndexBufferSRV.Get());
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
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.EmplaceBack(Instance);
    }

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
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size(), false);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(RT_Color_Depth->GetUnorderedAccessView());
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTRayPDF->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.LightInfoBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(RandomDataBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.PointLightsBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.PointLightsPosRadBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.ShadowCastingPointLightsBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.ShadowCastingPointLightsPosRadBuffer.Get());
    Resources.GlobalResources.AddConstantBuffer(LightSetup.DirectionalLightsBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView());

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

    UInt32 Width  = Resources.RTReflections->GetWidth();
    UInt32 Height = Resources.RTReflections->GetHeight();
    CmdList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CmdList.UnorderedAccessTextureBarrier(RT_Color_Depth.Get());
    CmdList.UnorderedAccessTextureBarrier(Resources.RTRayPDF.Get());

    CmdList.TransitionTexture(RT_Color_Depth.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource);

    CmdList.SetComputePipelineState(RTSpatialPSO.Get());
    
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), RT_Color_Depth->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.RTRayPDF->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView(RTSpatialShader.Get(), Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 4);

    CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), RT_History->GetUnorderedAccessView(), 0);
    CmdList.SetUnorderedAccessView(RTSpatialShader.Get(), Resources.RTReflections->GetUnorderedAccessView(), 1);
    
    CmdList.SetConstantBuffer(RTSpatialShader.Get(), Resources.CameraBuffer.Get(), 0);
    //CmdList.SetConstantBuffer(RTSpatialShader.Get(), RandomDataBuffer.Get(), 1);

    XMUINT3 ThreadGroup = RTSpatialShader->GetThreadGroupXYZ();
    Width  = Math::DivideByMultiple(RT_History->GetWidth(), ThreadGroup.x);
    Height = Math::DivideByMultiple(RT_History->GetHeight(), ThreadGroup.y);
    CmdList.Dispatch(Width, Height, ThreadGroup.z);

    CmdList.TransitionTexture(RT_Color_Depth.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess);
    CmdList.TransitionTexture(Resources.RTRayPDF.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess);

    CmdList.UnorderedAccessTextureBarrier(RT_History.Get());
    CmdList.UnorderedAccessTextureBarrier(Resources.RTReflections.Get());

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

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RT_Color_Depth->GetShaderResourceView()),
        RT_Color_Depth,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.RTRayPDF->GetShaderResourceView()),
        Resources.RTRayPDF,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(RT_History->GetShaderResourceView()),
        RT_History,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);

    Resources.DebugTextures.EmplaceBack(
        MakeSharedRef<ShaderResourceView>(Resources.RTReflections->GetShaderResourceView()),
        Resources.RTReflections,
        EResourceState::UnorderedAccess,
        EResourceState::UnorderedAccess);
}
