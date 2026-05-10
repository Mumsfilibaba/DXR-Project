#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Renderer/RayTracer.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Scene/SceneStaticMesh.h"

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

    RayGenShader = RHI::CreateRayGenShader(Code);
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

    RayClosestHitShader = RHI::CreateRayClosestHitShader(Code);
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

    RayMissShader = RHI::CreateRayMissShader(Code);
    if (!RayMissShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRayTracingPipelineStateDesc PSODesc;
    PSODesc.RayGenShaders           = { RayGenShader.Get() };
    PSODesc.MissShaders             = { RayMissShader.Get() };
    PSODesc.HitGroups               = { FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShader.Get() }) };
    PSODesc.MaxRecursionDepth       = 4;
    PSODesc.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
    PSODesc.MaxPayloadSizeInBytes   = sizeof(FRayPayload);

    Pipeline = RHI::CreateRayTracingPipelineState(PSODesc);
    if (!Pipeline)
    {
        DEBUG_BREAK();
        return false;
    }

	const uint32 Width  = Resources.CurrentRenderWidth;
	const uint32 Height = Resources.CurrentRenderHeight;

    FRHITextureDesc RTOutputDesc = FRHITextureDesc::CreateTexture2D(FGlobalTextureFormats::RTOutputFormat, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture);
    Resources.RTOutput = RHI::CreateTexture(RTOutputDesc, EResourceAccess::UnorderedAccess);
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

    for (const FSceneStaticMesh* StaticMesh : Scene->StaticMeshes)
    {
        TSharedPtr<FMaterial> Material = StaticMesh->GetMaterial();
        if (Material->HasAlphaMask())
        {
            continue;
        }

        uint32 AlbedoIndex = Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->AlbedoMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->NormalMap));
        Resources.RTMaterialTextureCache.Add(SafeGetDefaultSRV(Material->MaterialMap));
        Sampler = Material->GetMaterialSampler();

        const FMatrix3x4 TinyTransform;// = StaticMesh->Actor->GetTransform().GetTinyMatrix();

        uint32 HitGroupIndex = 0;
        if (uint32* ExistingIndex = Resources.RTMeshToHitGroupIndex.Find(StaticMesh->GetMesh().Get()))
        {
            HitGroupIndex = *ExistingIndex;
        }
        else
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
            Resources.RTMeshToHitGroupIndex[StaticMesh->GetMesh().Get()] = HitGroupIndex;
            
            FRayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";

            if (FRHIShaderResourceView* VertexBufferSRV = StaticMesh->GetMesh()->GetVertexBufferSRV(EVertexStream::Packed))
            {
                HitGroupResources.AddShaderResourceView(VertexBufferSRV);
            }
            if (FRHIShaderResourceView* IndexBufferSRV = StaticMesh->GetMesh()->GetIndexBufferSRV())
            {
                HitGroupResources.AddShaderResourceView(IndexBufferSRV);
            }
            
            Resources.RTHitGroupResources.Emplace(HitGroupResources);
        }

        FRHIGeometryAccelerationStructureInstance Instance;
        Instance.Geometry      = StaticMesh->GetRayTracingGeometry();
        Instance.Flags         = ERayTracingInstanceFlags::None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.Emplace(Instance);
    }

    if (!Resources.RTScene)
    {
        FRHISceneAccelerationStructureDesc SceneDesc(MakeArrayView(Resources.RTGeometryInstances), EAccelerationStructureBuildFlags::None);
        Resources.RTScene = RHI::CreateSceneAccelerationStructure(SceneDesc);
    }
    else
    {
        FRHISceneAccelerationStructureBuildDesc BuildSceneDesc;
        BuildSceneDesc.Instances    = Resources.RTGeometryInstances.Data();
        BuildSceneDesc.NumInstances = Resources.RTGeometryInstances.Size();
        BuildSceneDesc.bUpdate      = false;
        CommandList.BuildSceneAccelerationStructure(Resources.RTScene.Get(), BuildSceneDesc);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTOutput->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());

    if (Scene->Skybox)
    {
        FRHITextureRef Skybox = Scene->Skybox->GetCubeMap();
        Resources.GlobalResources.AddShaderResourceView(Skybox->GetShaderResourceView());
    }

    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView());

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

    uint32 Width  = Resources.RTOutput->GetDesc().Extent.X;
    uint32 Height = Resources.RTOutput->GetDesc().Extent.Y;
    CommandList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CommandList.UnorderedAccessTextureBarrier(Resources.RTOutput.Get());
}
