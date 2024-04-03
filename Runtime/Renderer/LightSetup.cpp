#include "LightSetup.h"
#include "RendererScene.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
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


bool FLightSetup::Initialize()
{
    // Initialize the light-setup from CVars
    CascadeSize                 = ClampTextureSize(512, 4096, CVarCSMCascadeSize.GetValue());
    PointLightShadowSize        = ClampTextureSize(128, 1024, CVarPointLightShadowMapSize.GetValue());
    IrradianceProbeSize         = ClampTextureSize(32, 512, CVarEnvironmentIrradianceProbeSize.GetValue());
    SpecularIrradianceProbeSize = ClampTextureSize(256, 1024, CVarEnvironmentSpecularIrradianceProbeSize.GetValue());

    {
        FRHIBufferDesc BufferDesc(sizeof(DirectionalLightData), sizeof(DirectionalLightData), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        DirectionalLightsBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!DirectionalLightsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalLightsBuffer->SetDebugName("DirectionalLightsBuffer");
        }
    }


    {
        PointLightsData.Reserve(MAX_LIGHTS_PER_TILE);
        
        FRHIBufferDesc BufferDesc(PointLightsData.CapacityInBytes(), PointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PointLightsBuffer->SetDebugName("PointLightsBuffer");
        }
    }

    {
        PointLightsPosRad.Reserve(MAX_LIGHTS_PER_TILE);

        FRHIBufferDesc BufferDesc(PointLightsPosRad.CapacityInBytes(), PointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsPosRadBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PointLightsPosRadBuffer->SetDebugName("PointLightsPosRadBuffer");
        }
    }

    {
        ShadowCastingPointLightsData.Reserve(8);

        FRHIBufferDesc BufferDesc(ShadowCastingPointLightsData.CapacityInBytes(), ShadowCastingPointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ShadowCastingPointLightsBuffer->SetDebugName("ShadowCastingPointLightsBuffer");
        }
    }

    {
        ShadowCastingPointLightsPosRad.Reserve(8);

        FRHIBufferDesc BufferDesc(ShadowCastingPointLightsPosRad.CapacityInBytes(), ShadowCastingPointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsPosRadBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
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

void FLightSetup::BeginFrame(FRHICommandList& CommandList, FRendererScene* Scene)
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    PointLightShadowMapsGenerationData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    for (int32 Index = 0; Index < Scene->Lights.Size(); Index++)
    {
        FLight* Light = Scene->Lights[Index];

        const float Intensity = Light->GetIntensity();
        FVector3 Color = Light->GetColor();
        Color = Color * Intensity;

        if (FPointLight* PointLight = Cast<FPointLight>(Light))
        {
            constexpr float MinLuma = 0.005f;
            const float Dot    = Color.x * 0.2126f + Color.y * 0.7152f + Color.z * 0.0722f;
            const float Radius = FMath::Sqrt(Dot / MinLuma);

            FVector3 Position = PointLight->GetPosition();
            FVector4 PosRad   = FVector4(Position, Radius);
            if (PointLight->IsShadowCaster())
            {
                FShadowCastingPointLightData Data;
                Data.Color         = Color;
                Data.FarPlane      = PointLight->GetShadowFarPlane();
                Data.MaxShadowBias = PointLight->GetMaxShadowBias();
                Data.ShadowBias    = PointLight->GetShadowBias();

                ShadowCastingPointLightsData.Emplace(Data);
                ShadowCastingPointLightsPosRad.Emplace(PosRad);

                FPointLightShadowMapGenerationData ShadowMapData;
                ShadowMapData.LightIndex = Index;
                ShadowMapData.FarPlane   = PointLight->GetShadowFarPlane();
                ShadowMapData.Position   = PointLight->GetPosition();

                for (uint32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
                {
                    ShadowMapData.Matrix[FaceIndex]     = PointLight->GetMatrix(FaceIndex);
                    ShadowMapData.ViewMatrix[FaceIndex] = PointLight->GetViewMatrix(FaceIndex);
                    ShadowMapData.ProjMatrix[FaceIndex] = PointLight->GetProjectionMatrix(FaceIndex);
                }

                PointLightShadowMapsGenerationData.Emplace(ShadowMapData);
            }
            else
            {
                FPointLightData Data;
                Data.Color = Color;

                PointLightsData.Emplace(Data);
                PointLightsPosRad.Emplace(PosRad);
            }
        }
        else if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Light))
        {
            DirectionalLight->Tick(*Scene->Camera);

            CascadeSplitLambda                 = DirectionalLight->GetCascadeSplitLambda();
            DirectionalLightData.Color         = FVector3(Color.x, Color.y, Color.z);
            DirectionalLightData.ShadowBias    = DirectionalLight->GetShadowBias();
            DirectionalLightData.Direction     = DirectionalLight->GetDirection();
            // TODO: Is this used?
            DirectionalLightData.UpVector      = DirectionalLight->GetUp();
            DirectionalLightData.MaxShadowBias = DirectionalLight->GetMaxShadowBias();
            DirectionalLightData.LightSize     = DirectionalLight->GetSize();
            DirectionalLightData.ShadowMatrix  = DirectionalLight->GetShadowMatrix();
            DirectionalLightFarPlane           = DirectionalLight->GetShadowFarPlane();
            DirectionalLightViewMatrix         = DirectionalLight->GetViewMatrix();
            DirectionalLightProjMatrix         = DirectionalLight->GetProjectionMatrix();

            DirectionalLightDataDirty = true;
        }
    }

    if (PointLightsData.SizeInBytes() > static_cast<int32>(PointLightsBuffer->GetSize()))
    {
        CommandList.DestroyResource(PointLightsBuffer.Get());

        FRHIBufferDesc BufferDesc(PointLightsData.CapacityInBytes(), PointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > static_cast<int32>(PointLightsPosRadBuffer->GetSize()))
    {
        CommandList.DestroyResource(PointLightsPosRadBuffer.Get());

        FRHIBufferDesc BufferDesc(PointLightsPosRad.CapacityInBytes(), PointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        PointLightsPosRadBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsBuffer->GetSize()))
    {
        CommandList.DestroyResource(ShadowCastingPointLightsBuffer.Get());

        FRHIBufferDesc BufferDesc(ShadowCastingPointLightsData.CapacityInBytes(), ShadowCastingPointLightsData.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > static_cast<int32>(ShadowCastingPointLightsPosRadBuffer->GetSize()))
    {
        CommandList.DestroyResource(ShadowCastingPointLightsPosRadBuffer.Get());

        FRHIBufferDesc BufferDesc(ShadowCastingPointLightsPosRad.CapacityInBytes(), ShadowCastingPointLightsPosRad.Stride(), EBufferUsageFlags::ConstantBuffer | EBufferUsageFlags::Default);
        ShadowCastingPointLightsPosRadBuffer = RHICreateBuffer(BufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    CommandList.TransitionBuffer(DirectionalLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);

    if (DirectionalLightDataDirty)
    {
        CommandList.UpdateBuffer(DirectionalLightsBuffer.Get(), FBufferRegion(0, sizeof(DirectionalLightData)), &DirectionalLightData);
        DirectionalLightDataDirty = false;
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

    CommandList.TransitionBuffer(DirectionalLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(PointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    CommandList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Update Lights");
}

void FLightSetup::Release()
{
    DirectionalShadowMask.Reset();
    CascadeIndexBuffer.Reset();

    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();

    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();

    DirectionalLightsBuffer.Reset();

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
