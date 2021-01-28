#include "FrameResources.h"

void FrameResources::Release()
{
    BackBuffer    = nullptr;
    BackBufferRTV = nullptr;

    CameraBuffer.Reset();
    TransformBuffer.Reset();

    ShadowMapSampler.Reset();
    ShadowMapCompSampler.Reset();
    IrradianceSampler.Reset();

    Skybox.Reset();
    SkyboxSRV.Reset();

    ReflectionTexture.Reset();
    ReflectionTextureSRV.Reset();
    ReflectionTextureUAV.Reset();

    IntegrationLUT.Reset();
    IntegrationLUTSRV.Reset();
    IntegrationLUTSampler.Reset();

    FinalTarget.Reset();
    FinalTargetSRV.Reset();
    FinalTargetRTV.Reset();
    FinalTargetUAV.Reset();

    for (UInt32 i = 0; i < 5; i++)
    {
        GBuffer[i].Reset();
        GBufferSRVs[i].Reset();
        GBufferRTVs[i].Reset();
    }
    
    GBufferDSV.Reset();
    GBufferSampler.Reset();

    SSAOBuffer.Reset();
    SSAOBufferSRV.Reset();
    SSAOBufferUAV.Reset();

    StdInputLayout.Reset();

    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();

    DebugTextures.Clear();

    MainWindowViewport.Reset();
}

void SceneLightSetup::Release()
{
    PointLightBuffer.Reset();
    DirectionalLightBuffer.Reset();

    PointLightShadowMaps.Reset();
    PointLightShadowMapSRV.Reset();
    
    for (auto& DSVCube : PointLightShadowMapDSVs)
    {
        for (UInt32 i = 0; i < 6; i++)
        {
            DSVCube[i].Reset();
        }
    }

    DirLightShadowMapSRV.Reset();
    DirLightShadowMapDSV.Reset();
    DirLightShadowMaps.Reset();

    IrradianceMap.Reset();
    IrradianceMapUAV.Reset();
    IrradianceMapSRV.Reset();

    SpecularIrradianceMap.Reset();
    SpecularIrradianceMapSRV.Reset();

    for (auto& UAV : SpecularIrradianceMapUAVs)
    {
        UAV.Reset();
    }
}
