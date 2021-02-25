#include "RayTracer.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Resources/Material.h"
#include "Resources/Mesh.h"

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
    Resources.RTOutput = CreateTexture2D(Resources.RTOutputFormat, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::UnorderedAccess, nullptr);
    if (!Resources.RTOutput)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.RTOutput->SetName("RayTracing Output");
    }

    return true;
}

void RayTracer::Release()
{
    Pipeline.Reset();
}

void RayTracer::PreRender(CommandList& CmdList, FrameResources& Resources, const Scene& Scene)
{
    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();
    Resources.RTHitGroupResources.Clear();
    Resources.RTMaterialToHitGroupIndex.clear();

    UInt32 HitGroupIndex      = 0;
    UInt32 InstanceIndexIndex = 0;
    for (const MeshDrawCommand& Cmd : Scene.GetMeshDrawCommands())
    {
        const XMFLOAT3X4 TinyTransform = Cmd.CurrentActor->GetTransform().GetTinyMatrix();

        // TODO: Change this to something less performant
        auto HitGroupIndexPair = Resources.RTMaterialToHitGroupIndex.find(Cmd.Material);
        if (HitGroupIndex < 1)
        {
            HitGroupIndex = Resources.RTHitGroupResources.Size();
            Resources.RTMaterialToHitGroupIndex[Cmd.Material] = HitGroupIndex;
            HitGroupIndex++;

            RayTracingShaderResources HitGroupResources;
            HitGroupResources.Identifier = "HitGroup";
           /* HitGroupResources.AddConstantBuffer(Cmd.Material->GetMaterialBuffer());
            HitGroupResources.AddSamplerState(Cmd.Material->GetMaterialSampler());
            if (Cmd.Material->AlbedoMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->AlbedoMap->GetShaderResourceView());
            }
            if (Cmd.Material->NormalMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->NormalMap->GetShaderResourceView());
            }
            if (Cmd.Material->RoughnessMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->RoughnessMap->GetShaderResourceView());
            }
            if (Cmd.Material->HeightMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->HeightMap->GetShaderResourceView());
            }
            if (Cmd.Material->MetallicMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->MetallicMap->GetShaderResourceView());
            }
            if (Cmd.Material->AOMap)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Material->AOMap->GetShaderResourceView());
            }
            if (Cmd.Mesh->VertexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->VertexBufferSRV.Get());
            }
            if (Cmd.Mesh->IndexBufferSRV)
            {
                HitGroupResources.AddShaderResourceView(Cmd.Mesh->IndexBufferSRV.Get());
            }*/

            Resources.RTHitGroupResources.EmplaceBack(HitGroupResources);
        }
        //else
        //{
        //    HitGroupIndex = HitGroupIndexPair->second;
        //}

        RayTracingGeometryInstance Instance;
        Instance.Instance      = MakeSharedRef<RayTracingGeometry>(Cmd.Geometry);
        Instance.Flags         = RayTracingInstanceFlags_None;
        Instance.HitGroupIndex = 0;
        Instance.InstanceIndex = InstanceIndexIndex++;
        Instance.Mask          = 0xff;
        Instance.Transform     = TinyTransform;
        Resources.RTGeometryInstances.EmplaceBack(Instance);
    }

    if (!Resources.RTScene)
    {
        Resources.RTScene = CreateRayTracingScene(RayTracingStructureBuildFlag_AllowUpdate, Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size());
    }
    else
    {
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), Resources.RTGeometryInstances.Data(), Resources.RTGeometryInstances.Size(), false);
    }

    Resources.GlobalResources.Reset();
    Resources.GlobalResources.AddUnorderedAccessView(Resources.RTOutput->GetUnorderedAccessView());
    Resources.GlobalResources.AddConstantBuffer(Resources.CameraBuffer.Get());
    // TODO: Change to correct samplers
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddSamplerState(Resources.GBufferSampler.Get());
    Resources.GlobalResources.AddShaderResourceView(Resources.RTScene->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.Skybox->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView());
    Resources.GlobalResources.AddShaderResourceView(Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView());

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

    UInt32 Width  = Resources.RTOutput->GetWidth();
    UInt32 Height = Resources.RTOutput->GetHeight();
    CmdList.DispatchRays(Resources.RTScene.Get(), Pipeline.Get(), Width, Height, 1);
}
