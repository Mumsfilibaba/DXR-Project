#include "RayTracer.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"

Bool RayTracer::Init()
{
    return true;
}

void RayTracer::PreRender(CommandList& CmdList, FrameResources& Resources, const Scene& Scene)
{
    TRACE_SCOPE("Gather Instances");

    for (const MeshDrawCommand& Cmd : Scene.GetMeshDrawCommands())
    {
        const XMFLOAT3X4 TinyTransform = Cmd.CurrentActor->GetTransform().GetTinyMatrix();
        Resources.RTGeometryInstances.EmplaceBack(MakeSharedRef<RayTracingGeometry>(Cmd.Geometry), Cmd.Material, TinyTransform);
    }

    if (!Resources.RTScene)
    {
        Resources.RTScene = CreateRayTracingScene(RayTracingStructureBuildFlag_None, Resources.RTGeometryInstances);
    }
}
