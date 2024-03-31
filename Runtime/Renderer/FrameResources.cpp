#include "FrameResources.h"
#include "LightSetup.h"

void FFrameResources::Release()
{
    BackBuffer = nullptr;

    CameraBuffer.Reset();
    TransformBuffer.Reset();

    PointLightShadowSampler.Reset();
    IrradianceSampler.Reset();
    DirectionalLightShadowSampler.Reset();

    IntegrationLUT.Reset();
    IntegrationLUTSampler.Reset();

    Skybox.Reset();

    FXAASampler.Reset();

    SSAOBuffer.Reset();
    FinalTarget.Reset();

    for (FRHITextureRef& Buffer : GBuffer)
    {
        Buffer.Reset();
    }

    ReducedDepthBuffer[0].Reset();
    ReducedDepthBuffer[1].Reset();

    GBufferSampler.Reset();

    MeshInputLayout.Reset();

    RTScene.Reset();
    RTOutput.Reset();
    RTGeometryInstances.Clear();
    RTHitGroupResources.Clear();
    RTMeshToHitGroupIndex.Clear();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();
}
