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
    1024);

static TAutoConsoleVariable<float> CVarCSMTightFrustumShrink(
    "Renderer.CSM.TightFrustum.ShrinkLerp",
    "How quickly the tight-frustum depth range shrinks toward the current min/max (0 = never shrink, 1 = immediate). Expansion is immediate.",
    0.2f);

static TAutoConsoleVariable<bool> CVarCSMTightFrustumStableExtents(
    "Renderer.CSM.TightFrustum.StableExtents",
    "When enabled, tight-frustum cascades keep stable XY extents based on the reference frustum to prevent temporal shimmering during camera movement.",
    true);

static TAutoConsoleVariable<float> CVarCSMTightFrustumDepthQuant(
    "Renderer.CSM.TightFrustum.DepthQuant",
    "Quantization steps used for tight-frustum min/max depth snapping (higher = finer, lower = more stable).",
    1024.0f);

static TAutoConsoleVariable<bool> CVarCSMUseDepthReducedRange(
    "Renderer.CSM.UseDepthReducedRange",
    "Use reduced scene depth min/max to tighten the split-generation near/far range in Auto (Lambda) mode.",
    false);

static TAutoConsoleVariable<float> CVarCSMNearDistance(
    "Renderer.CSM.NearDistance",
    "Near distance (view-space) used for CSM split generation. 0 uses the camera near plane.",
    0.0f);

static TAutoConsoleVariable<float> CVarCSMPCSSMaxRadiusTexels(
    "Renderer.CSM.PCSS.MaxRadiusTexels",
    "Maximum PCSS kernel radius in texels used for cascade guard bands.",
    192.0f);

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
        CascadeSize       = NewCascadeSize;
        bCascadeSizeDirty = true;
        return true;
    }

    return false;
}

FFrameResources::FFrameResources()
    : bDirectionalLightDataDirty(true)
    , CascadeSplitLambda(0.0f)
    , bCascadeGenerationDataDirty(true)
    , bCascadeSizeDirty(false)
{
    DirectionalLightData  = {};
    CascadeGenerationData = {};
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
    FRHIBufferDesc BufferDesc;
    BufferDesc.Stride = sizeof(FDirectionalLightDataHLSL);
    BufferDesc.Size   = sizeof(FDirectionalLightDataHLSL);
    BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
    BufferDesc.bEnableResourceStateTracking = true;

    DirectionalLightDataBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);

    if (!DirectionalLightDataBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        DirectionalLightDataBuffer->SetDebugName("DirectionalLightData Buffer");
    }

    BufferDesc.Stride = sizeof(FCascadeGenerationInfoHLSL);
    BufferDesc.Size   = sizeof(FCascadeGenerationInfoHLSL);

    CascadeGenerationDataBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);

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

    BufferDesc.Stride = PointLightsData.Stride();
    BufferDesc.Size   = PointLightsData.CapacityInBytes();
    BufferDesc.bEnableResourceStateTracking = true;

    PointLightsBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);

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

    BufferDesc.Stride = PointLightsPosRad.Stride();
    BufferDesc.Size   = PointLightsPosRad.CapacityInBytes();
    BufferDesc.bEnableResourceStateTracking = true;

    PointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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

    BufferDesc.Stride = ShadowCastingPointLightsData.Stride();
    BufferDesc.Size   = ShadowCastingPointLightsData.CapacityInBytes();
    BufferDesc.bEnableResourceStateTracking = true;

    ShadowCastingPointLightsBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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

    BufferDesc.Stride = ShadowCastingPointLightsPosRad.Stride();
    BufferDesc.Size   = ShadowCastingPointLightsPosRad.CapacityInBytes();
    BufferDesc.bEnableResourceStateTracking = true;

    ShadowCastingPointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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

    BufferDesc.Stride = LightProbeInfos.Stride();
    BufferDesc.Size   = LightProbeInfos.CapacityInBytes();
    BufferDesc.bEnableResourceStateTracking = true;

    LightProbeBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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
        bDirectionalLightDataDirty         = true;

        // Update HLSL data
        CascadeGenerationData.CascadeSplitLambda  = DirectionalLight->GetCascadeSplitLambda();
        CascadeGenerationData.LightPositionOffset = DirectionalLight->GetShadowPositionOffset();
        CascadeGenerationData.LightNearPlane      = 0.0f;
        CascadeGenerationData.LightFarPlane       = 0.0f;
        CascadeGenerationData.LightUp             = DirectionalLightData.UpVector;
        CascadeGenerationData.LightDirection      = DirectionalLightData.Direction;
        CascadeGenerationData.ShadowMatrix        = DirectionalLightData.ShadowMatrix;
        CascadeGenerationData.CascadeResolution   = static_cast<float>(CascadeSize);
        CascadeGenerationData.MaxCascadeIndex     = Math::Max(NUM_SHADOW_CASCADES - 1, 0);

        // Force sphere-fit path without tight-frustum depth fitting.
        CascadeGenerationData.bEnableTightFrustum = false;

        if (IConsoleVariable* CVarCSMStableCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.StableCascades"))
        {
            CascadeGenerationData.bEnableStableCascades = CVarCSMStableCascades->GetBool();
        }
        else
        {
            CascadeGenerationData.bEnableStableCascades = true;
        }

        const float PrevUseDepthReducedRange = CascadeGenerationData.UseDepthReducedRange;
        const float PrevCSMNearDistance      = CascadeGenerationData.CSMNearDistance;
        const int32 PrevCascadeSplitMode     = CascadeGenerationData.CascadeSplitMode;
        const float PrevManualSplit0         = CascadeGenerationData.ManualCascadeSplitDistance0;
        const float PrevManualSplit1         = CascadeGenerationData.ManualCascadeSplitDistance1;
        const float PrevManualSplit2         = CascadeGenerationData.ManualCascadeSplitDistance2;

        CascadeGenerationData.UseDepthReducedRange = CVarCSMUseDepthReducedRange.GetValue() ? 1.0f : 0.0f;
        CascadeGenerationData.CSMNearDistance      = Math::Max(CVarCSMNearDistance.GetValue(), 0.0f);
        CascadeGenerationData.CascadeSplitMode     = static_cast<int32>(DirectionalLight->GetCascadeSplitMode());
        CascadeGenerationData.ManualCascadeSplitDistance0 = Math::Max(DirectionalLight->GetManualCascadeSplitDistance(0), 0.0f);
        CascadeGenerationData.ManualCascadeSplitDistance1 = Math::Max(DirectionalLight->GetManualCascadeSplitDistance(1), 0.0f);
        CascadeGenerationData.ManualCascadeSplitDistance2 = Math::Max(DirectionalLight->GetManualCascadeSplitDistance(2), 0.0f);

        if (IConsoleVariable* CVarMaxShadowDistance = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.MaxShadowDistance"))
        {
            CascadeGenerationData.MaxShadowDistance = Math::Max<float>(CVarMaxShadowDistance->GetFloat(), 0.0f);
        }
        else
        {
            CascadeGenerationData.MaxShadowDistance = 0.0f;
        }

        if (IConsoleVariable* CVarShadowPancaking = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.ShadowPancaking"))
        {
            CascadeGenerationData.ShadowPancakingEnabled = CVarShadowPancaking->GetBool() ? 1.0f : 0.0f;
        }
        else
        {
            CascadeGenerationData.ShadowPancakingEnabled = 0.0f;
        }

        CascadeGenerationData.PCSSMaxRadiusTexels = Math::Clamp(CVarCSMPCSSMaxRadiusTexels.GetValue(), 4.0f, 512.0f);
        CascadeGenerationData.Padding0 = 0.0f;
        CascadeGenerationData.Padding1 = 0.0f;
        CascadeGenerationData.Padding2 = 0.0f;
        CascadeGenerationData.Padding3 = 0.0f;
        CascadeGenerationData.Padding4 = 0.0f;
        CascadeGenerationData.Padding5 = 0.0f;
        CascadeGenerationData.Padding6 = 0.0f;
        CascadeGenerationData.Padding7 = 0.0f;
        CascadeGenerationData.Padding8 = 0.0f;
        CascadeGenerationData.Padding9 = 0.0f;
        CascadeGenerationData.Padding10 = 0.0f;
        CascadeGenerationData.Padding11 = 0.0f;

        const bool bUseDepthReducedRangeChanged = Math::Abs(CascadeGenerationData.UseDepthReducedRange - PrevUseDepthReducedRange) > 1e-4f;
        const bool bCSMNearDistanceChanged      = Math::Abs(CascadeGenerationData.CSMNearDistance - PrevCSMNearDistance) > 1e-4f;
        const bool bSplitModeChanged            = CascadeGenerationData.CascadeSplitMode != PrevCascadeSplitMode;
        const bool bManualSplitsChanged =
            Math::Abs(CascadeGenerationData.ManualCascadeSplitDistance0 - PrevManualSplit0) > 1e-4f ||
            Math::Abs(CascadeGenerationData.ManualCascadeSplitDistance1 - PrevManualSplit1) > 1e-4f ||
            Math::Abs(CascadeGenerationData.ManualCascadeSplitDistance2 - PrevManualSplit2) > 1e-4f;

        if (bUseDepthReducedRangeChanged || bCSMNearDistanceChanged)
        {
            bCSMMinMaxHistoryInitialized = false;
            bCSMCascadeHistoryInitialized = false;
        }

        if (bSplitModeChanged || bManualSplitsChanged)
        {
            bCSMCascadeHistoryInitialized = false;
        }

        if (CascadeGenerationData.UseDepthReducedRange <= 0.5f)
        {
            bCSMMinMaxHistoryInitialized = false;
        }

        if (!CascadeGenerationData.bEnableStableCascades)
        {
            bCSMCascadeHistoryInitialized = false;
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

        if (IConsoleVariable* CVarTightFrustumShrink = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum.ShrinkLerp"))
        {
            CascadeGenerationData.TightFrustumShrinkFactor = Math::Clamp<float>(CVarTightFrustumShrink->GetFloat(), 0.0f, 1.0f);
        }
        else
        {
            CascadeGenerationData.TightFrustumShrinkFactor = 1.0f;
        }

        if (IConsoleVariable* CVarStableExtents = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum.StableExtents"))
        {
            CascadeGenerationData.TightFrustumStableExtents = CVarStableExtents->GetBool() ? 1.0f : 0.0f;
        }
        else
        {
            CascadeGenerationData.TightFrustumStableExtents = 1.0f;
        }

        if (IConsoleVariable* CVarDepthQuant = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.TightFrustum.DepthQuant"))
        {
            CascadeGenerationData.TightFrustumDepthQuant = Math::Clamp<float>(CVarDepthQuant->GetFloat(), 1.0f, 8192.0f);
        }
        else
        {
            CascadeGenerationData.TightFrustumDepthQuant = 1024.0f;
        }
        CascadeGenerationData.TightFrustumPadding = 0.0f;

        if (IConsoleVariable* CVarFilterMode = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.FilterMode"))
        {
            CascadeGenerationData.FilterMode = Math::Clamp<int32>(CVarFilterMode->GetInt(), 0, 1);
        }
        else
        {
            CascadeGenerationData.FilterMode = 0;
        }

        if (IConsoleVariable* CVarPCFFilterWorld = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCF.FilterWorld"))
        {
            CascadeGenerationData.PCFFilterWorld = Math::Max<float>(CVarPCFFilterWorld->GetFloat(), 0.0f);
        }
        else
        {
            CascadeGenerationData.PCFFilterWorld = 0.0f;
        }

        if (IConsoleVariable* CVarPCFMinFilterRadius = FConsoleManager::Get().FindConsoleVariable("Renderer.CSM.PCF.MinFilterRadiusTexels"))
        {
            CascadeGenerationData.PCFMinFilterRadiusTexels = Math::Max<float>(CVarPCFMinFilterRadius->GetFloat(), 0.0f);
        }
        else
        {
            CascadeGenerationData.PCFMinFilterRadiusTexels = 0.0f;
        }

        bCascadeGenerationDataDirty = true;
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

        FVector3 Position          = PointLight->GetPosition();
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
    if (PointLightsData.SizeInBytes() > static_cast<int32>(PointLightsBuffer->GetDesc().Size))
    {
        FRHIBufferDesc BufferDesc;
        BufferDesc.Stride = PointLightsData.CapacityInBytes();
        BufferDesc.Size   = PointLightsData.Stride();
        BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferDesc.bEnableResourceStateTracking = true;

        PointLightsBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > static_cast<int32>(PointLightsPosRadBuffer->GetDesc().Size))
    {
        FRHIBufferDesc BufferDesc;
        BufferDesc.Stride = PointLightsPosRad.CapacityInBytes();
        BufferDesc.Size   = PointLightsPosRad.Stride();
        BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferDesc.bEnableResourceStateTracking = true;

        PointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsBuffer->GetDesc().Size))
    {
        FRHIBufferDesc BufferDesc;
        BufferDesc.Stride = ShadowCastingPointLightsData.CapacityInBytes();
        BufferDesc.Size   = ShadowCastingPointLightsData.Stride();
        BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferDesc.bEnableResourceStateTracking = true;

        ShadowCastingPointLightsBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsPosRadBuffer->GetDesc().Size))
    {
        FRHIBufferDesc BufferDesc;
        BufferDesc.Stride = ShadowCastingPointLightsPosRad.CapacityInBytes();
        BufferDesc.Size   = ShadowCastingPointLightsPosRad.Stride();
        BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferDesc.bEnableResourceStateTracking = true;

        ShadowCastingPointLightsPosRadBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (LightProbeInfos.SizeInBytes() > static_cast<int32>(LightProbeBuffer->GetDesc().Size))
    {
        FRHIBufferDesc BufferDesc;
        BufferDesc.Stride = LightProbeInfos.CapacityInBytes();
        BufferDesc.Size   = LightProbeInfos.Stride();
        BufferDesc.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;
        BufferDesc.bEnableResourceStateTracking = true;

        LightProbeBuffer = FRHI::Get()->CreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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
    if (bDirectionalLightDataDirty)
    {
        CommandList.UpdateBuffer(DirectionalLightDataBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalLightDataHLSL)), &DirectionalLightData);
        bDirectionalLightDataDirty = false;
    }

    if (bCascadeGenerationDataDirty)
    {
        CommandList.UpdateBuffer(CascadeGenerationDataBuffer.Get(), FBufferRegion(0, sizeof(FCascadeGenerationInfoHLSL)), &CascadeGenerationData);
        bCascadeGenerationDataDirty = false;
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

    bCSMMinMaxHistoryInitialized  = false;
    bCSMCascadeHistoryInitialized = false;

    CSMMinMaxDepthHistory.Reset();
    CascadeSnapHistoryBuffer.Reset();
    CascadeSnapHistoryBufferUAV.Reset();
    CascadeSplitHistoryBuffer.Reset();
    CascadeSplitHistoryBufferUAV.Reset();

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
    ShadowMaskRaw.Reset();
    ShadowDebugBuffer.Reset();
    ShadowMaskHistory[0].Reset();
    ShadowMaskHistory[1].Reset();
    ShadowMaskMomentsHistory[0].Reset();
    ShadowMaskMomentsHistory[1].Reset();
    CascadeIndexBuffer.Reset();

    bShadowMaskHistoryInitialized = false;
    bShadowMaskMomentsHistoryInitialized = false;
    ShadowMaskHistoryLatestIndex = 0;

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
