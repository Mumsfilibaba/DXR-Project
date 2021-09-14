#include "LightSetup.h"

#include "RenderLayer/RenderLayer.h"

#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Core/Debug/Profiler.h"

bool LightSetup::Init()
{
    DirectionalLightsBuffer = CreateConstantBuffer( sizeof( DirectionalLightData ), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
    if ( !DirectionalLightsBuffer )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DirectionalLightsBuffer->SetName( "DirectionalLightsBuffer" );
    }

    PointLightsData.Reserve( 256 );
    PointLightsBuffer = CreateConstantBuffer( PointLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
    if ( !PointLightsBuffer )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PointLightsBuffer->SetName( "PointLightsBuffer" );
    }

    PointLightsPosRad.Reserve( 256 );
    PointLightsPosRadBuffer = CreateConstantBuffer( PointLightsPosRad.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
    if ( !PointLightsPosRadBuffer )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PointLightsPosRadBuffer->SetName( "PointLightsPosRadBuffer" );
    }

    ShadowCastingPointLightsData.Reserve( 8 );
    ShadowCastingPointLightsBuffer = CreateConstantBuffer(
        ShadowCastingPointLightsData.CapacityInBytes(),
        BufferFlag_Default,
        EResourceState::VertexAndConstantBuffer,
        nullptr );
    if ( !ShadowCastingPointLightsBuffer )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsBuffer->SetName( "ShadowCastingPointLightsBuffer" );
    }

    ShadowCastingPointLightsPosRad.Reserve( 8 );
    ShadowCastingPointLightsPosRadBuffer = CreateConstantBuffer(
        ShadowCastingPointLightsPosRad.CapacityInBytes(),
        BufferFlag_Default,
        EResourceState::VertexAndConstantBuffer,
        nullptr );
    if ( !ShadowCastingPointLightsPosRadBuffer )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        ShadowCastingPointLightsPosRadBuffer->SetName( "ShadowCastingPointLightsPosRadBuffer" );
    }

    return true;
}

void LightSetup::BeginFrame( CommandList& CmdList, const Scene& Scene )
{
    PointLightsPosRad.Clear();
    PointLightsData.Clear();
    ShadowCastingPointLightsPosRad.Clear();
    ShadowCastingPointLightsData.Clear();
    PointLightShadowMapsGenerationData.Clear();

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin Update Lights" );

    TRACE_SCOPE( "Update LightBuffers" );

    Camera* Camera = Scene.GetCamera();
    Assert( Camera != nullptr );

    for ( Light* Light : Scene.GetLights() )
    {
        float Intensity = Light->GetIntensity();
        CVector3 Color = Light->GetColor();
        Color = Color * Intensity;

        if ( IsSubClassOf<PointLight>( Light ) )
        {
            PointLight* CurrentLight = Cast<PointLight>( Light );
            Assert( CurrentLight != nullptr );

            constexpr float MinLuma = 0.005f;
            float Dot = Color.x * 0.2126f + Color.y * 0.7152f + Color.z * 0.0722f;
            float Radius = sqrt( Dot / MinLuma );

            CVector3 Position = CurrentLight->GetPosition();
            CVector4 PosRad = CVector4( Position, Radius );
            if ( CurrentLight->IsShadowCaster() )
            {
                ShadowCastingPointLightData Data;
                Data.Color = Color;
                Data.FarPlane = CurrentLight->GetShadowFarPlane();
                Data.MaxShadowBias = CurrentLight->GetMaxShadowBias();
                Data.ShadowBias = CurrentLight->GetShadowBias();

                ShadowCastingPointLightsData.Emplace( Data );
                ShadowCastingPointLightsPosRad.Emplace( PosRad );

                PointLightShadowMapGenerationData ShadowMapData;
                ShadowMapData.FarPlane = CurrentLight->GetShadowFarPlane();
                ShadowMapData.Position = CurrentLight->GetPosition();

                for ( uint32 Face = 0; Face < 6; Face++ )
                {
                    ShadowMapData.Matrix[Face] = CurrentLight->GetMatrix( Face );
                    ShadowMapData.ViewMatrix[Face] = CurrentLight->GetViewMatrix( Face );
                    ShadowMapData.ProjMatrix[Face] = CurrentLight->GetProjectionMatrix( Face );
                }

                PointLightShadowMapsGenerationData.Emplace( ShadowMapData );
            }
            else
            {
                PointLightData Data;
                Data.Color = Color;

                PointLightsData.Emplace( Data );
                PointLightsPosRad.Emplace( PosRad );
            }
        }
        else if ( IsSubClassOf<DirectionalLight>( Light ) )
        {
            DirectionalLight* CurrentLight = Cast<DirectionalLight>( Light );
            Assert( CurrentLight != nullptr );

            CurrentLight->UpdateCascades( *Camera );

            DirectionalLightData.Color = CVector3( Color.x, Color.y, Color.z );
            DirectionalLightData.ShadowBias = CurrentLight->GetShadowBias();
            DirectionalLightData.Direction = CurrentLight->GetDirection();
            DirectionalLightData.Up = CurrentLight->GetUp();
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

    if ( PointLightsData.SizeInBytes() > (int32)PointLightsBuffer->GetSize() )
    {
        CmdList.DiscardResource( PointLightsBuffer.Get() );

        PointLightsBuffer = CreateConstantBuffer( PointLightsData.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
        if ( !PointLightsBuffer )
        {
            Debug::DebugBreak();
        }
    }

    if ( PointLightsPosRad.SizeInBytes() > (int32)PointLightsPosRadBuffer->GetSize() )
    {
        CmdList.DiscardResource( PointLightsPosRadBuffer.Get() );

        PointLightsPosRadBuffer = CreateConstantBuffer( PointLightsPosRad.CapacityInBytes(), BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
        if ( !PointLightsPosRadBuffer )
        {
            Debug::DebugBreak();
        }
    }

    if ( ShadowCastingPointLightsData.SizeInBytes() > (int32)ShadowCastingPointLightsBuffer->GetSize() )
    {
        CmdList.DiscardResource( ShadowCastingPointLightsBuffer.Get() );

        ShadowCastingPointLightsBuffer = CreateConstantBuffer(
            ShadowCastingPointLightsData.CapacityInBytes(),
            BufferFlag_Default,
            EResourceState::VertexAndConstantBuffer,
            nullptr );
        if ( !ShadowCastingPointLightsBuffer )
        {
            Debug::DebugBreak();
        }
    }

    if ( ShadowCastingPointLightsPosRad.SizeInBytes() > (int32)ShadowCastingPointLightsPosRadBuffer->GetSize() )
    {
        CmdList.DiscardResource( ShadowCastingPointLightsPosRadBuffer.Get() );

        ShadowCastingPointLightsPosRadBuffer = CreateConstantBuffer(
            ShadowCastingPointLightsPosRad.CapacityInBytes(),
            BufferFlag_Default,
            EResourceState::VertexAndConstantBuffer,
            nullptr );
        if ( !ShadowCastingPointLightsPosRadBuffer )
        {
            Debug::DebugBreak();
        }
    }

    CmdList.TransitionBuffer( DirectionalLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
    CmdList.TransitionBuffer( PointLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
    CmdList.TransitionBuffer( PointLightsPosRadBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
    CmdList.TransitionBuffer( ShadowCastingPointLightsBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
    CmdList.TransitionBuffer( ShadowCastingPointLightsPosRadBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );

    if ( DirectionalLightDataDirty )
    {
        CmdList.UpdateBuffer( DirectionalLightsBuffer.Get(), 0, sizeof( DirectionalLightData ), &DirectionalLightData );

        DirectionalLightDataDirty = false;
    }

    if ( !PointLightsData.IsEmpty() )
    {
        CmdList.UpdateBuffer( PointLightsBuffer.Get(), 0, PointLightsData.SizeInBytes(), PointLightsData.Data() );
        CmdList.UpdateBuffer( PointLightsPosRadBuffer.Get(), 0, PointLightsPosRad.SizeInBytes(), PointLightsPosRad.Data() );
    }

    if ( !ShadowCastingPointLightsData.IsEmpty() )
    {
        CmdList.UpdateBuffer( ShadowCastingPointLightsBuffer.Get(), 0, ShadowCastingPointLightsData.SizeInBytes(), ShadowCastingPointLightsData.Data() );
        CmdList.UpdateBuffer( ShadowCastingPointLightsPosRadBuffer.Get(), 0, ShadowCastingPointLightsPosRad.SizeInBytes(), ShadowCastingPointLightsPosRad.Data() );
    }

    CmdList.TransitionBuffer( DirectionalLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );
    CmdList.TransitionBuffer( PointLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );
    CmdList.TransitionBuffer( PointLightsPosRadBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );
    CmdList.TransitionBuffer( ShadowCastingPointLightsBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );
    CmdList.TransitionBuffer( ShadowCastingPointLightsPosRadBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End Update Lights" );
}

void LightSetup::Release()
{
    DirectionalShadowMask.Reset();

    PointLightsPosRadBuffer.Reset();
    PointLightsBuffer.Reset();

    ShadowCastingPointLightsBuffer.Reset();
    ShadowCastingPointLightsPosRadBuffer.Reset();

    DirectionalLightsBuffer.Reset();

    PointLightShadowMaps.Reset();

    for ( auto& DSVCube : PointLightShadowMapDSVs )
    {
        for ( uint32 i = 0; i < 6; i++ )
        {
            DSVCube[i].Reset();
        }
    }

    for ( uint32 i = 0; i < 4; i++ )
    {
        ShadowMapCascades[i].Reset();
    }

    IrradianceMap.Reset();
    IrradianceMapUAV.Reset();

    SpecularIrradianceMap.Reset();

    for ( auto& UAV : SpecularIrradianceMapUAVs )
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
