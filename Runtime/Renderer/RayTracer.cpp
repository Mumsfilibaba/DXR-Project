#include "RayTracer.h"
#include "SceneRenderer.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

bool FRayTracer::Initialize(FFrameResources& Resources)
{
    TArray<uint8> Code;
    
    {
        FShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_3, EShaderStage::RayGen);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    RayGenShader = RHICreateRayGenShader(Code);
    if (!RayGenShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_3, EShaderStage::RayClosestHit);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    RayClosestHitShader = RHICreateRayClosestHitShader(Code);
    if (!RayClosestHitShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_3, EShaderStage::RayMiss);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    RayMissShader = RHICreateRayMissShader(Code);
    if (!RayMissShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRayTracingPipelineStateDesc PSOInitializer;
    PSOInitializer.RayGenShaders           = { RayGenShader.Get() };
    PSOInitializer.MissShaders             = { RayMissShader.Get() };
    PSOInitializer.HitGroups               = { FRHIRayTracingHitGroupDesc("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShader.Get() }) };
    PSOInitializer.MaxRecursionDepth       = 4;
    PSOInitializer.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
    PSOInitializer.MaxPayloadSizeInBytes   = sizeof(FRayPayload);

    Pipeline = RHICreateRayTracingPipelineState(PSOInitializer);
    if (!Pipeline)
    {
        DEBUG_BREAK();
        return false;
    }

    uint32 Width  = Resources.MainViewport->GetWidth();
    uint32 Height = Resources.MainViewport->GetHeight();

    FRHITextureDesc RTOutputDesc = FRHITextureDesc::CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    Resources.RTOutput = RHICreateTexture(RTOutputDesc, EResourceAccess::UnorderedAccess);
    if (!Resources.RTOutput)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.RTOutput->SetDebugName("RayTracing Output");
    }

    return true;
}

void FRayTracer::Release()
{
    Pipeline.Reset();
    RayGenShader.Reset();
    RayMissShader.Reset();
    RayClosestHitShader.Reset();
}

void FRayTracer::PreRender(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    UNREFERENCED_VARIABLE(Scene);

    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    FRHISamplerState* Sampler = nullptr;

    for (const FProxySceneComponent* Component : Scene->Primitives)
    {
        FMaterial* Material = Component->Material;
        if (Component->Material->HasAlphaMask())
        {
            continue;
        }

        uint32 AlbedoIndex = Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->AlbedoMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->NormalMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->RoughnessMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->HeightMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->MetallicMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->AOMap));
        Sampler = Material->GetMaterialSampler();

        const FMatrix3x4 TinyTransform = Component->CurrentActor->GetTransform().GetTinyMatrix();

        uint32 HitGroupIndex = 0;
        if (uint32* ExistingIndex = Resources.RTMeshToHitGroupIndex.Find(Component->Mesh))
        {
            HitGroupIndex = *ExistingIndex;
        }
        else
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
            Resources.RTMeshToHitGroupIndex[Component->Mesh] = HitGroupIndex;
            
            FRayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";

            if (Component->Mesh->VertexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Component->Mesh->VertexBufferSRV.Get());
            }
            if (Component->Mesh->IndexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Component->Mesh->IndexBufferSRV.Get());
            }
            
            Resources.RTHitGroupResources.Emplace(HitGroupResources);
        }

        FRHIRayTracingGeometryInstance Instance;
        Instance.Geometry      = Component->Geometry;
        Instance.Flags         = ERayTracingInstanceFlags::None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.Emplace(Instance);
    }

    if (!Resources.RTScene)
    {
        FRHIRayTracingSceneDesc SceneInitializer(MakeArrayView(Resources.RTGeometryInstances), EAccelerationStructureBuildFlags::None);
        Resources.RTScene = RHICreateRayTracingScene(SceneInitializer);
    }
    else
    {
        FRayTracingSceneBuildInfo BuildScene;
        BuildScene.Instances    = Resources.RTGeometryInstances.Data();
        BuildScene.NumInstances = Resources.RTGeometryInstances.Size();
        BuildScene.bUpdate      = false;
        CommandList.BuildRayTracingScene(Resources.RTScene.Get(), BuildScene);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTOutput->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView());

    for (uint32 i = 0; i < Resources.RTMaterialTextureCache.Size(); i++)
    {
        Resources.GlobalResources.AddShaderResourceView(Resources.RTMaterialTextureCache.Get(i));
    }

    Resources.RayGenLocalResources.Reset();
    Resources.RayGenLocalResources.Identifier = "RayGen";

    Resources.MissLocalResources.Reset();
    Resources.MissLocalResources.Identifier = "Miss";

    // TODO: NO MORE BINDINGS CAN BE BOUND BEFORE DISPATCH RAYS, FIX THIS
    CommandList.SetRayTracingBindings(
        Resources.RTScene.Get(),
        Pipeline.Get(),
        &Resources.GlobalResources,
        &Resources.RayGenLocalResources,
        &Resources.MissLocalResources,
        Resources.RTHitGroupResources.Data(),
        Resources.RTHitGroupResources.Size());

    uint32 Width  = Resources.RTOutput->GetWidth();
    uint32 Height = Resources.RTOutput->GetHeight();
    CommandList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CommandList.UnorderedAccessTextureBarrier(Resources.RTOutput.Get());

    GetRenderer()->AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.RTOutput->GetShaderResourceView()),
        Resources.RTOutput,
        EResourceAccess::UnorderedAccess,
        EResourceAccess::UnorderedAccess);
}
