#include "FrameResources.h"
#include "LightSetup.h"

void FrameResources::Release()
{
    BackBuffer = nullptr;

    CameraBuffer.Reset();
    TransformBuffer.Reset();

    DirectionalShadowSampler.Reset();
    PointShadowSampler.Reset();
    IrradianceSampler.Reset();

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
    
    GBufferSampler.Reset();

    StdInputLayout.Reset();

    RTScene.Reset();
    RTOutput.Reset();
    RTGeometryInstances.Clear();
    RTHitGroupResources.Clear();
    RTMeshToHitGroupIndex.clear();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();

    DebugTextures.Clear();

    MainWindowViewport.Reset();
}