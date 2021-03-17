#include "LightSetup.h"

#include "RenderLayer/RenderLayer.h"

#include "Debug/Profiler.h"

#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

bool LightSetup::Init()
{
    DirectionalLightsData.Reserve(1);
    DirectionalLightsBuffer = CreateConstantBuffer(DirectionalLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!DirectionalLightsBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DirectionalLightsBuffer->SetName("DirectionalLightsBuffer");
    }
    
    PointLightsData.Reserve(256);
    PointLightsBuffer = CreateConstantBuffer(PointLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!PointLightsBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PointLightsBuffer->SetName("PointLightsBuffer");
    }

    PointLightsPosRad.Reserve(256);
    PointLightsPosRadBuffer = CreateConstantBuffer(PointLightsPosRad.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!PointLightsPosRadBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PointLightsPosRadBuffer->SetName("PointLightsPosRadBuffer");
    }

    ShadowCastingPointLightsData.Reserve(8);
    ShadowCastingPointLightsBuffer = CreateConstantBuffer(
        ShadowCastingPointLightsData.CapacityInBytes(), 
        BufferFlag_Default, 
        EResourceState::VertexAndConstantBuffer, 
        nullptr);
    if (!ShadowCastingPointLightsBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsBuffer->SetName("ShadowCastingPointLightsBuffer");
    }

    ShadowCastingPointLightsPosRad.Reserve(8);
    ShadowCastingPointLightsPosRadBuffer = CreateConstantBuffer(
        ShadowCastingPointLightsPosRad.CapacityInBytes(), 
        BufferFlag_Default, 
        EResourceState::VertexAndConstantBuffer, 
        nullptr);
    if (!ShadowCastingPointLightsPosRadBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsPosRadBuffer->SetName("ShadowCastingPointLightsPosRadBuffer");
    }

    return true;
}

void LightSetup::BeginFrame(CommandList& CmdList, const Scene& Scene)
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    PointLightShadowMapsGenerationData.Clear();
    DirLightShadowMapsGenerationData.Clear();
    DirectionalLightsData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    for (Light* Light : Scene.GetLights())
    {
        float Intensity = Light->GetIntensity();
        XMFLOAT3 Color  = Light->GetColor();
        Color = Color * Intensity;
        if (IsSubClassOf<PointLight>(Light))
        {
            PointLight* CurrentLight = Cast<PointLight>(Light);
            Assert(CurrentLight != nullptr);

            constexpr float MinLuma = 0.005f;
            float Dot    = Color.x * 0.2126f + Color.y * 0.7152f + Color.z * 0.0722f;
            float Radius = sqrt(Dot / MinLuma);
            
            XMFLOAT3 Position = CurrentLight->GetPosition();
            XMFLOAT4 PosRad   = XMFLOAT4(Position.x, Position.y, Position.z, Radius);
            if (CurrentLight->IsShadowCaster())
            {
                ShadowCastingPointLightData Data;
                Data.Color         = Color;
                Data.FarPlane      = CurrentLight->GetShadowFarPlane();
                Data.MaxShadowBias = CurrentLight->GetMaxShadowBias();
                Data.ShadowBias    = CurrentLight->GetShadowBias();

                ShadowCastingPointLightsData.EmplaceBack(Data);
                ShadowCastingPointLightsPosRad.EmplaceBack(PosRad);

                PointLightShadowMapGenerationData ShadowMapData;
                ShadowMapData.FarPlane = CurrentLight->GetShadowFarPlane();
                ShadowMapData.Position = CurrentLight->GetPosition();
                
                for (uint32 Face = 0; Face < 6; Face++)
                {
                    ShadowMapData.Matrix[Face]     = CurrentLight->GetMatrix(Face);
                    ShadowMapData.ViewMatrix[Face] = CurrentLight->GetViewMatrix(Face);
                    ShadowMapData.ProjMatrix[Face] = CurrentLight->GetProjectionMatrix(Face);
                }

                PointLightShadowMapsGenerationData.EmplaceBack(ShadowMapData);
            }
            else
            {
                PointLightData Data;
                Data.Color = Color;

                PointLightsData.EmplaceBack(Data);
                PointLightsPosRad.EmplaceBack(PosRad);
            }
        }
        else if (IsSubClassOf<DirectionalLight>(Light))
        {
            DirectionalLight* CurrentLight = Cast<DirectionalLight>(Light);
            Assert(CurrentLight != nullptr);

            DirectionalLightData Data;
            Data.Color      = Color;
            Data.ShadowBias = CurrentLight->GetShadowBias();
            Data.Direction  = CurrentLight->GetDirection();

            // TODO: Should not be the done in the renderer
            XMFLOAT3 CameraPosition = Scene.GetCamera()->GetPosition();
            XMFLOAT3 CameraForward  = Scene.GetCamera()->GetForward();

            float Near       = Scene.GetCamera()->GetNearPlane();
            float DirFrustum = 35.0f;
            XMFLOAT3 LookAt  = CameraPosition + (CameraForward * (DirFrustum + Near));
            CurrentLight->SetLookAt(LookAt);

            Data.LightMatrix   = CurrentLight->GetMatrix();
            Data.MaxShadowBias = CurrentLight->GetMaxShadowBias();

            DirectionalLightsData.EmplaceBack(Data);

            DirLightShadowMapGenerationData ShadowData;
            ShadowData.Matrix   = CurrentLight->GetMatrix();
            ShadowData.FarPlane = CurrentLight->GetShadowFarPlane();
            ShadowData.Position = CurrentLight->GetShadowMapPosition();

            DirLightShadowMapsGenerationData.EmplaceBack(ShadowData);
        }
    }

    if (DirectionalLightsData.SizeInBytes() > DirectionalLightsBuffer->GetSize())
    {
        CmdList.DiscardResource(DirectionalLightsBuffer.Get());

        DirectionalLightsBuffer = CreateConstantBuffer(DirectionalLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!DirectionalLightsBuffer)
        {
            Debug::DebugBreak();
        }
    }

    if (PointLightsData.SizeInBytes() > PointLightsBuffer->GetSize())
    {
        CmdList.DiscardResource(PointLightsBuffer.Get());

        PointLightsBuffer = CreateConstantBuffer(PointLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!PointLightsBuffer)
        {
            Debug::DebugBreak();
        }
    }

    if (PointLightsPosRad.SizeInBytes() > PointLightsPosRadBuffer->GetSize())
    {
        CmdList.DiscardResource(PointLightsPosRadBuffer.Get());

        PointLightsPosRadBuffer = CreateConstantBuffer(PointLightsPosRad.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!PointLightsPosRadBuffer)
        {
            Debug::DebugBreak();
        }
    }

    if (ShadowCastingPointLightsData.SizeInBytes() > ShadowCastingPointLightsBuffer->GetSize())
    {
        CmdList.DiscardResource(ShadowCastingPointLightsBuffer.Get());

        ShadowCastingPointLightsBuffer = CreateConstantBuffer(
            ShadowCastingPointLightsData.CapacityInBytes(), 
            BufferFlag_Default, 
            EResourceState::VertexAndConstantBuffer, 
            nullptr);
        if (!ShadowCastingPointLightsBuffer)
        {
            Debug::DebugBreak();
        }
    }

    if (ShadowCastingPointLightsPosRad.SizeInBytes() > ShadowCastingPointLightsPosRadBuffer->GetSize())
    {
        CmdList.DiscardResource(ShadowCastingPointLightsPosRadBuffer.Get());

        ShadowCastingPointLightsPosRadBuffer = CreateConstantBuffer(
            ShadowCastingPointLightsPosRad.CapacityInBytes(), 
            BufferFlag_Default, 
            EResourceState::VertexAndConstantBuffer, 
            nullptr);
        if (!ShadowCastingPointLightsPosRadBuffer)
        {
            Debug::DebugBreak();
        }
    }

    CmdList.TransitionBuffer(DirectionalLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.TransitionBuffer(PointLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

    if (!DirectionalLightsData.IsEmpty())
    {
        CmdList.UpdateBuffer(DirectionalLightsBuffer.Get(), 0, DirectionalLightsData.SizeInBytes(), DirectionalLightsData.Data());
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

    CmdList.TransitionBuffer(DirectionalLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(PointLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(PointLightsPosRadBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(ShadowCastingPointLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    CmdList.TransitionBuffer(ShadowCastingPointLightsPosRadBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update Lights");
}

void LightSetup::Release()
{
    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();
    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();
    DirectionalLightsBuffer.Reset();

    PointLightShadowMaps.Reset();

    for (auto& DSVCube : PointLightShadowMapDSVs)
    {
        for (uint32 i = 0; i < 6; i++)
        {
            DSVCube[i].Reset();
        }
    }

    DirLightShadowMaps.Reset();

    IrradianceMap.Reset();
    IrradianceMapUAV.Reset();

    SpecularIrradianceMap.Reset();

    for (auto& UAV : SpecularIrradianceMapUAVs)
    {
        UAV.Reset();
    }
}
