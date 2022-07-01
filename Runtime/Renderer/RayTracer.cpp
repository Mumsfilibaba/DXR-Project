#include "RayTracer.h"
#include "Renderer.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRayTracer

bool CRayTracer::Init(SFrameResources& Resources)
{
    TArray<uint8> Code;
    
    {
        FShaderCompileInfo CompileInfo("RayGen", EShaderModel::SM_6_3, EShaderStage::RayGen);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/RayGen.hlsl", CompileInfo, Code))
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    RayGenShader = RHICreateRayGenShader(Code);
    if (!RayGenShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("ClosestHit", EShaderModel::SM_6_3, EShaderStage::RayClosestHit);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ClosestHit.hlsl", CompileInfo, Code))
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    RayClosestHitShader = RHICreateRayClosestHitShader(Code);
    if (!RayClosestHitShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("Miss", EShaderModel::SM_6_3, EShaderStage::RayMiss);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Miss.hlsl", CompileInfo, Code))
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    RayMissShader = RHICreateRayMissShader(Code);
    if (!RayMissShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    FRHIRayTracingPipelineStateInitializer PSOInitializer;
    PSOInitializer.RayGenShaders           = { RayGenShader.Get() };
    PSOInitializer.MissShaders             = { RayMissShader.Get() };
    PSOInitializer.HitGroups               = { FRHIRayTracingHitGroupInitializer("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShader.Get() }) };
    PSOInitializer.MaxRecursionDepth       = 4;
    PSOInitializer.MaxAttributeSizeInBytes = sizeof(FRayIntersectionAttributes);
    PSOInitializer.MaxPayloadSizeInBytes   = sizeof(FRayPayload);

    Pipeline = RHICreateRayTracingPipelineState(PSOInitializer);
    if (!Pipeline)
    {
        CDebug::DebugBreak();
        return false;
    }

    uint32 Width  = Resources.MainWindowViewport->GetWidth();
    uint32 Height = Resources.MainWindowViewport->GetHeight();

    FRHITexture2DInitializer RTOutputInitializer(Resources.RTOutputFormat, Width, Height, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::UnorderedAccess);
    Resources.RTOutput = RHICreateTexture2D(RTOutputInitializer);
    if (!Resources.RTOutput)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Resources.RTOutput->SetName("RayTracing Output");
    }

    return true;
}

void CRayTracer::Release()
{
    Pipeline.Reset();
    RayGenShader.Reset();
    RayMissShader.Reset();
    RayClosestHitShader.Reset();
}

void CRayTracer::PreRender(FRHICommandList& CmdList, SFrameResources& Resources, const CScene& Scene)
{
    UNREFERENCED_VARIABLE(Scene);

    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    FRHISamplerState* Sampler = nullptr;

    for (int32 Index = 0; Index < Resources.GlobalMeshDrawCommands.Size(); ++Index)
    {
        const SMeshDrawCommand& Command = Resources.GlobalMeshDrawCommands[Index];

        CMaterial* Material = Command.Material;
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

        const CMatrix3x4 TinyTransform = Command.CurrentActor->GetTransform().GetTinyMatrix();
        uint32 HitGroupIndex = 0;

        auto HitGroupIndexPair = Resources.RTMeshToHitGroupIndex.find(Command.Mesh);
        if (HitGroupIndexPair == Resources.RTMeshToHitGroupIndex.end())
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
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
        FRHIRayTracingSceneInitializer SceneInitializer(Resources.RTGeometryInstances.CreateView(), EAccelerationStructureBuildFlags::None);
        Resources.RTScene = RHICreateRayTracingScene(SceneInitializer);
    }
    else
    {
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), TArrayView(Resources.RTGeometryInstances.CreateView()), false);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTOutput->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView());

    for (uint32 i = 0; i < Resources.RTMaterialTextureCache.Size(); i++)
    {
        Resources.GlobalResources.AddShaderResourceView(Resources.RTMaterialTextureCache.Get(i));
    }

    Resources.RayGenLocalResources.Reset();
    Resources.RayGenLocalResources.Identifier = "RayGen";

    Resources.MissLocalResources.Reset();
    Resources.MissLocalResources.Identifier = "Miss";

    // TODO: NO MORE BINDINGS CAN BE BOUND BEFORE DISPATCH RAYS, FIX THIS
    CmdList.SetRayTracingBindings( Resources.RTScene.Get()
                                 , Pipeline.Get()
                                 , &Resources.GlobalResources
                                 , &Resources.RayGenLocalResources
                                 , &Resources.MissLocalResources
                                 , Resources.RTHitGroupResources.Data()
                                 , Resources.RTHitGroupResources.Size());

    uint32 Width  = Resources.RTOutput->GetWidth();
    uint32 Height = Resources.RTOutput->GetHeight();
    CmdList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CmdList.UnorderedAccessTextureBarrier(Resources.RTOutput.Get());

    AddDebugTexture( MakeSharedRef<FRHIShaderResourceView>(Resources.RTOutput->GetShaderResourceView())
                   , Resources.RTOutput
                   , EResourceAccess::UnorderedAccess
                   , EResourceAccess::UnorderedAccess);
}
