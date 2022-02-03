#include "FrameResources.h"
#include "LightSetup.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SFrameResources

void SFrameResources::Release()
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

    for (uint32 i = 0; i < 5; i++)
    {
        GBuffer[i].Reset();
    }

    ReducedDepthBuffer[0].Reset();
    ReducedDepthBuffer[1].Reset();

    GBufferSampler.Reset();

    StdInputLayout.Reset();

    RTScene.Reset();
    RTOutput.Reset();
    RTGeometryInstances.Clear();
    RTHitGroupResources.Clear();
    RTMeshToHitGroupIndex.clear();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();

    MainWindowViewport.Reset();
}