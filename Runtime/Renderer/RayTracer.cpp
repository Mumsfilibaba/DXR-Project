#include "RayTracer.h"
#include "Renderer.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRayTracer

bool CRayTracer::Init(SFrameResources& Resources)
{
    TArray<uint8> Code;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/RayGen.hlsl", "RayGen", nullptr, ERHIShaderStage::RayGen, EShaderModel::SM_6_3, Code))
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
    else
    {
        RayGenShader->SetName("RayGenShader");
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ClosestHit.hlsl", "ClosestHit", nullptr, ERHIShaderStage::RayClosestHit, EShaderModel::SM_6_3, Code))
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
    else
    {
        RayClosestHitShader->SetName("RayClosestHitShader");
    }

    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/Miss.hlsl", "Miss", nullptr, ERHIShaderStage::RayMiss, EShaderModel::SM_6_3, Code))
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
    else
    {
        RayMissShader->SetName("RayMissShader");
    }

    SRHIRayTracingPipelineStateDesc CreateInfo;
    CreateInfo.RayGen = RayGenShader.Get();
    CreateInfo.ClosestHitShaders = { RayClosestHitShader.Get() };
    CreateInfo.MissShaders = { RayMissShader.Get() };
    CreateInfo.HitGroups = { SRayTracingHitGroup("HitGroup", nullptr, RayClosestHitShader.Get()) };
    CreateInfo.MaxRecursionDepth = 4;
    CreateInfo.MaxAttributeSizeInBytes = sizeof(SRayIntersectionAttributes);
    CreateInfo.MaxPayloadSizeInBytes = sizeof(SRayPayload);

    Pipeline = RHICreateRayTracingPipelineState(CreateInfo);
    if (!Pipeline)
    {
        CDebug::DebugBreak();
        return false;
    }

    uint32 Width = Resources.MainWindowViewport->GetWidth();
    uint32 Height = Resources.MainWindowViewport->GetHeight();
    Resources.RTOutput = RHICreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, ERHIResourceAccess::UnorderedAccess, nullptr);
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
    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    CRHISamplerState* Sampler = nullptr;

    for (const SMeshDrawCommand& Cmd : Scene.GetMeshDrawCommands())
    {
        CMaterial* Mat = Cmd.Material;
        if (Cmd.Material->HasAlphaMask())
        {
            continue;
        }

        uint32 AlbedoIndex = Resources.RTMaterialTextureCache.Add(Mat->AlbedoMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->NormalMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->RoughnessMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->HeightMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->MetallicMap->GetShaderResourceView());
        Resources.RTMaterialTextureCache.Add(Mat->AOMap->GetShaderResourceView());
        Sampler = Mat->GetMaterialSampler();

        const CMatrix3x4 TinyTransform = Cmd.CurrentActor->GetTransform().GetTinyMatrix();
        uint32 HitGroupIndex = 0;

        auto HitGroupIndexPair = Resources.RTMeshToHitGroupIndex.find(Cmd.Mesh);
        if (HitGroupIndexPair == Resources.RTMeshToHitGroupIndex.end())
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
            Resources.RTMeshToHitGroupIndex[Cmd.Mesh] = HitGroupIndex;

            SRayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";
            if (Cmd.Mesh->VertexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->VertexBufferSRV.Get());
            }
            if (Cmd.Mesh->IndexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->IndexBufferSRV.Get());
            }

            Resources.RTHitGroupResources.Emplace(HitGroupResources);
        }
        else
        {
            HitGroupIndex = HitGroupIndexPair->second;
        }

        SRHIRayTracingGeometryInstance Instance;
        Instance.Instance = MakeSharedRef<CRHIRayTracingGeometry>(Cmd.Geometry);
        Instance.Flags = RayTracingInstanceFlags_None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = AlbedoIndex;
        Instance.Mask = 0xff;
        Instance.Transform = TinyTransform;
        Resources.RTGeometryInstances.Emplace(Instance);
    }

    if (!Resources.RTScene)
    {
        Resources.RTScene = RHICreateRayTracingScene(RayTracingStructureBuildFlag_None, Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size());
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
    CmdList.SetRayTracingBindings(
        Resources.RTScene.Get(),
        Pipeline.Get(),
        &Resources.GlobalResources,
        &Resources.RayGenLocalResources,
        &Resources.MissLocalResources,
        Resources.RTHitGroupResources.Data(),
        Resources.RTHitGroupResources.Size());

    uint32 Width = Resources.RTOutput->GetWidth();
    uint32 Height = Resources.RTOutput->GetHeight();
    CmdList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);

    CmdList.UnorderedAccessTextureBarrier(Resources.RTOutput.Get());

    AddDebugTexture(
        MakeSharedRef<CRHIShaderResourceView>(Resources.RTOutput->GetShaderResourceView()),
        Resources.RTOutput,
        ERHIResourceAccess::UnorderedAccess,
        ERHIResourceAccess::UnorderedAccess);
}
