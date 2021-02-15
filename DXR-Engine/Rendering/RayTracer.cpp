#include "RayTracer.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

Bool RayTracer::Init()
{
    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/RayGen.hlsl", "RayGen", nullptr, EShaderStage::RayGen, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    TRef<RayGenShader> RayGen = CreateRayGenShader(Code);
    if (!RayGen)
    {
        Debug::DebugBreak();
        return false;
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ClosestHit.hlsl", "ClosestHit", nullptr, EShaderStage::RayClosestHit, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    TRef<RayClosestHitShader> ClosestHit = CreateRayClosestHitShader(Code);
    if (!ClosestHit)
    {
        Debug::DebugBreak();
        return false;
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Miss.hlsl", "Miss", nullptr, EShaderStage::RayMiss, EShaderModel::SM_6_3, Code))
    {
        Debug::DebugBreak();
        return false;
    }

    TRef<RayMissShader> Miss = CreateRayMissShader(Code);
    if (!Miss)
    {
        Debug::DebugBreak();
        return false;
    }

    RayTracingPipelineStateCreateInfo CreateInfo;
    CreateInfo.RayGen                  = RayGen.Get();
    CreateInfo.ClosestHitShaders       = { ClosestHit.Get() };
    CreateInfo.MissShaders             = { Miss.Get() };
    CreateInfo.HitGroups               = { RayTracingHitGroup("HitGroup", nullptr, ClosestHit.Get()) };
    CreateInfo.MaxRecursionDepth       = 4;
    CreateInfo.MaxAttributeSizeInBytes = sizeof(Float) * 2;
    CreateInfo.MaxPayloadSizeInBytes   = sizeof(Float) * 3 + sizeof(UInt32);

    Pipeline = CreateRayTracingPipelineState(CreateInfo);
    if (!Pipeline)
    {
        Debug::DebugBreak();
        return false;
    }

    return true;
}

void RayTracer::PreRender(CommandList& CmdList, FrameResources& Resources, const Scene& Scene)
{
    TRACE_SCOPE("Gather Instances");

    Resources.RTGeometryInstances.Clear();

    UInt32 InstanceIndexIndex = 0;
    RayTracingShaderResources HitGroupResources;
    for (const MeshDrawCommand& Cmd : Scene.GetMeshDrawCommands())
    {
        const XMFLOAT3X4 TinyTransform = Cmd.CurrentActor->GetTransform().GetTinyMatrix();

        HitGroupResources.Reset();
        Resources.RTHitGroupResources.EmplaceBack(HitGroupResources);


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
        Resources.RTScene = CreateRayTracingScene(RayTracingStructureBuildFlag_AllowUpdate, TArrayView<RayTracingGeometryInstance>(Resources.RTGeometryInstances));
    }
    else
    {
        CmdList.BuildRayTracingScene(Resources.RTScene.Get(), TArrayView<RayTracingGeometryInstance>(Resources.RTGeometryInstances), true);
    }

    CmdList.SetHitGroups(Resources.RTScene.Get(), Pipeline.Get(), TArrayView<RayTracingShaderResources>(Resources.RTHitGroupResources));
}
