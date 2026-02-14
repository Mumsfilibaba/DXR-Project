#include "RHI/RHI.h"
#include "Core/Math/Math.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/FrameResources.h"
#include "Renderer/Scene/Scene.h"

static TAutoConsoleVariable<int32> CVarCSMCascadeSize(
    "Renderer.CSM.CascadeSize",
    "Specifies the resolution of each Shadow Cascade",
    2048);

static TAutoConsoleVariable<int32> CVarPointLightShadowMapSize(
    "Renderer.Shadows.PointLightShadowMapSize",
    "Specifies the resolution of each Shadow Cascade",
    512);

static TAutoConsoleVariable<int32> CVarEnvironmentIrradianceProbeSize(
    "Renderer.Environment.IrradianceProbeSize",
    "Specifies the resolution of each Irradiance Probe (Cube-Map) size",
    32);

static TAutoConsoleVariable<int32> CVarEnvironmentSpecularIrradianceProbeSize(
    "Renderer.Environment.SpecularIrradianceProbeSize",
    "Specifies the resolution of each Specular Irradiance Probe (Cube-Map) size",
    256);

static int32 ClampTextureSize(int32 MinSize, int32 MaxSize, int32 NewSize)
{
    const int32 Result = Math::Clamp(NewSize, MinSize, MaxSize);
    return Math::ClosestPowerOfTwo(Result);
}

bool FFrameResources::UpdateCascadeSizeFromCVar()
{
    const int32 NewCascadeSize = ClampTextureSize(512, 4096, CVarCSMCascadeSize.GetValue());
    if (NewCascadeSize != CascadeSize)
    {
        CascadeSize = NewCascadeSize;
        CascadeSizeDirty = true;
        return true;
    }

    return false;
}

FFrameResources::FFrameResources()
    : DirectionalLightDataDirty(true)
    , CascadeSplitLambda(0.0f)
    , CascadeGenerationDataDirty(true)
    , CascadeSizeDirty(false)
{
}

FFrameResources::~FFrameResources()
{
}

bool FFrameResources::Initialize()
{
    // Initialize the light-setup from CVars
    CascadeSize                 = ClampTextureSize(512, 4096, CVarCSMCascadeSize.GetValue());
    PointLightShadowSize        = ClampTextureSize(128, 1024, CVarPointLightShadowMapSize.GetValue());
    IrradianceProbeSize         = ClampTextureSize(32, 512, CVarEnvironmentIrradianceProbeSize.GetValue());
    SpecularIrradianceProbeSize = ClampTextureSize(256, 1024, CVarEnvironmentSpecularIrradianceProbeSize.GetValue());

    // Directional-Light
    FRHIBufferInfo BufferInfo;
    BufferInfo.Stride = sizeof(FDirectionalLightDataHLSL);
    BufferInfo.Size   = sizeof(FDirectionalLightDataHLSL);
    BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
    BufferInfo.bEnableResourceStateTracking = true;

    DirectionalLightDataBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);

    if (!DirectionalLightDataBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        DirectionalLightDataBuffer->SetDebugName("DirectionalLightData Buffer");
    }

    BufferInfo.Stride = sizeof(FCascadeGenerationInfoHLSL);
    BufferInfo.Size   = sizeof(FCascadeGenerationInfoHLSL);

    CascadeGenerationDataBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);

    if (!CascadeGenerationDataBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CascadeGenerationDataBuffer->SetDebugName("CascadeGenerationData Buffer");
    }

    // Point-Lights
    PointLightsData.Reserve(MAX_LIGHTS_PER_TILE);

    BufferInfo.Stride = PointLightsData.Stride();
    BufferInfo.Size   = PointLightsData.CapacityInBytes();
    BufferInfo.bEnableResourceStateTracking = true;

    PointLightsBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);

    if (!PointLightsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PointLightsBuffer->SetDebugName("PointLights Buffer");
    }

    PointLightsPosRad.Reserve(MAX_LIGHTS_PER_TILE);

    BufferInfo.Stride = PointLightsPosRad.Stride();
    BufferInfo.Size   = PointLightsPosRad.CapacityInBytes();
    BufferInfo.bEnableResourceStateTracking = true;

    PointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!PointLightsPosRadBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PointLightsPosRadBuffer->SetDebugName("PointLights Position and Radius Buffer");
    }

    ShadowCastingPointLightsData.Reserve(NUM_SHADOW_CASTING_POINT_LIGHTS);

    BufferInfo.Stride = ShadowCastingPointLightsData.Stride();
    BufferInfo.Size   = ShadowCastingPointLightsData.CapacityInBytes();
    BufferInfo.bEnableResourceStateTracking = true;

    ShadowCastingPointLightsBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!ShadowCastingPointLightsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadowCastingPointLightsBuffer->SetDebugName("ShadowCasting PointLights Buffer");
    }

    ShadowCastingPointLightsPosRad.Reserve(NUM_SHADOW_CASTING_POINT_LIGHTS);

    BufferInfo.Stride = ShadowCastingPointLightsPosRad.Stride();
    BufferInfo.Size   = ShadowCastingPointLightsPosRad.CapacityInBytes();
    BufferInfo.bEnableResourceStateTracking = true;

    ShadowCastingPointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!ShadowCastingPointLightsPosRadBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadowCastingPointLightsPosRadBuffer->SetDebugName("ShadowCastingPointLightsPosRadBuffer");
    }

    // Light-Probes
    LightProbeInfos.Reserve(NUM_LIGHT_PROBES);

    BufferInfo.Stride = LightProbeInfos.Stride();
    BufferInfo.Size   = LightProbeInfos.CapacityInBytes();
    BufferInfo.bEnableResourceStateTracking = true;

    LightProbeBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!LightProbeBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        LightProbeBuffer->SetDebugName("Light-Probe Infos");
    }

    return true;
}

void FFrameResources::BuildLightBuffers(FRHICommandList& CommandList, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    UpdateCascadeSizeFromCVar();

    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    ShadowCastingPointLightsData.Clear();
    LightProbeInfos.Clear();

    // Update DirectionalLight
    if (FSceneDirectionalLight* DirectionalLight = Scene->DirectionalLight)
    {
        // Update data necessary for other stages
        DirectionalLightData.Color         = DirectionalLight->GetColor();
        DirectionalLightData.ShadowBias    = DirectionalLight->GetShadowBias();
        DirectionalLightData.Direction     = DirectionalLight->GetDirectionVector();
        DirectionalLightData.UpVector      = DirectionalLight->GetUpVector();
        DirectionalLightData.LightSize     = DirectionalLight->GetLightArea();
        DirectionalLightData.ShadowMatrix  = DirectionalLight->GetShadowMatrix();
        DirectionalLightData.ShadowMatrix  = DirectionalLightData.ShadowMatrix.GetTranspose();
        DirectionalLightDataDirty = true;

        // Update HLSL data
        CascadeGenerationData.CascadeSplitLambda  = DirectionalLight->GetCascadeSplitLambda();
        CascadeGenerationData.LightPositionOffset = DirectionalLight->GetShadowPositionOffset();
        CascadeGenerationData.LightNearPlane      = DirectionalLight->GetShadowNearPlane();
        CascadeGenerationData.LightFarPlane       = DirectionalLight->GetShadowFarPlane();
        CascadeGenerationData.LightUp             = DirectionalLightData.UpVector;
        CascadeGenerationData.LightDirection      = DirectionalLightData.Direction;
        CascadeGenerationData.ShadowMatrix        = DirectionalLightData.ShadowMatrix;
        CascadeGenerationData.CascadeResolution   = static_cast<float>(CascadeSize);
        CascadeGenerationData.MaxCascadeIndex     = Math::Max(NUM_SHADOW_CASCADES - 1, 0);

        if (IConsoleVariable* CVarCSMTightFrustum = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum"))
        {
            CascadeGenerationData.bEnableTightFrustum = CVarCSMTightFrustum->GetBool();
        }
        else
        {
            CascadeGenerationData.bEnableTightFrustum = true;
        }

        if (IConsoleVariable* CVarCSMStableCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.StableCascades"))
        {
            CascadeGenerationData.bEnableStableCascades = CVarCSMStableCascades->GetBool();
        }
        else
        {
            CascadeGenerationData.bEnableStableCascades = true;
        }

        if (IConsoleVariable* CVarMaxPenumbraWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MaxPenumbraWorld"))
        {
            CascadeGenerationData.MaxPenumbraWorld = Math::Max<float>(CVarMaxPenumbraWorld->GetFloat(), 0.0f);
        }
        else
        {
            CascadeGenerationData.MaxPenumbraWorld = 0.0f;
        }

        if (IConsoleVariable* CVarMaxSearchDistanceWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCSS.MaxSearchDistanceWorld"))
        {
            CascadeGenerationData.MaxSearchDistanceWorld = Math::Max<float>(CVarMaxSearchDistanceWorld->GetFloat(), 0.0f);
        }
        else
        {
            CascadeGenerationData.MaxSearchDistanceWorld = 0.0f;
        }

        CascadeGenerationDataDirty = true;
    }

    // Update PointLights
    for (int32 Index = 0; Index < Scene->PointLights.Size(); Index++)
    {
        FPointLight* PointLight = Scene->PointLights[Index]->PointLight;

        // Pre-multiply light intensity (PointLight intensity is expressed in lumens -> convert to candela)
        FVector3 Color = PointLight->GetColor();
        const float LuminousIntensity = PointLight->GetIntensity() * (1.0f / (4.0f * Math::Constants::PI));
        Color = Color * LuminousIntensity;

        const float Radius = PointLight->GetShadowFarPlane();
        FVector3 Position = PointLight->GetPosition();
        FVector4 PositionAndRadius = FVector4(Position, Radius);

        if (PointLight->IsShadowCaster())
        {
            FShadowCastingPointLightDataHLSL Data;
            Data.Color      = Color;
            Data.FarPlane   = PointLight->GetShadowFarPlane();
            Data.ShadowBias = PointLight->GetShadowBias();
            Data.Padding0   = 0.0f;
            Data.Padding1   = 0.0f;
            Data.Padding2   = 0.0f;

            ShadowCastingPointLightsData.Emplace(Data);
            ShadowCastingPointLightsPosRad.Emplace(PositionAndRadius);
        }
        else
        {
            FPointLightDataHLSL Data;
            Data.Color = Color;

            PointLightsData.Emplace(Data);
            PointLightsPosRad.Emplace(PositionAndRadius);
        }
    }

    // Update LightProbes
    for (int32 Index = 0; Index < Scene->LightProbes.Size(); Index++)
    {
        FSceneLightProbe* LightProbe = Scene->LightProbes[Index];

        FLightProbeInfoHLSL Info;
        Info.BoxOriginWS   = LightProbe->Origin;
        Info.BoxProjection = LightProbe->bBoxProjection ? 1.0f : 0.0f;
        Info.BoxMinWS      = LightProbe->BoxMin;
        Info.Padding0      = 0.0f;
        Info.BoxMaxWS      = LightProbe->BoxMax;
        Info.Padding1      = 0.0f;

        LightProbeInfos.Emplace(Info);
    }

    // Update GPU Buffers
    if (PointLightsData.SizeInBytes() > static_cast<int32>(PointLightsBuffer->GetInfo().Size))
    {
        FRHIBufferInfo BufferInfo;
        BufferInfo.Stride = PointLightsData.CapacityInBytes();
        BufferInfo.Size   = PointLightsData.Stride();
        BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferInfo.bEnableResourceStateTracking = true;

        PointLightsBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > static_cast<int32>(PointLightsPosRadBuffer->GetInfo().Size))
    {
        FRHIBufferInfo BufferInfo;
        BufferInfo.Stride = PointLightsPosRad.CapacityInBytes();
        BufferInfo.Size   = PointLightsPosRad.Stride();
        BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferInfo.bEnableResourceStateTracking = true;

        PointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsBuffer->GetInfo().Size))
    {
        FRHIBufferInfo BufferInfo;
        BufferInfo.Stride = ShadowCastingPointLightsData.CapacityInBytes();
        BufferInfo.Size   = ShadowCastingPointLightsData.Stride();
        BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferInfo.bEnableResourceStateTracking = true;

        ShadowCastingPointLightsBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsPosRadBuffer->GetInfo().Size))
    {
        FRHIBufferInfo BufferInfo;
        BufferInfo.Stride = ShadowCastingPointLightsPosRad.CapacityInBytes();
        BufferInfo.Size   = ShadowCastingPointLightsPosRad.Stride();
        BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferInfo.bEnableResourceStateTracking = true;

        ShadowCastingPointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (LightProbeInfos.SizeInBytes() > static_cast<int32>(LightProbeBuffer->GetInfo().Size))
    {
        FRHIBufferInfo BufferInfo;
        BufferInfo.Stride = LightProbeInfos.CapacityInBytes();
        BufferInfo.Size   = LightProbeInfos.Stride();
        BufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferInfo.bEnableResourceStateTracking = true;

        LightProbeBuffer = FRHI::Get()->CreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!LightProbeBuffer)
        {
            DEBUG_BREAK();
        }
    }

    // Directional-Light
    CommandList.TransitionBuffer(DirectionalLightDataBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(CascadeGenerationDataBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);

    // Point-Lights
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);

    // Light-Probes
    CommandList.TransitionBuffer(LightProbeBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);

    // Directional-Light
    if (DirectionalLightDataDirty)
    {
        CommandList.UpdateBuffer(DirectionalLightDataBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalLightDataHLSL)), &DirectionalLightData);
        DirectionalLightDataDirty = false;
    }

    if (CascadeGenerationDataDirty)
    {
        CommandList.UpdateBuffer(CascadeGenerationDataBuffer.Get(), FBufferRegion(0, sizeof(FCascadeGenerationInfoHLSL)), &CascadeGenerationData);
        CascadeGenerationDataDirty = false;
    }

    // Point-Lights
    if (!PointLightsData.IsEmpty())
    {
        CommandList.UpdateBuffer(PointLightsBuffer.Get(), FBufferRegion(0, PointLightsData.SizeInBytes()), PointLightsData.Data());
        CommandList.UpdateBuffer(PointLightsPosRadBuffer.Get(), FBufferRegion(0, PointLightsPosRad.SizeInBytes()), PointLightsPosRad.Data());
    }

    if (!ShadowCastingPointLightsData.IsEmpty())
    {
        CommandList.UpdateBuffer(ShadowCastingPointLightsBuffer.Get(), FBufferRegion(0, ShadowCastingPointLightsData.SizeInBytes()), ShadowCastingPointLightsData.Data());
        CommandList.UpdateBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), FBufferRegion(0, ShadowCastingPointLightsPosRad.SizeInBytes()), ShadowCastingPointLightsPosRad.Data());
    }

    // Light-Probes
    if (!LightProbeInfos.IsEmpty())
    {
        CommandList.UpdateBuffer(LightProbeBuffer.Get(), FBufferRegion(0, LightProbeInfos.SizeInBytes()), LightProbeInfos.Data());
    }

    // Directional-Light
    CommandList.TransitionBuffer(DirectionalLightDataBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(CascadeGenerationDataBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    // Point-Lights
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    // Light-Probes
    CommandList.TransitionBuffer(LightProbeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Update Lights");
}

void FFrameResources::Release()
{
    CameraBuffer.Reset();
    TransformBuffer.Reset();

    PointLightShadowSampler.Reset();
    ShadowSamplerPoint.Reset();
    ShadowSamplerPointCmp.Reset();
    ShadowSamplerLinearCmp.Reset();
    LightProbeSampler.Reset();
    GBufferSampler.Reset();

    IntegrationLUT.Reset();
    IntegrationLUTSampler.Reset();

    SSAOBuffer.Reset();
    FinalTarget.Reset();
    TonemappedTarget.Reset();

    for (FRHITextureRef& Buffer : GBuffer)
    {
        Buffer.Reset();
    }

#if EDITOR_BUILD
    EditorNoJitterDepth.Reset();
    EditorObjectID_NoJitter.Reset();
#endif

    ReducedDepthBuffer[0].Reset();
    ReducedDepthBuffer[1].Reset();

    MeshInputLayout.Reset();

    RTScene.Reset();
    RTOutput.Reset();
    RTGeometryInstances.Clear();
    RTHitGroupResources.Clear();
    RTMeshToHitGroupIndex.Clear();

    DirectionalShadowMask.Reset();
    CascadeIndexBuffer.Reset();

    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();

    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();

    DirectionalLightDataBuffer.Reset();
    CascadeGenerationDataBuffer.Reset();

    PointLightShadowMaps.Reset();
    ShadowCascades.Reset();

    CascadeMatrixBuffer.Reset();
    CascadeMatrixBufferSRV.Reset();
    CascadeMatrixBufferUAV.Reset();

    CascadeSplitsBuffer.Reset();
    CascadeSplitsBufferSRV.Reset();
    CascadeSplitsBufferUAV.Reset();

    LightProbeBuffer.Reset();
}
