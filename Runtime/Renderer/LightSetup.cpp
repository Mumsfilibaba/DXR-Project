#include "LightSetup.h"

#include "RHI/RHIInterface.h"

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
        DEBUG_BREAK();
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
        DEBUG_BREAK();
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
        DEBUG_BREAK();
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
        DEBUG_BREAK();
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
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadowCastingPointLightsPosRadBuffer->SetName("ShadowCastingPointLightsPosRadBuffer");
    }

    return true;
}

void FLightSetup::BeginFrame(FRHICommandList& CommandList, const FScene& Scene)
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    PointLightShadowMapsGenerationData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    FCamera* Camera = Scene.GetCamera();
    Check(Camera != nullptr);

    for (FLight* Light : Scene.GetLights())
    {
        const float Intensity = Light->GetIntensity();
        FVector3 Color = Light->GetColor();
        Color = Color * Intensity;

        if (IsSubClassOf<FPointLight>(Light))
        {
            FPointLight* CurrentLight = Cast<FPointLight>(Light);
            Check(CurrentLight != nullptr);

            constexpr float MinLuma = 0.005f;
            const float Dot    = Color.x * 0.2126f + Color.y * 0.7152f + Color.z * 0.0722f;
            const float Radius = NMath::Sqrt(Dot / MinLuma);

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

            CurrentLight->Tick(*Camera);

            DirectionalLightData.Color         = FVector3(Color.x, Color.y, Color.z);
            DirectionalLightData.ShadowBias    = CurrentLight->GetShadowBias();
            DirectionalLightData.Direction     = CurrentLight->GetDirection();
            // TODO: Is this used?
            DirectionalLightData.UpVector      = CurrentLight->GetUp();
            DirectionalLightData.MaxShadowBias = CurrentLight->GetMaxShadowBias();
            DirectionalLightData.LightSize     = CurrentLight->GetSize();
            DirectionalLightData.ShadowMatrix  = CurrentLight->GetShadowMatrix();

            CascadeSplitLambda = CurrentLight->GetCascadeSplitLambda();

            DirectionalLightDataDirty = true;
        }
    }

    if (PointLightsData.SizeInBytes() > (int32)PointLightsBuffer->GetSize())
    {
        CommandList.DestroyResource(PointLightsBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, PointLightsData.CapacityInBytes());
        PointLightsBuffer = RHICreateConstantBuffer(Initializer);
        if (!PointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > (int32)PointLightsPosRadBuffer->GetSize())
    {
        CommandList.DestroyResource(PointLightsPosRadBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, PointLightsPosRad.CapacityInBytes());
        PointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
        if (!PointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > (int32)ShadowCastingPointLightsBuffer->GetSize())
    {
        CommandList.DestroyResource(ShadowCastingPointLightsBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, ShadowCastingPointLightsData.CapacityInBytes());
        ShadowCastingPointLightsBuffer = RHICreateConstantBuffer(Initializer);
        if (!ShadowCastingPointLightsBuffer)
        {
            DEBUG_BREAK();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > (int32)ShadowCastingPointLightsPosRadBuffer->GetSize())
    {
        CommandList.DestroyResource(ShadowCastingPointLightsPosRadBuffer.Get());

        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, ShadowCastingPointLightsPosRad.CapacityInBytes());
        ShadowCastingPointLightsPosRadBuffer = RHICreateConstantBuffer(Initializer);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            DEBUG_BREAK();
        }
    }

    CommandList.TransitionBuffer(
        DirectionalLightsBuffer.Get(),
        EResourceAccess::VertexAndConstantBuffer,
        EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(
        PointLightsBuffer.Get(),
        EResourceAccess::VertexAndConstantBuffer, 
        EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(
        PointLightsPosRadBuffer.Get(),
        EResourceAccess::VertexAndConstantBuffer,
        EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(
        ShadowCastingPointLightsBuffer.Get(),
        EResourceAccess::VertexAndConstantBuffer,
        EResourceAccess::CopyDest);
    CommandList.TransitionBuffer(
        ShadowCastingPointLightsPosRadBuffer.Get(),
        EResourceAccess::VertexAndConstantBuffer, 
        EResourceAccess::CopyDest);

    if (DirectionalLightDataDirty)
    {
        CommandList.UpdateBuffer(DirectionalLightsBuffer.Get(), 0, sizeof(DirectionalLightData), &DirectionalLightData);
        DirectionalLightDataDirty = false;
    }

    if (!PointLightsData.IsEmpty())
    {
        CommandList.UpdateBuffer(PointLightsBuffer.Get(), 0, PointLightsData.SizeInBytes(), PointLightsData.GetData());
        CommandList.UpdateBuffer(PointLightsPosRadBuffer.Get(), 0, PointLightsPosRad.SizeInBytes(), PointLightsPosRad.GetData());
    }

    if (!ShadowCastingPointLightsData.IsEmpty())
    {
        CommandList.UpdateBuffer(
            ShadowCastingPointLightsBuffer.Get(),
            0,
            ShadowCastingPointLightsData.SizeInBytes(),
            ShadowCastingPointLightsData.GetData());
        CommandList.UpdateBuffer(
            ShadowCastingPointLightsPosRadBuffer.Get(),
            0,
            ShadowCastingPointLightsPosRad.SizeInBytes(),
            ShadowCastingPointLightsPosRad.GetData());
    }

    CommandList.TransitionBuffer(
        DirectionalLightsBuffer.Get(),
        EResourceAccess::CopyDest,
        EResourceAccess::VertexAndConstantBuffer);
    CommandList.TransitionBuffer(
        PointLightsBuffer.Get(),
        EResourceAccess::CopyDest,
        EResourceAccess::VertexAndConstantBuffer);
    CommandList.TransitionBuffer(
        PointLightsPosRadBuffer.Get(), 
        EResourceAccess::CopyDest,
        EResourceAccess::VertexAndConstantBuffer);
    CommandList.TransitionBuffer(
        ShadowCastingPointLightsBuffer.Get(),
        EResourceAccess::CopyDest,
        EResourceAccess::VertexAndConstantBuffer);
    CommandList.TransitionBuffer(
        ShadowCastingPointLightsPosRadBuffer.Get(),
        EResourceAccess::CopyDest,
        EResourceAccess::VertexAndConstantBuffer);

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

    for (FRHITexture2DRef& ShadowMap : ShadowMapCascades)
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
