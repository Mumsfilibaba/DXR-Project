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

    ReflectionTexture.Reset();
    SSAOBuffer.Reset();
    FinalTarget.Reset();

    for (UInt32 i = 0; i < 5; i++)
    {
        GBuffer[i].Reset();
    }
    
    GBufferSampler.Reset();

    StdInputLayout.Reset();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();

    DebugTextures.Clear();

    MainWindowViewport.Reset();
}