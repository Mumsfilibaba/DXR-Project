#include "RayTracer.h"
#include "Renderer.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "Core/Misc/FrameProfiler.h"

bool FRayTracer::Init(FFrameResources& Resources)
{
    TArray<uint8> Code;
    
    {
        FRHIShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_3, EShaderStage::RayGen);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
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
        FRHIShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_3, EShaderStage::RayClosestHit);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
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
        FRHIShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_3, EShaderStage::RayMiss);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
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

    uint32 Width  = Resources.MainWindowViewport->GetWidth();
    uint32 Height = Resources.MainWindowViewport->GetHeight();

    FRHITextureDesc RTOutputDesc = FRHITextureDesc::CreateTexture2D(
        Resources.RTOutputFormat, 
        Width, 
        Height, 
        1, 
        1, 
        ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    Resources.RTOutput = RHICreateTexture(RTOutputDesc, EResourceAccess::UnorderedAccess);
    if (!Resources.RTOutput)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.RTOutput->SetName("RayTracing Output");
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

void FRayTracer::PreRender(FRHICommandList& CommandList, FFrameResources& Resources, const FScene& Scene)
{
    UNREFERENCED_VARIABLE(Scene);

    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    FRHISamplerState* Sampler = nullptr;

    for (int32 Index = 0; Index < Resources.GlobalMeshDrawCommands.GetSize(); ++Index)
    {
        const FMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[Index];

        FMaterial* Material = Command.Material;
        if (Command.Material->HasAlphaMask())
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

        const FMatrix3x4 TinyTransform = Command.CurrentActor->GetTransform().GetTinyMatrix();
        uint32 HitGroupIndex = 0;

        auto HitGroupIndexPair = Resources.RTMeshToHitGroupIndex.find(Command.Mesh);
        if (HitGroupIndexPair == Resources.RTMeshToHitGroupIndex.end())
        {
            HitGroupIndex = Resources.RTHitGroupResources.GetSize();
            Resources.RTMeshToHitGroupIndex[Command.Mesh] = HitGroupIndex;

            FRayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";
            if (Command.Mesh->VertexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Command.Mesh->VertexBufferSRV.Get());
            }
            if (Command.Mesh->IndexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Command.Mesh->IndexBufferSRV.Get());
            }

            Resources.RTHitGroupResources.Emplace(HitGroupResources);
        }
        else
        {
            HitGroupIndex = HitGroupIndexPair->second;
        }

        FRHIRayTracingGeometryInstance Instance;
        Instance.Geometry      = Command.Geometry;
        Instance.Flags         = ERayTracingInstanceFlags::None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.Emplace(Instance);
    }

    if (!Resources.RTScene)
    {
        FRHIRayTracingSceneDesc SceneInitializer(Resources.RTGeometryInstances.CreateView(), EAccelerationStructureBuildFlags::None);
        Resources.RTScene = RHICreateRayTracingScene(SceneInitializer);
    }
    else
    {
        CommandList.BuildRayTracingScene(Resources.RTScene.Get(), TArrayView(Resources.RTGeometryInstances.CreateView()), false);
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

    for (uint32 i = 0; i < Resources.RTMaterialTextureCache.GetSize(); i++)
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
        Resources.RTHitGroupResources.GetData(),
        Resources.RTHitGroupResources.GetSize());

    uint32 Width  = Resources.RTOutput->GetWidth();
    uint32 Height = Resources.RTOutput->GetHeight();
    CommandList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CommandList.UnorderedAccessTextureBarrier(Resources.RTOutput.Get());

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(Resources.RTOutput->GetShaderResourceView()),
        Resources.RTOutput,
        EResourceAccess::UnorderedAccess,
        EResourceAccess::UnorderedAccess);
}
