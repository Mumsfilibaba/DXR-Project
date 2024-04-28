#include "FrameResources.h"
#include "Scene.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Core/Misc/ConsoleManager.h"

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

// TODO: Move to FMath
static int32 NextPower2(int32 Value)
{
    if ((Value & (Value - 1)) == 0)
    {
        return Value;
    }

    // Find the next power of 2
    int32 RoundedValue = 1;
    while (RoundedValue < Value)
    {
        RoundedValue <<= 1;
    }

    return RoundedValue;
}

static int32 ClosestPowerOf2(int32 Value)
{
    if (Value <= 0)
    {
        return 0;
    }

    const int32 NextPow = NextPower2(Value);
    const int32 PrevPow = NextPow >> 1;

    if (Value - PrevPow <= NextPow - Value)
    {
        return PrevPow;
    }
    else
    {
        return NextPow;
    }
}

static int32 ClampTextureSize(int32 MinSize, int32 MaxSize, int32 NewSize)
{
    const int32 Result = FMath::Clamp(MinSize, MaxSize, NewSize);
    return ClosestPowerOf2(Result);
}

bool FFrameResources::Initialize()
{
    // Initialize the light-setup from CVars
    CascadeSize                 = ClampTextureSize(512, 4096, CVarCSMCascadeSize.GetValue());
    PointLightShadowSize        = ClampTextureSize(128, 1024, CVarPointLightShadowMapSize.GetValue());
    IrradianceProbeSize         = ClampTextureSize(32, 512, CVarEnvironmentIrradianceProbeSize.GetValue());
    SpecularIrradianceProbeSize = ClampTextureSize(256, 1024, CVarEnvironmentSpecularIrradianceProbeSize.GetValue());

    {
        FRHIBufferInfo BufferInfo(sizeof(FDirectionalLightDataHLSL), sizeof(FDirectionalLightDataHLSL), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        DirectionalLightDataBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!DirectionalLightDataBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalLightDataBuffer->SetDebugName("DirectionalLightData Buffer");
        }
    }

    {
        FRHIBufferInfo BufferInfo(sizeof(FCascadeGenerationInfoHLSL), sizeof(FCascadeGenerationInfoHLSL), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        CascadeGenerationDataBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!CascadeGenerationDataBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            CascadeGenerationDataBuffer->SetDebugName("CascadeGenerationData Buffer");
        }
    }

    {
        PointLightsData.Reserve(MAX_LIGHTS_PER_TILE);

        FRHIBufferInfo BufferInfo(PointLightsData.CapacityInBytes(), PointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PointLightsBuffer->SetDebugName("PointLights Buffer");
        }
    }

    {
        PointLightsPosRad.Reserve(MAX_LIGHTS_PER_TILE);

        FRHIBufferInfo BufferInfo(PointLightsPosRad.CapacityInBytes(), PointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsPosRadBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PointLightsPosRadBuffer->SetDebugName("PointLights Position and Radius Buffer");
        }
    }

    {
        ShadowCastingPointLightsData.Reserve(NUM_DEFAULT_SHADOW_CASTING_POINT_LIGHTS);

        FRHIBufferInfo BufferInfo(ShadowCastingPointLightsData.CapacityInBytes(), ShadowCastingPointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ShadowCastingPointLightsBuffer->SetDebugName("ShadowCasting PointLights Buffer");
        }
    }

    {
        ShadowCastingPointLightsPosRad.Reserve(NUM_DEFAULT_SHADOW_CASTING_POINT_LIGHTS);

        FRHIBufferInfo BufferInfo(ShadowCastingPointLightsPosRad.CapacityInBytes(), ShadowCastingPointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsPosRadBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ShadowCastingPointLightsPosRadBuffer->SetDebugName("ShadowCastingPointLightsPosRadBuffer");
        }
    }

    return true;
}

void FFrameResources::BuildLightBuffers(FRHICommandList& CommandList, FScene* Scene)
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    // Update DirectionalLight
    if (Scene->DirectionalLight)
    {
        FDirectionalLight* DirectionalLight = Scene->DirectionalLight->Light;

        // Pre-multiply light intensity TODO: Just specify the light color directly FVector4(100.0f, 1.0f, 58.0f, 6.0f)
        FVector3 Color = DirectionalLight->GetColor();
        Color = Color * DirectionalLight->GetIntensity();

        DirectionalLight->Tick(*Scene->Camera);

        DirectionalLightData.Color         = Color;
        DirectionalLightData.ShadowBias    = DirectionalLight->GetShadowBias();
        DirectionalLightData.Direction     = DirectionalLight->GetDirection();
        DirectionalLightData.UpVector      = DirectionalLight->GetUp();
        DirectionalLightData.MaxShadowBias = DirectionalLight->GetMaxShadowBias();
        DirectionalLightData.LightSize     = DirectionalLight->GetSize();
        DirectionalLightData.ShadowMatrix  = DirectionalLight->GetShadowMatrix();
        DirectionalLightDataDirty = true;

        CascadeGenerationData.CascadeSplitLambda = DirectionalLight->GetCascadeSplitLambda();
        CascadeGenerationData.LightUp            = DirectionalLightData.UpVector;
        CascadeGenerationData.LightDirection     = DirectionalLightData.Direction;
        CascadeGenerationData.CascadeResolution  = static_cast<float>(CascadeSize);
        CascadeGenerationData.ShadowMatrix       = DirectionalLightData.ShadowMatrix;
        CascadeGenerationData.MaxCascadeIndex    = FMath::Max(NUM_SHADOW_CASCADES - 1, 0);

        if (IConsoleVariable* CVarPrePassDepthReduce = FConsoleManager::Get().FindConsoleVariable("Renderer.PrePass.DepthReduce"))
        {
            CascadeGenerationData.bDepthReductionEnabled = CVarPrePassDepthReduce->GetBool();
        }
        else
        {
            CascadeGenerationData.bDepthReductionEnabled = true;
        }

        CascadeGenerationDataDirty = true;
    }

    // Update PointLights
    for (int32 Index = 0; Index < Scene->PointLights.Size(); Index++)
    {
        FPointLight* PointLight = Scene->PointLights[Index]->Light;

        // Pre-multiply light intensity TODO: Just specify the light color directly FVector4(100.0f, 1.0f, 58.0f, 6.0f)
        FVector3 Color = PointLight->GetColor();
        Color = Color * PointLight->GetIntensity();

        const float Radius         = PointLight->GetShadowFarPlane();
        FVector3 Position          = PointLight->GetPosition();
        FVector4 PositionAndRadius = FVector4(Position, Radius);

        if (PointLight->IsShadowCaster())
        {
            FShadowCastingPointLightDataHLSL Data;
            Data.Color         = Color;
            Data.FarPlane      = PointLight->GetShadowFarPlane();
            Data.MaxShadowBias = PointLight->GetMaxShadowBias();
            Data.ShadowBias    = PointLight->GetShadowBias();

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

    // Update GPU Buffers
    if (PointLightsData.SizeInBytes() > static_cast<int32>(PointLightsBuffer->GetSize()))
    {
        FRHIBufferInfo BufferInfo(PointLightsData.CapacityInBytes(), PointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > static_cast<int32>(PointLightsPosRadBuffer->GetSize()))
    {
        FRHIBufferInfo BufferInfo(PointLightsPosRad.CapacityInBytes(), PointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsPosRadBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsBuffer->GetSize()))
    {
        FRHIBufferInfo BufferInfo(ShadowCastingPointLightsData.CapacityInBytes(), ShadowCastingPointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsPosRadBuffer->GetSize()))
    {
        FRHIBufferInfo BufferInfo(ShadowCastingPointLightsPosRad.CapacityInBytes(), ShadowCastingPointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsPosRadBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    CommandList.TransitionBuffer(DirectionalLightDataBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(CascadeGenerationDataBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);

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

    CommandList.TransitionBuffer(DirectionalLightDataBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(CascadeGenerationDataBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Update Lights");
}

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

    DirectionalShadowMask.Reset();
    CascadeIndexBuffer.Reset();

    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();

    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();

    DirectionalLightDataBuffer.Reset();
    CascadeGenerationDataBuffer.Reset();

    PointLightShadowMaps.Reset();

    for (FRHITextureRef& ShadowMap : ShadowMapCascades)
    {
        ShadowMap.Reset();
    }

    Skylight.Release();
    LocalProbe.Release();

    CascadeMatrixBuffer.Reset();
    CascadeMatrixBufferSRV.Reset();
    CascadeMatrixBufferUAV.Reset();

    CascadeSplitsBuffer.Reset();
    CascadeSplitsBufferSRV.Reset();
    CascadeSplitsBufferUAV.Reset();
}
