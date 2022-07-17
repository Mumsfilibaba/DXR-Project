#include "LightSetup.h"

#include "RHI/RHICoreInterface.h"

#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightSetup

bool FLightSetup::Init()
{
    FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, sizeof(DirectionalLightData));

    DirectionalLightsBuffer = RHICreateConstantBuffer(Initializer);
    if (!DirectionalLightsBuffer)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        DirectionalLightsBuffer->SetName("DirectionalLightsBuffer");
    }

    PointLightsData.Reserve(256);
    Initializer = FRHIConstantBufferInitializer(EBufferUsageFlags::Default, PointLightsData.CapacityInBytes());

    PointLightsBuffer = RHICreateConstantBuffer(Initializer);
    if (!PointLightsBuffer)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        PointLightsBuffer->SetName("PointLightsBuffer");
    }

    PointLightsPosRad.Reserve(256);
    Initializer = FRHIConstantBufferInitializer(EBufferUsageFlags::Default, PointLightsPosRad.CapacityInBytes());

    PointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
    if (!PointLightsPosRadBuffer)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        PointLightsPosRadBuffer->SetName("PointLightsPosRadBuffer");
    }

    ShadowCastingPointLightsData.Reserve(8);
    Initializer = FRHIConstantBufferInitializer(EBufferUsageFlags::Default, ShadowCastingPointLightsData.CapacityInBytes());

    ShadowCastingPointLightsBuffer = RHICreateConstantBuffer(Initializer);
    if (!ShadowCastingPointLightsBuffer)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsBuffer->SetName("ShadowCastingPointLightsBuffer");
    }

    ShadowCastingPointLightsPosRad.Reserve(8);
    Initializer = FRHIConstantBufferInitializer(EBufferUsageFlags::Default, ShadowCastingPointLightsPosRad.CapacityInBytes());

    ShadowCastingPointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
    if (!ShadowCastingPointLightsPosRadBuffer)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsPosRadBuffer->SetName("ShadowCastingPointLightsPosRadBuffer");
    }

    return true;
}

void FLightSetup::BeginFrame(FRHICommandList& CmdList, const FScene& Scene)
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    PointLightShadowMapsGenerationData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    FCamera* Camera = Scene.GetCamera();
    Check(Camera != nullptr);

    for (FLight* Light : Scene.GetLights())
    {
        float Intensity = Light->GetIntensity();
        FVector3 Color = Light->GetColor();
        Color = Color * Intensity;

        if (IsSubClassOf<FPointLight>(Light))
        {
            FPointLight* CurrentLight = Cast<FPointLight>(Light);
            Check(CurrentLight != nullptr);

            constexpr float MinLuma = 0.005f;
            float Dot = Color.x * 0.2126f + Color.y * 0.7152f + Color.z * 0.0722f;
            float Radius = sqrt(Dot / MinLuma);

            FVector3 Position = CurrentLight->GetPosition();
            FVector4 PosRad   = FVector4(Position, Radius);
            if (CurrentLight->IsShadowCaster())
            {
                FShadowCastingPointLightData Data;
                Data.Color         = Color;
                Data.FarPlane      = CurrentLight->GetShadowFarPlane();
                Data.MaxShadowBias = CurrentLight->GetMaxShadowBias();
                Data.ShadowBias    = CurrentLight->GetShadowBias();

                ShadowCastingPointLightsData.Emplace(Data);
                ShadowCastingPointLightsPosRad.Emplace(PosRad);

                FPointLightShadowMapGenerationData ShadowMapData;
                ShadowMapData.FarPlane = CurrentLight->GetShadowFarPlane();
                ShadowMapData.Position = CurrentLight->GetPosition();

                for (uint32 Face = 0; Face < 6; Face++)
                {
                    ShadowMapData.Matrix[Face]     = CurrentLight->GetMatrix(Face);
                    ShadowMapData.ViewMatrix[Face] = CurrentLight->GetViewMatrix(Face);
                    ShadowMapData.ProjMatrix[Face] = CurrentLight->GetProjectionMatrix(Face);
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
        else if (IsSubClassOf<FDirectionalLight>(Light))
        {
            FDirectionalLight* CurrentLight = Cast<FDirectionalLight>(Light);
            Check(CurrentLight != nullptr);

            CurrentLight->UpdateCascades(*Camera);

            DirectionalLightData.Color         = FVector3(Color.x, Color.y, Color.z);
            DirectionalLightData.ShadowBias    = CurrentLight->GetShadowBias();
            DirectionalLightData.Direction     = CurrentLight->GetDirection();
            DirectionalLightData.Up            = CurrentLight->GetUp();
            DirectionalLightData.MaxShadowBias = CurrentLight->GetMaxShadowBias();
            //DirectionalLightData.Position      = CurrentLight->GetPosition();

            //for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
            //{
            //    DirectionalLightData.CascadeRadius[i]   = CurrentLight->GetCascadeRadius(i);
            //}

            //DirectionalLightData.NearPlane = CurrentLight->GetShadowNearPlane();
            //DirectionalLightData.FarPlane  = CurrentLight->GetShadowFarPlane();
            DirectionalLightData.LightSize = CurrentLight->GetSize();

            CascadeSplitLambda = CurrentLight->GetCascadeSplitLambda();

            DirectionalLightDataDirty = true;
        }
    }

    if (PointLightsData.SizeInBytes() > (int32)PointLightsBuffer->GetSize())
    {
        CmdList.DestroyResource(PointLightsBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, PointLightsData.CapacityInBytes());
        PointLightsBuffer = RHICreateConstantBuffer(Initializer);
        if (!PointLightsBuffer)
        {
            PlatformDebugBreak();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > (int32)PointLightsPosRadBuffer->GetSize())
    {
        CmdList.DestroyResource(PointLightsPosRadBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, PointLightsPosRad.CapacityInBytes());
        PointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
        if (!PointLightsPosRadBuffer)
        {
            PlatformDebugBreak();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > (int32)ShadowCastingPointLightsBuffer->GetSize())
    {
        CmdList.DestroyResource(ShadowCastingPointLightsBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, ShadowCastingPointLightsData.CapacityInBytes());
        ShadowCastingPointLightsBuffer = RHICreateConstantBuffer(Initializer);
        if (!ShadowCastingPointLightsBuffer)
        {
            PlatformDebugBreak();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > (int32)ShadowCastingPointLightsPosRadBuffer->GetSize())
    {
        CmdList.DestroyResource(ShadowCastingPointLightsPosRadBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, ShadowCastingPointLightsPosRad.CapacityInBytes());
        ShadowCastingPointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            PlatformDebugBreak();
        }
    }

    CmdList.TransitionBuffer(DirectionalLightsBuffer.Get()             , EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(PointLightsBuffer.Get()                   , EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(PointLightsPosRadBuffer.Get()             , EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get()      , EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CmdList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

    if (DirectionalLightDataDirty)
    {
        CmdList.UpdateBuffer(DirectionalLightsBuffer.Get(), 0, sizeof(DirectionalLightData), &DirectionalLightData);
        DirectionalLightDataDirty = false;
    }

    if (!PointLightsData.IsEmpty())
    {
        CmdList.UpdateBuffer(PointLightsBuffer.Get(), 0, PointLightsData.SizeInBytes(), PointLightsData.Data());
        CmdList.UpdateBuffer(PointLightsPosRadBuffer.Get(), 0, PointLightsPosRad.SizeInBytes(), PointLightsPosRad.Data());
    }

    if (!ShadowCastingPointLightsData.IsEmpty())
    {
        CmdList.UpdateBuffer(ShadowCastingPointLightsBuffer.Get(), 0, ShadowCastingPointLightsData.SizeInBytes(), ShadowCastingPointLightsData.Data());
        CmdList.UpdateBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), 0, ShadowCastingPointLightsPosRad.SizeInBytes(), ShadowCastingPointLightsPosRad.Data());
    }

    CmdList.TransitionBuffer(DirectionalLightsBuffer.Get()             , EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(PointLightsBuffer.Get()                   , EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(PointLightsPosRadBuffer.Get()             , EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get()      , EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update Lights");
}

void FLightSetup::Release()
{
    DirectionalShadowMask.Reset();

    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();

    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();

    DirectionalLightsBuffer.Reset();

    PointLightShadowMaps.Reset();

    for (uint32 i = 0; i < 4; i++)
    {
        ShadowMapCascades[i].Reset();
    }

    IrradianceMap.Reset();
    IrradianceMapUAV.Reset();

    SpecularIrradianceMap.Reset();

    for (auto& UAV : SpecularIrradianceMapUAVs)
    {
        UAV.Reset();
    }

    CascadeMatrixBuffer.Reset();
    CascadeMatrixBufferSRV.Reset();
    CascadeMatrixBufferUAV.Reset();

    CascadeSplitsBuffer.Reset();
    CascadeSplitsBufferSRV.Reset();
    CascadeSplitsBufferUAV.Reset();
}
