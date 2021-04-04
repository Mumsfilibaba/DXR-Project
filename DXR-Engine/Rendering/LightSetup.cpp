#include "LightSetup.h"

#include "RenderLayer/RenderLayer.h"

#include "Debug/Profiler.h"

#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

bool LightSetup::Init()
{
    DirectionalLightsBuffer = CreateConstantBuffer(sizeof(DirectionalLightData), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
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

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update Lights");

    TRACE_SCOPE("Update LightBuffers");

    Camera* Camera = Scene.GetCamera();

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

            DirLightData.Color      = Color;
            DirLightData.ShadowBias = CurrentLight->GetShadowBias();
            DirLightData.Direction  = CurrentLight->GetDirection();

            // TODO: Should not be the done in the renderer
            XMFLOAT3 CameraPosition = Camera->GetPosition();
            XMFLOAT3 CameraForward  = Camera->GetForward();

            XMFLOAT4X4 InvCamera = Camera->GetViewProjectionInverseMatrix();
            XMFLOAT4X4 LightView = CurrentLight->GetViewMatrix();

            DirLightShadowMapsGenerationData.Matrix   = CurrentLight->GetMatrix();
            DirLightShadowMapsGenerationData.FarPlane = CurrentLight->GetShadowFarPlane();
            DirLightShadowMapsGenerationData.Position = CurrentLight->GetShadowMapPosition();

            float NearPlane = Camera->GetNearPlane();
            float FarPlane  = Camera->GetFarPlane();
            float ClipRange = FarPlane - NearPlane;

            float MinZ = NearPlane;
            float MaxZ = FarPlane;

            float Range = ClipRange;
            float Ratio = MaxZ / MinZ;

            // Calculate split depths based on view camera frustum
            // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
            for (uint32 i = 0; i < 4; i++) 
            {
                float p = (i + 1) / static_cast<float>(4);
                float Log = MinZ * std::pow(Ratio, p);
                float Uniform = MinZ + Range * p;
                float d = CascadeSplitLambda * (Log - Uniform) + Uniform;
                DirLightShadowMapsGenerationData.ShadowCascadesFarPlanes[i] = (d - NearPlane) / ClipRange;
            }

            float LastSplitDist = 0.0f;
            for (uint32 i = 0; i < 4; i++)
            {
                float SplitDist = DirLightShadowMapsGenerationData.ShadowCascadesFarPlanes[i];

                XMFLOAT4 FrustumCorners[8] = 
                {
                    XMFLOAT4(-1.0f,  1.0f, -1.0f, 1.0f),
                    XMFLOAT4( 1.0f,  1.0f, -1.0f, 1.0f),
                    XMFLOAT4( 1.0f, -1.0f, -1.0f, 1.0f),
                    XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f),
                    XMFLOAT4(-1.0f,  1.0f,  1.0f, 1.0f),
                    XMFLOAT4( 1.0f,  1.0f,  1.0f, 1.0f),
                    XMFLOAT4( 1.0f, -1.0f,  1.0f, 1.0f),
                    XMFLOAT4(-1.0f, -1.0f,  1.0f, 1.0f),
                };

                // Calculate position of light frustum
                XMMATRIX XmLightView = XMMatrixTranspose(XMLoadFloat4x4(&LightView));
                XMMATRIX XmInvCamera = XMMatrixTranspose(XMLoadFloat4x4(&InvCamera));
                for (uint32 j = 0; j < 8; j++)
                {
                    XMVECTOR XmCorner = XMLoadFloat4(&FrustumCorners[j]);
                    XmCorner = XMVector4Transform(XmCorner, XmInvCamera);
                    XMStoreFloat4(&FrustumCorners[j], XmCorner);

                    FrustumCorners[j] = FrustumCorners[j] / FrustumCorners[j].w;
                }

                for (uint32 j = 0; j < 4; j++)
                {
                    const XMFLOAT4 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
                    FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
                    FrustumCorners[j]     = FrustumCorners[j] + (Distance * LastSplitDist);
                }

                // Calc frustum center
                XMFLOAT4 Center = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
                for (uint32 j = 0; j < 8; j++) 
                {
                    Center = Center + FrustumCorners[j];
                }
                Center = Center / 8.0f;

                float Radius = 0.0f;
                for (uint32 j = 0; j < 8; j++)
                {
                    float Distance = Length(FrustumCorners[j] - Center);
                    Radius = Math::Max(Radius, Distance);
                }
                Radius = std::ceil(Radius * 16.0f) / 16.0f;

                XMFLOAT3 MaxExtents = XMFLOAT3(Radius, Radius, Radius);
                XMFLOAT3 MinExtents = -MaxExtents;

                XMFLOAT3 LightUp   = CurrentLight->GetUp();
                XMFLOAT3 Direction = CurrentLight->GetDirection();
                XMFLOAT3 Position  = XMFLOAT3(Center.x, Center.y, Center.z) - Direction * -MinExtents.z;

                XMVECTOR EyePosition  = XMVectorSet(Position.x, Position.y, Position.z, 1.0f);
                XMVECTOR LookPosition = XMVectorSet(Center.x, Center.y, Center.z, 0.0f);
                XMVECTOR Up           = XMVectorSet(LightUp.x, LightUp.y, LightUp.z, 0.0f);

                XMMATRIX XmViewMatrix = XMMatrixLookAtLH(EyePosition, LookPosition, Up);
                XMMATRIX XmOrtoMatrix = XMMatrixOrthographicOffCenterLH(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, 0.0f, MaxExtents.z - MinExtents.z);
                XMStoreFloat4x4(&DirLightShadowMapsGenerationData.CascadeMatrices[i], XMMatrixMultiplyTranspose(XmViewMatrix, XmOrtoMatrix));

                LastSplitDist = SplitDist;
            }

            DirLightData.LightMatrix   = CurrentLight->GetMatrix();
            DirLightData.MaxShadowBias = CurrentLight->GetMaxShadowBias();

            DirectionalLightDataDirty = true;
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

    if (DirectionalLightDataDirty)
    {
        CmdList.UpdateBuffer(DirectionalLightsBuffer.Get(), 0, sizeof(DirectionalLightData), &DirLightData);
        
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

    DirLightShadowMap.Reset();

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
}
