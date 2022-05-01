#include "RayTracer.h"
#include "Renderer.h"

#include "RHI/RHICoreInstance.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRayTracer

bool CRayTracer::Init(SFrameResources& Resources)
{
    TArray<uint8> Code;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/RayGen.hlsl", "RayGen", nullptr, EShaderStage::RayGen, EShaderModel::SM_6_3, Code))
    {
        CDebug::DebugBreak();
        return false;
    }

    RayGenShader = RHICreateRayGenShader(Code);
    if (!RayGenShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ClosestHit.hlsl", "ClosestHit", nullptr, EShaderStage::RayClosestHit, EShaderModel::SM_6_3, Code))
    {
        CDebug::DebugBreak();
        return false;
    }

    RayClosestHitShader = RHICreateRayClosestHitShader(Code);
    if (!RayClosestHitShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Miss.hlsl", "Miss", nullptr, EShaderStage::RayMiss, EShaderModel::SM_6_3, Code))
    {
        CDebug::DebugBreak();
        return false;
    }

    RayMissShader = RHICreateRayMissShader(Code);
    if (!RayMissShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    CRHIRayTracingPipelineStateInitializer Initializer;
    Initializer.RayGenShaders           = { RayGenShader.Get() };
    Initializer.MissShaders             = { RayMissShader.Get() };
    Initializer.HitGroups               = { CRHIRayTracingHitGroupInitializer("HitGroup", ERayTracingHitGroupType::Triangles, { RayClosestHitShader.Get() }) };
    Initializer.MaxRecursionDepth       = 4;
    Initializer.MaxAttributeSizeInBytes = sizeof(SRayIntersectionAttributes);
    Initializer.MaxPayloadSizeInBytes   = sizeof(SRayPayload);

    Pipeline = RHICreateRayTracingPipelineState(Initializer);
    if (!Pipeline)
    {
        CDebug::DebugBreak();
        return false;
    }

    uint32 Width  = Resources.MainWindowViewport->GetWidth();
    uint32 Height = Resources.MainWindowViewport->GetHeight();
    Resources.RTOutput = RHICreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::UnorderedAccess, nullptr);
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

void CRayTracer::PreRender(CRHICommandList& CmdList, SFrameResources& Resources, const CScene& Scene)
{
    UNREFERENCED_VARIABLE(Scene);

    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    CRHISamplerState* Sampler = nullptr;

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

            SRayTracingShaderResources HitGroupResources;
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

        SRayTracingGeometryInstance Instance;
        Instance.Instance      = MakeSharedRef<CRHIRayTracingGeometry>(Command.Geometry);
        Instance.Flags         = RayTracingInstanceFlags_None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.Emplace(Instance);
    }

    if (!Resources.RTScene)
    {
        Resources.RTScene = RHICreateRayTracingScene(RayTracingStructureBuildFlag_None, Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size());
    }
    else
    {
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size(), false);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTOutput->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Sampler);
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetDefaultShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetDefaultShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDefaultShaderResourceView());

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

    AddDebugTexture( MakeSharedRef<CRHIShaderResourceView>(Resources.RTOutput->GetDefaultShaderResourceView())
                   , Resources.RTOutput
                   , EResourceAccess::UnorderedAccess
                   , EResourceAccess::UnorderedAccess);
}
