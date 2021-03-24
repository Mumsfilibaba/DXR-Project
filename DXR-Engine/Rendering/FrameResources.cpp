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

    for (UInt32 i = 0; i < 6; i++)
    {
        GBuffer[i].Reset();
    }
    
    PrevDepth.Reset();
    PrevGeomNormals.Reset();

    GBufferSampler.Reset();

    StdInputLayout.Reset();

    RTScene.Reset();
    RTReflections.Reset();
    RTRayPDF.Reset();
    RTGeometryInstances.Clear();
    RTHitGroupResources.Clear();
    RTMeshToHitGroupIndex.clear();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();

    DebugTextures.Clear();

    BlueNoise.Reset();

    MainWindowViewport.Reset();
}