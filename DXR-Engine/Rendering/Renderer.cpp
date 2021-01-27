#include "Renderer.h"
#include "TextureFactory.h"
#include "DebugUI.h"
#include "Mesh.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Application/Events/EventDispatcher.h"

#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include <algorithm>
#include <imgui_internal.h>

static const UInt32  ShadowMapSampleCount = 2;

ConsoleVariable GlobalDrawTextureDebugger(ConsoleVariableType_Bool);
ConsoleVariable GlobalDrawRendererInfo(ConsoleVariableType_Bool);

ConsoleVariable GlobalEnableSSAO(ConsoleVariableType_Bool);
ConsoleVariable GlobalEnableFXAA(ConsoleVariableType_Bool);

ConsoleVariable GlobalSSAORadius(ConsoleVariableType_Float);
ConsoleVariable GlobalSSAOBias(ConsoleVariableType_Float);
ConsoleVariable GlobalSSAOKernelSize(ConsoleVariableType_Int);

struct CameraBufferDesc
{
    XMFLOAT4X4 ViewProjection;
    XMFLOAT4X4 View;
    XMFLOAT4X4 ViewInv;
    XMFLOAT4X4 Projection;
    XMFLOAT4X4 ProjectionInv;
    XMFLOAT4X4 ViewProjectionInv;
    XMFLOAT3   Position;
    Float      NearPlane;
    Float      FarPlane;
    Float      AspectRatio;
};

void Renderer::Tick(const Scene& CurrentScene)
{
    // Perform frustum culling
    DeferredVisibleCommands.Clear();
    ForwardVisibleCommands.Clear();
    DebugTextures.Clear();

    PointLightFrame++;
    if (PointLightFrame > 6)
    {
        UpdatePointLight = true;
        PointLightFrame  = 0;
    }

    DirLightFrame++;
    if (DirLightFrame > 6)
    {
        UpdateDirLight = true;
        DirLightFrame  = 0;
    }

    if (GlobalFrustumCullEnabled)
    {
        TRACE_SCOPE("Frustum Culling");

        Camera* Camera        = CurrentScene.GetCamera();
        Frustum CameraFrustum = Frustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
        for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
        {
            const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
            XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
            XMVECTOR XmTop       = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
            XMVECTOR XmBottom    = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
            XmTop    = XMVector4Transform(XmTop, XmTransform);
            XmBottom = XMVector4Transform(XmBottom, XmTransform);

            AABB Box;
            XMStoreFloat3(&Box.Top, XmTop);
            XMStoreFloat3(&Box.Bottom, XmBottom);
            if (CameraFrustum.CheckAABB(Box))
            {
                if (Command.Material->HasAlphaMask())
                {
                    ForwardVisibleCommands.EmplaceBack(Command);
                }
                else
                {
                    DeferredVisibleCommands.EmplaceBack(Command);
                }
            }
        }
    }
    else
    {
        for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
        {
            if (Command.Material->HasAlphaMask())
            {
                ForwardVisibleCommands.EmplaceBack(Command);
            }
            else
            {
                DeferredVisibleCommands.EmplaceBack(Command);
            }
        }
    }

    Resources.BackBuffer    = Resources.MainWindowViewport->GetBackBuffer();
    Resources.BackBufferRTV = Resources.MainWindowViewport->GetRenderTargetView();

    CmdList.Begin();
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--BEGIN FRAME--");

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update LightBuffers");

    {
        TRACE_SCOPE("Update LightBuffers");

        CmdList.TransitionBuffer(
            Resources.PointLightBuffer.Get(),
            EResourceState::ResourceState_VertexAndConstantBuffer, 
            EResourceState::ResourceState_CopyDest);

        CmdList.TransitionBuffer(
            Resources.DirectionalLightBuffer.Get(),
            EResourceState::ResourceState_VertexAndConstantBuffer, 
            EResourceState::ResourceState_CopyDest);

        UInt32 NumPointLights    = 0;
        UInt32 NumDirLights        = 0;
        for (Light* Light : CurrentScene.GetLights())
        {
            XMFLOAT3 Color     = Light->GetColor();
            Float    Intensity = Light->GetIntensity();
            if (IsSubClassOf<PointLight>(Light) && UpdatePointLight)
            {
                PointLight* CurrentLight = Cast<PointLight>(Light);
                VALIDATE(CurrentLight != nullptr);

                PointLightProperties Properties;
                Properties.Color         = XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
                Properties.Position      = CurrentLight->GetPosition();
                Properties.ShadowBias    = CurrentLight->GetShadowBias();
                Properties.MaxShadowBias = CurrentLight->GetMaxShadowBias();
                Properties.FarPlane      = CurrentLight->GetShadowFarPlane();

                constexpr Float MinLuma = 0.05f;
                const Float Dot =
                    Properties.Color.x * 0.2126f +
                    Properties.Color.y * 0.7152f +
                    Properties.Color.z * 0.0722f;

                Float Radius = sqrt(Dot / MinLuma);
                Properties.Radius = Radius;

                constexpr UInt32 SizeInBytes = sizeof(PointLightProperties);
                CmdList.UpdateBuffer(
                    Resources.PointLightBuffer.Get(), 
                    NumPointLights * SizeInBytes, 
                    SizeInBytes, 
                    &Properties);

                NumPointLights++;
            }
            else if (IsSubClassOf<DirectionalLight>(Light) && UpdateDirLight)
            {
                DirectionalLight* CurrentLight = Cast<DirectionalLight>(Light);
                VALIDATE(CurrentLight != nullptr);

                DirectionalLightProperties Properties;
                Properties.Color            = XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
                Properties.ShadowBias        = CurrentLight->GetShadowBias();
                Properties.Direction        = CurrentLight->GetDirection();
                Properties.LightMatrix        = CurrentLight->GetMatrix();
                Properties.MaxShadowBias    = CurrentLight->GetMaxShadowBias();

                constexpr UInt32 SizeInBytes = sizeof(DirectionalLightProperties);
                CmdList.UpdateBuffer(
                    DirectionalLightBuffer.Get(),
                    NumDirLights * SizeInBytes,
                    SizeInBytes,
                    &Properties);

                NumDirLights++;
            }
        }

        CmdList.TransitionBuffer(
            Resources.PointLightBuffer.Get(),
            EResourceState::ResourceState_CopyDest, 
            EResourceState::ResourceState_VertexAndConstantBuffer);
    
        CmdList.TransitionBuffer(
            Resources.DirectionalLightBuffer.Get(),
            EResourceState::ResourceState_CopyDest, 
            EResourceState::ResourceState_VertexAndConstantBuffer);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update LightBuffers");

    // Transition GBuffer
    CmdList.TransitionTexture(
        GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
        EResourceState::ResourceState_NonPixelShaderResource, 
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);

    CmdList.TransitionTexture(
        GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    // Transition ShadowMaps
    CmdList.TransitionTexture(
        PointLightShadowMaps.Get(), 
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);
    
    CmdList.TransitionTexture(
        DirLightShadowMaps.Get(), 
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);
    
    // Render DirectionalLight ShadowMaps
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");
    
    if (UpdateDirLight)
    {
        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        CmdList.ClearDepthStencilView(DirLightShadowMapDSV.Get(), DepthStencilClearValue(1.0f, 0));

#if ENABLE_VSM
        CmdList.TransitionTexture(VSMDirLightShadowMaps.Get(), EResourceState::ResourceState_PixelShaderResource, EResourceState::ResourceState_RenderTarget);

        //Float32 DepthClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        //CmdList.ClearRenderTargetView(VSMDirLightShadowMaps->GetRenderTargetView(0).Get(), DepthClearColor);
        //
        //D3D12RenderTargetView* DirLightRTVS[] =
        //{
        //    VSMDirLightShadowMaps->GetRenderTargetView(0).Get(),
        //};
        //CmdList.BindRenderTargets(DirLightRTVS, 1, DirLightShadowMaps->GetDepthStencilView(0).Get());
        //CmdList.BindGraphicsPipelineState(VSMShadowMapPSO.Get());
#else
        CmdList.BindRenderTargets(nullptr, 0, DirLightShadowMapDSV.Get());
        CmdList.BindGraphicsPipelineState(ShadowMapPSO.Get());
#endif

        // Setup view
        CmdList.BindViewport(
            static_cast<Float>(CurrentLightSettings.ShadowMapWidth),
            static_cast<Float>(CurrentLightSettings.ShadowMapHeight),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            CurrentLightSettings.ShadowMapWidth,
            CurrentLightSettings.ShadowMapHeight,
             0, 0);

        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4    Matrix;
            Float        ShadowOffset;
        } ShadowPerObjectBuffer;
    
        PerShadowMap PerShadowMapData;
        for (Light* Light : CurrentScene.GetLights())
        {
            if (IsSubClassOf<DirectionalLight>(Light))
            {
                DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
                PerShadowMapData.Matrix        = DirLight->GetMatrix();
                PerShadowMapData.Position    = DirLight->GetShadowMapPosition();
                PerShadowMapData.FarPlane    = DirLight->GetShadowFarPlane();
            
                CmdList.TransitionBuffer(
                    PerShadowMapBuffer.Get(),
                    EResourceState::ResourceState_VertexAndConstantBuffer,
                    EResourceState::ResourceState_CopyDest);

                CmdList.UpdateBuffer(
                    PerShadowMapBuffer.Get(),
                    0, sizeof(PerShadowMap),
                    &PerShadowMapData);

                CmdList.TransitionBuffer(
                    PerShadowMapBuffer.Get(),
                    EResourceState::ResourceState_CopyDest,
                    EResourceState::ResourceState_VertexAndConstantBuffer);

                CmdList.BindConstantBuffers(
                    EShaderStage::ShaderStage_Vertex,
                    PerShadowMapBuffer.GetAddressOf(), 
                    1, 0);

                // Draw all objects to depthbuffer
                for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
                {
                    CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                    CmdList.BindIndexBuffer(Command.IndexBuffer);

                    ShadowPerObjectBuffer.Matrix        = Command.CurrentActor->GetTransform().GetMatrix();
                    ShadowPerObjectBuffer.ShadowOffset    = Command.Mesh->ShadowOffset;

                    CmdList.Bind32BitShaderConstants(
                        EShaderStage::ShaderStage_Vertex,
                        &ShadowPerObjectBuffer, 17);

                    CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                }

                break;
            }
        }

        UpdateDirLight = false;
    }
    else
    {
        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");

    // Render PointLight ShadowMaps
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");
    
    if (UpdatePointLight)
    {
        TRACE_SCOPE("Render PointLight ShadowMaps");

        const UInt32 PointLightShadowSize = CurrentLightSettings.PointLightShadowSize;
        CmdList.BindViewport(
            static_cast<Float>(PointLightShadowSize),
            static_cast<Float>(PointLightShadowSize),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            static_cast<Float>(PointLightShadowSize),
            static_cast<Float>(PointLightShadowSize),
            0, 0);

        CmdList.BindGraphicsPipelineState(LinearShadowMapPSO.Get());

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4    Matrix;
            Float        ShadowOffset;
        } ShadowPerObjectBuffer;

        UInt32 PointLightShadowIndex = 0;
        PerShadowMap PerShadowMapData;
        for (Light* Light : CurrentScene.GetLights())
        {
            if (IsSubClassOf<PointLight>(Light))
            {
                PointLight* CurrentLight = Cast<PointLight>(Light);
                for (UInt32 Face = 0; Face < 6; Face++)
                {
                    auto& Cube = PointLightShadowMapDSVs[PointLightShadowIndex];
                    CmdList.ClearDepthStencilView(Cube[Face].Get(), DepthStencilClearValue(1.0f, 0));
                    CmdList.BindRenderTargets(nullptr, 0, Cube[Face].Get());

                    PerShadowMapData.Matrix        = CurrentLight->GetMatrix(Face);
                    PerShadowMapData.Position    = CurrentLight->GetPosition();
                    PerShadowMapData.FarPlane    = CurrentLight->GetShadowFarPlane();

                    CmdList.TransitionBuffer(
                        PerShadowMapBuffer.Get(),
                        EResourceState::ResourceState_VertexAndConstantBuffer,
                        EResourceState::ResourceState_CopyDest);

                    CmdList.UpdateBuffer(
                        PerShadowMapBuffer.Get(),
                        0, sizeof(PerShadowMap),
                        &PerShadowMapData);

                    CmdList.TransitionBuffer(
                        PerShadowMapBuffer.Get(),
                        EResourceState::ResourceState_CopyDest,
                        EResourceState::ResourceState_VertexAndConstantBuffer);

                    CmdList.BindConstantBuffers(
                        EShaderStage::ShaderStage_Vertex,
                        PerShadowMapBuffer.GetAddressOf(),
                        1, 0);

                    // Draw all objects to depthbuffer
                    if (GlobalFrustumCullEnabled)
                    {
                        Frustum CameraFrustum = Frustum(CurrentLight->GetShadowFarPlane(), CurrentLight->GetViewMatrix(Face), CurrentLight->GetProjectionMatrix(Face));
                        for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
                        {
                            const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
                            XMMATRIX XmTransform    = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
                            XMVECTOR XmTop            = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
                            XMVECTOR XmBottom        = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
                            XmTop        = XMVector4Transform(XmTop, XmTransform);
                            XmBottom    = XMVector4Transform(XmBottom, XmTransform);

                            AABB Box;
                            XMStoreFloat3(&Box.Top, XmTop);
                            XMStoreFloat3(&Box.Bottom, XmBottom);
                            if (CameraFrustum.CheckAABB(Box))
                            {
                                CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                                CmdList.BindIndexBuffer(Command.IndexBuffer);

                                ShadowPerObjectBuffer.Matrix        = Command.CurrentActor->GetTransform().GetMatrix();
                                ShadowPerObjectBuffer.ShadowOffset    = Command.Mesh->ShadowOffset;
                            
                                CmdList.Bind32BitShaderConstants(
                                    EShaderStage::ShaderStage_Vertex,
                                    &ShadowPerObjectBuffer, 17);

                                CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
                        {
                            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.BindIndexBuffer(Command.IndexBuffer);

                            ShadowPerObjectBuffer.Matrix        = Command.CurrentActor->GetTransform().GetMatrix();
                            ShadowPerObjectBuffer.ShadowOffset    = Command.Mesh->ShadowOffset;
                        
                            CmdList.Bind32BitShaderConstants(
                                EShaderStage::ShaderStage_Vertex,
                                &ShadowPerObjectBuffer, 17);

                            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                        }
                    }
                }

                PointLightShadowIndex++;
            }
        }

        UpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

    // Transition ShadowMaps
#if ENABLE_VSM
    CmdList.TransitionTexture(
        VSMDirLightShadowMaps.Get(), 
        EResourceState::ResourceState_RenderTarget, 
        EResourceState::ResourceState_PixelShaderResource);
#endif
    CmdList.TransitionTexture(
        DirLightShadowMaps.Get(), 
        EResourceState::ResourceState_DepthWrite, 
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        PointLightShadowMaps.Get(), 
        EResourceState::ResourceState_DepthWrite, 
        EResourceState::ResourceState_NonPixelShaderResource);

    // Update camerabuffer
    CameraBufferDesc CamBuff;
    CamBuff.ViewProjection        = CurrentScene.GetCamera()->GetViewProjectionMatrix();
    CamBuff.View                = CurrentScene.GetCamera()->GetViewMatrix();
    CamBuff.ViewInv                = CurrentScene.GetCamera()->GetViewInverseMatrix();
    CamBuff.Projection            = CurrentScene.GetCamera()->GetProjectionMatrix();
    CamBuff.ProjectionInv        = CurrentScene.GetCamera()->GetProjectionInverseMatrix();
    CamBuff.ViewProjectionInv    = CurrentScene.GetCamera()->GetViewProjectionInverseMatrix();
    CamBuff.Position            = CurrentScene.GetCamera()->GetPosition();
    CamBuff.NearPlane            = CurrentScene.GetCamera()->GetNearPlane();
    CamBuff.FarPlane            = CurrentScene.GetCamera()->GetFarPlane();
    CamBuff.AspectRatio            = CurrentScene.GetCamera()->GetAspectRatio();

    CmdList.TransitionBuffer(
        CameraBuffer.Get(), 
        EResourceState::ResourceState_VertexAndConstantBuffer, 
        EResourceState::ResourceState_CopyDest);

    CmdList.UpdateBuffer(CameraBuffer.Get(), 0, sizeof(CameraBufferDesc), &CamBuff);
    
    CmdList.TransitionBuffer(
        CameraBuffer.Get(), 
        EResourceState::ResourceState_CopyDest, 
        EResourceState::ResourceState_VertexAndConstantBuffer);

    // Clear GBuffer
    ColorClearValue BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearDepthStencilView(GBufferDSV.Get(), DepthStencilClearValue(1.0f, 0));

    // Setup view
    const UInt32 RenderWidth    = MainWindowViewport->GetWidth();
    const UInt32 RenderHeight    = MainWindowViewport->GetHeight();

    CmdList.BindViewport(
        static_cast<Float>(RenderWidth),
        static_cast<Float>(RenderHeight),
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    CmdList.BindScissorRect(
        static_cast<Float>(RenderWidth),
        static_cast<Float>(RenderHeight),
        0, 0);

    // Perform PrePass
    if (GlobalPrePassEnabled)
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

        TRACE_SCOPE("PrePass");

        struct PerObject
        {
            XMFLOAT4X4 Matrix;
        } PerObjectBuffer;

        // Setup Pipeline
        CmdList.BindRenderTargets(nullptr, 0, GBufferDSV.Get());

        CmdList.BindGraphicsPipelineState(PrePassPSO.Get());
        
        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Vertex,
            CameraBuffer.GetAddressOf(),
            1, 0);

        // Draw all objects to depthbuffer
        for (const MeshDrawCommand& Command : DeferredVisibleCommands)
        {
            if (!Command.Material->HasHeightMap())
            {
                CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                CmdList.BindIndexBuffer(Command.IndexBuffer);

                PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
            
                CmdList.Bind32BitShaderConstants(
                    EShaderStage::ShaderStage_Vertex,
                    &PerObjectBuffer, 16);

                CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
            }
        }

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End PrePass");
    }

    // Render all objects to the GBuffer
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

    {
        TRACE_SCOPE("GeometryPass");
    
        RenderTargetView* RenderTargets[] =
        {
            GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(),
            GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(),
            GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(),
            GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        };
        CmdList.BindRenderTargets(RenderTargets, 4, GBufferDSV.Get());

        // Setup Pipeline
        CmdList.BindGraphicsPipelineState(GeometryPSO.Get());

        struct TransformBuffer
        {
            XMFLOAT4X4 Transform;
            XMFLOAT4X4 TransformInv;
        } TransformPerObject;

        for (const MeshDrawCommand& Command : DeferredVisibleCommands)
        {
            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.BindIndexBuffer(Command.IndexBuffer);

            if (Command.Material->IsBufferDirty())
            {
                Command.Material->BuildBuffer(CmdList);
            }

            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Vertex,
                &CameraBuffer,
                1, 0);

            ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Pixel,
                &MaterialBuffer,
                1, 1);

            TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
            TransformPerObject.TransformInv    = Command.CurrentActor->GetTransform().GetMatrixInverse();
        
            ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
            CmdList.BindShaderResourceViews(
                EShaderStage::ShaderStage_Pixel,
                ShaderResourceViews, 
                6, 0);

            SamplerState* Sampler = Command.Material->GetMaterialSampler();
            CmdList.BindSamplerStates(
                EShaderStage::ShaderStage_Pixel,
                &Sampler,
                1, 0);

            CmdList.Bind32BitShaderConstants(
                EShaderStage::ShaderStage_Vertex,
                &TransformPerObject, 32);

            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
        }

        // Setup GBuffer for Read
        CmdList.TransitionTexture(
            GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
            EResourceState::ResourceState_RenderTarget, 
            EResourceState::ResourceState_NonPixelShaderResource);
    
        DebugTextures.EmplaceBack(
            GBufferSRVs[GBUFFER_ALBEDO_INDEX],
            GBuffer[GBUFFER_ALBEDO_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
            EResourceState::ResourceState_RenderTarget, 
            EResourceState::ResourceState_NonPixelShaderResource);

        DebugTextures.EmplaceBack(
            GBufferSRVs[GBUFFER_NORMAL_INDEX],
            GBuffer[GBUFFER_NORMAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            EResourceState::ResourceState_RenderTarget,
            EResourceState::ResourceState_NonPixelShaderResource);

        DebugTextures.EmplaceBack(
            GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX],
            GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
            EResourceState::ResourceState_RenderTarget, 
            EResourceState::ResourceState_NonPixelShaderResource);

        DebugTextures.EmplaceBack(
            GBufferSRVs[GBUFFER_MATERIAL_INDEX],
            GBuffer[GBUFFER_MATERIAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
            EResourceState::ResourceState_DepthWrite, 
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            IrradianceMap.Get(),
            EResourceState::ResourceState_PixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            SpecularIrradianceMap.Get(),
            EResourceState::ResourceState_PixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            IntegrationLUT.Get(),
            EResourceState::ResourceState_PixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");

    // RayTracing
    if (RenderLayer::IsRayTracingSupported() && GlobalRayTracingEnabled)
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin RayTracing");

        TraceRays(BackBuffer, CmdList);
        RayTracingGeometryInstances.Clear();

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End RayTracing");
    }

    // SSAO
    CmdList.TransitionTexture(
        SSAOBuffer.Get(), 
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_UnorderedAccess);

    const Float WhiteColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    CmdList.ClearUnorderedAccessView(SSAOBufferUAV.Get(), WhiteColor);

    if (GlobalEnableSSAO.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin SSAO");
        
        TRACE_SCOPE("SSAO");

        struct SSAOSettings
        {
            XMFLOAT2 ScreenSize;
            XMFLOAT2 NoiseSize;
            Float    Radius;
            Float    Bias;
            Int32    KernelSize;
        } SSAOSettings;

        const UInt32 Width      = RenderWidth;
        const UInt32 Height     = RenderHeight;
        SSAOSettings.ScreenSize = XMFLOAT2(Float(Width), Float(Height));
        SSAOSettings.NoiseSize  = XMFLOAT2(4.0f, 4.0f);
        SSAOSettings.Radius     = GlobalSSAORadius.GetFloat();
        SSAOSettings.KernelSize = GlobalSSAOKernelSize.GetInt32();
        SSAOSettings.Bias       = GlobalSSAOBias.GetFloat();

        ShaderResourceView* ShaderResourceViews[] =
        {
            GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
            SSAONoiseSRV.Get(),
            SSAOSamplesSRV.Get()
        };

        CmdList.BindComputePipelineState(SSAOPSO.Get());
        
        CmdList.BindShaderResourceViews(
            EShaderStage::ShaderStage_Compute,
            ShaderResourceViews,
            4, 0);

        SamplerState* SamplerStates[] =
        {
            GBufferSampler.Get()
        };

        CmdList.BindSamplerStates(
            EShaderStage::ShaderStage_Compute,
            SamplerStates,
            1, 0);

        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Compute,
            CameraBuffer.GetAddressOf(),
            1, 0);

        CmdList.BindUnorderedAccessViews(
            EShaderStage::ShaderStage_Compute,
            SSAOBufferUAV.GetAddressOf(),
            1, 0);

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Compute,
            &SSAOSettings, 7);

        constexpr UInt32 ThreadCount = 16;
        const UInt32 DispatchWidth    = Math::DivideByMultiple<UInt32>(Width, ThreadCount);
        const UInt32 DispatchHeight = Math::DivideByMultiple<UInt32>(Height, ThreadCount);
        CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CmdList.UnorderedAccessTextureBarrier(SSAOBuffer.Get());

        CmdList.BindComputePipelineState(SSAOBlurHorizontal.Get());

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Compute,
            &SSAOSettings.ScreenSize, 2);

        CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CmdList.UnorderedAccessTextureBarrier(SSAOBuffer.Get());

        CmdList.BindComputePipelineState(SSAOBlurVertical.Get());

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Compute,
            &SSAOSettings.ScreenSize, 2);

        CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CmdList.UnorderedAccessTextureBarrier(SSAOBuffer.Get());

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End SSAO");
    }

    CmdList.TransitionTexture(
        SSAOBuffer.Get(),
        EResourceState::ResourceState_UnorderedAccess,
        EResourceState::ResourceState_NonPixelShaderResource);

    DebugTextures.EmplaceBack(
        SSAOBufferSRV,
        SSAOBuffer,
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    // Render to final output
    CmdList.TransitionTexture(
        FinalTarget.Get(), 
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_UnorderedAccess);
    
    CmdList.TransitionTexture(
        BackBuffer, 
        EResourceState::ResourceState_Present, 
        EResourceState::ResourceState_RenderTarget);

    // Setup LightPass
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

    {
        TRACE_SCOPE("LightPass");

        CmdList.BindComputePipelineState(DeferredLightPass.Get());

        ShaderResourceView* ShaderResourceViews[] =
        {
            GBufferSRVs[GBUFFER_ALBEDO_INDEX].Get(),
            GBufferSRVs[GBUFFER_NORMAL_INDEX].Get(),
            GBufferSRVs[GBUFFER_MATERIAL_INDEX].Get(),
            GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
            ReflectionTextureSRV.Get(),
            IrradianceMapSRV.Get(),
            SpecularIrradianceMapSRV.Get(),
            IntegrationLUTSRV.Get(),
            DirLightShadowMapSRV.Get(),
            PointLightShadowMapSRV.Get(),
            SSAOBufferSRV.Get()
        };
    
        CmdList.BindShaderResourceViews(
            EShaderStage::ShaderStage_Compute,
            ShaderResourceViews, 
            11, 0);

        ConstantBuffer* ConstantBuffers[] =
        {
            CameraBuffer.Get(),
            PointLightBuffer.Get(),
            DirectionalLightBuffer.Get()
        };

        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Compute,
            ConstantBuffers, 
            3, 0);

        SamplerState* SamplerStates[] =
        {
            GBufferSampler.Get(),
            IntegrationLUTSampler.Get(),
            IrradianceSampler.Get(),
            ShadowMapCompSampler.Get(),
            ShadowMapSampler.Get()
        };

        CmdList.BindSamplerStates(
            EShaderStage::ShaderStage_Compute,
            SamplerStates,
            5,
            0);

        CmdList.BindUnorderedAccessViews(
            EShaderStage::ShaderStage_Compute,
            &FinalTargetUAV,
            1,
            0);

        struct LightPassSettings
        {
            Int32 NumPointLights;
            Int32 NumSkyLightMips;
            Int32 ScreenWidth;
            Int32 ScreenHeight;
        } Settings;

        Settings.NumPointLights        = 4;
        Settings.NumSkyLightMips    = SpecularIrradianceMap->GetMipLevels();
        Settings.ScreenWidth        = FinalTarget->GetWidth();
        Settings.ScreenHeight        = FinalTarget->GetHeight();

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Compute,
            &Settings, 4);

        constexpr UInt32 ThreadCount    = 16;
        const UInt32 WorkGroupWidth        = Math::DivideByMultiple<UInt32>(Settings.ScreenWidth, ThreadCount);
        const UInt32 WorkGroupHeight    = Math::DivideByMultiple<UInt32>(Settings.ScreenHeight, ThreadCount);
        CmdList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End LightPass");

    // Draw skybox
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

    {
        TRACE_SCOPE("Render Skybox");

        RenderTargetView* RenderTarget[] = { FinalTargetRTV.Get() };
        CmdList.BindRenderTargets(RenderTarget, 1, nullptr);

        CmdList.BindViewport(
            static_cast<Float>(RenderWidth),
            static_cast<Float>(RenderHeight),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            static_cast<Float>(RenderWidth),
            static_cast<Float>(RenderHeight),
            0, 0);

        CmdList.TransitionTexture(
            GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
            EResourceState::ResourceState_NonPixelShaderResource, 
            EResourceState::ResourceState_DepthWrite);

        CmdList.TransitionTexture(
            FinalTarget.Get(),
            EResourceState::ResourceState_UnorderedAccess,
            EResourceState::ResourceState_RenderTarget);
    
        CmdList.BindRenderTargets(RenderTarget, 1, GBufferDSV.Get());
    
        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
        CmdList.BindVertexBuffers(&SkyboxVertexBuffer, 1, 0);
        CmdList.BindIndexBuffer(SkyboxIndexBuffer.Get());
        CmdList.BindGraphicsPipelineState(SkyboxPSO.Get());
    
        struct SimpleCameraBuffer
        {
            XMFLOAT4X4 Matrix;
        } SimpleCamera;
        SimpleCamera.Matrix = CurrentScene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Vertex,
            &SimpleCamera, 16);

        CmdList.BindShaderResourceViews(
            EShaderStage::ShaderStage_Pixel,
            &SkyboxSRV,
            1, 0);

        CmdList.BindSamplerStates(
            EShaderStage::ShaderStage_Pixel,
            &SkyboxSampler,
            1, 0);

        CmdList.DrawIndexedInstanced(
            static_cast<UInt32>(SkyboxMesh.Indices.Size()), 
            1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");

    // Render to BackBuffer
    CmdList.TransitionTexture(
        FinalTarget.Get(), 
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_PixelShaderResource);

    DebugTextures.EmplaceBack(
        FinalTargetSRV,
        FinalTarget,
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        PointLightShadowMaps.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        DirLightShadowMaps.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    DebugTextures.EmplaceBack(
        DirLightShadowMapSRV,
        DirLightShadowMaps,
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        IrradianceMap.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        IntegrationLUT.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);
    
    CmdList.BindRenderTargets(&BackBufferView, 1, nullptr);

    CmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Pixel,
        &FinalTargetSRV, 1, 0);

    CmdList.BindSamplerStates(
        EShaderStage::ShaderStage_Pixel,
        &GBufferSampler, 1, 0);

    if (GlobalEnableFXAA.GetBool())
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin FXAA");
        
        TRACE_SCOPE("FXAA");

        struct FXAASettings
        {
            Float Width;
            Float Height;
        } Settings;

        Settings.Width    = static_cast<Float>(RenderWidth);
        Settings.Height    = static_cast<Float>(RenderHeight);

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Pixel, 
            &Settings, 2);

        CmdList.BindGraphicsPipelineState(FXAAPSO.Get());

        CmdList.DrawInstanced(3, 1, 0, 0);

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End FXAA");
    }
    else
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Draw BackBuffer");

        CmdList.BindGraphicsPipelineState(PostPSO.Get());
        CmdList.DrawInstanced(3, 1, 0, 0);

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Draw BackBuffer");
    }

    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin ForwardPass");

    {
        TRACE_SCOPE("ForwardPass");

        CmdList.BindViewport(
            static_cast<Float>(RenderWidth),
            static_cast<Float>(RenderHeight),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            static_cast<Float>(RenderWidth),
            static_cast<Float>(RenderHeight),
            0, 0);

        // Render all transparent objects;
        CmdList.BindRenderTargets(
            &BackBufferView, 1, 
            GBufferDSV.Get());

        // Setup Pipeline
        {
            ConstantBuffer* ConstantBuffers[] =
            {
                CameraBuffer.Get(),
                PointLightBuffer.Get(),
                DirectionalLightBuffer.Get(),
            };

            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Pixel,
                ConstantBuffers,
                3, 0);
        }

        {
            ShaderResourceView* ShaderResourceViews[] =
            {
                IrradianceMapSRV.Get(),
                SpecularIrradianceMapSRV.Get(),
                IntegrationLUTSRV.Get(),
                DirLightShadowMapSRV.Get(),
                PointLightShadowMapSRV.Get(),
            };

            CmdList.BindShaderResourceViews(
                EShaderStage::ShaderStage_Pixel,
                ShaderResourceViews,
                5, 0);
        }

        {
            SamplerState* SamplerStates[] =
            {
                IntegrationLUTSampler.Get(),
                IrradianceSampler.Get(),
                ShadowMapCompSampler.Get(),
                ShadowMapSampler.Get()
            };

            CmdList.BindSamplerStates(
                EShaderStage::ShaderStage_Pixel,
                SamplerStates,
                4, 1);
        }

        struct TransformBuffer
        {
            XMFLOAT4X4 Transform;
            XMFLOAT4X4 TransformInv;
        } TransformPerObject;

        CmdList.BindGraphicsPipelineState(ForwardPSO.Get());
        for (const MeshDrawCommand& Command : ForwardVisibleCommands)
        {
            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.BindIndexBuffer(Command.IndexBuffer);

            if (Command.Material->IsBufferDirty())
            {
                Command.Material->BuildBuffer(CmdList);
            }
        
            ConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Pixel,
                &ConstantBuffer,
                1, 3);

            ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
            CmdList.BindShaderResourceViews(
                EShaderStage::ShaderStage_Pixel,
                ShaderResourceViews,
                7, 5);

            SamplerState* SamplerState = Command.Material->GetMaterialSampler();
            CmdList.BindSamplerStates(
                EShaderStage::ShaderStage_Pixel,
                &SamplerState,
                1, 0);

            TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
            TransformPerObject.TransformInv    = Command.CurrentActor->GetTransform().GetMatrixInverse();

            CmdList.Bind32BitShaderConstants(
                EShaderStage::ShaderStage_Vertex,
                &TransformPerObject, 32);

            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End ForwardPass");

    // Draw DebugBoxes
    if (GlobalDrawAABBs)
    {
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin DebugPass");

        TRACE_SCOPE("DebugPass");

        CmdList.BindGraphicsPipelineState(DebugBoxPSO.Get());

        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_LineList);

        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Vertex,
            &CameraBuffer,
            1, 0);

        CmdList.BindVertexBuffers(&AABBVertexBuffer, 1, 0);
        CmdList.BindIndexBuffer(AABBIndexBuffer.Get());

        for (const MeshDrawCommand& Command : DeferredVisibleCommands)
        {
            AABB& Box = Command.Mesh->BoundingBox;
            XMFLOAT3 Scale        = XMFLOAT3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
            XMFLOAT3 Position    = Box.GetCenter();

            XMMATRIX XmTranslation    = XMMatrixTranslation(Position.x, Position.y, Position.z);
            XMMATRIX XmScale        = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

            XMFLOAT4X4 Transform = Command.CurrentActor->GetTransform().GetMatrix();
            XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
            XMStoreFloat4x4(&Transform, XMMatrixMultiplyTranspose(XMMatrixMultiply(XmScale, XmTranslation), XmTransform));

            CmdList.Bind32BitShaderConstants(
                EShaderStage::ShaderStage_Vertex, 
                &Transform, 16);

            CmdList.DrawIndexedInstanced(24, 1, 0, 0, 0);
        }

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End DebugPass");
    }

    // Render UI
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin UI Render");

    {
        TRACE_SCOPE("Render UI");

        if (GlobalDrawTextureDebugger.GetBool())
        {
            DebugTextures.EmplaceBack(
                GBufferSRVs[GBUFFER_DEPTH_INDEX],
                GBuffer[GBUFFER_DEPTH_INDEX],
                EResourceState::ResourceState_DepthWrite,
                EResourceState::ResourceState_PixelShaderResource);

            DebugUI::DrawUI([]()
            {
                constexpr Float InvAspectRatio    = 16.0f / 9.0f;
                constexpr Float AspectRatio        = 9.0f / 16.0f;

                const UInt32 WindowWidth    = GlobalMainWindow->GetWidth();
                const UInt32 WindowHeight    = GlobalMainWindow->GetHeight();
                const Float Width            = Math::Max(WindowWidth * 0.6f, 400.0f);
                const Float Height            = WindowHeight * 0.75f;

                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

                ImGui::SetNextWindowPos(
                    ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f),
                    ImGuiCond_Appearing,
                    ImVec2(0.5f, 0.0f));

                ImGui::SetNextWindowSize(
                    ImVec2(Width, Height),
                    ImGuiCond_Appearing);
                
                const ImGuiWindowFlags Flags =
                    ImGuiWindowFlags_NoResize            |
                    ImGuiWindowFlags_NoScrollbar        |
                    ImGuiWindowFlags_NoCollapse            |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoSavedSettings;

                Bool TempDrawTextureDebugger = GlobalDrawTextureDebugger.GetBool();
                if (ImGui::Begin(
                    "FrameBuffer Debugger",
                    &TempDrawTextureDebugger,
                    Flags))
                {
                    ImGui::BeginChild(
                        "##ScrollBox",
                        ImVec2(Width * 0.985f, Height * 0.125f),
                        true,
                        ImGuiWindowFlags_HorizontalScrollbar);

                    const Int32 Count = GlobalRenderer->DebugTextures.Size();
                    static Int32 SelectedImage = -1;
                    if (SelectedImage >= Count)
                    {
                        SelectedImage = -1;
                    }

                    for (Int32 i = 0; i < Count; i++)
                    {
                        ImGui::PushID(i);

                        constexpr Float MenuImageSize = 96.0f;
                        Int32    FramePadding = 2;
                        ImVec2    Size = ImVec2(MenuImageSize * InvAspectRatio, MenuImageSize);
                        ImVec2    Uv0 = ImVec2(0.0f, 0.0f);
                        ImVec2    Uv1 = ImVec2(1.0f, 1.0f);
                        ImVec4    BgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                        ImVec4    TintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                        ImGuiImage* CurrImage = &GlobalRenderer->DebugTextures[i];
                        if (ImGui::ImageButton(CurrImage, Size, Uv0, Uv1, FramePadding, BgCol, TintCol))
                        {
                            SelectedImage = i;
                        }

                        ImGui::PopID();

                        if (i != Count - 1)
                        {
                            ImGui::SameLine();
                        }
                    }

                    ImGui::EndChild();

                    const Float ImageWidth    = Width * 0.985f;
                    const Float ImageHeight = ImageWidth * AspectRatio;
                    const Int32 ImageIndex    = SelectedImage < 0 ? 0 : SelectedImage;
                    ImGuiImage* CurrImage    = &GlobalRenderer->DebugTextures[ImageIndex];
                    ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));

                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                }

                ImGui::End();

                GlobalDrawTextureDebugger.SetBool(TempDrawTextureDebugger);
            });
        }
        else
        {
            CmdList.TransitionTexture(
                GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
                EResourceState::ResourceState_DepthWrite, 
                EResourceState::ResourceState_PixelShaderResource);
        }

        if (GlobalDrawRendererInfo.GetBool())
        {
            DebugUI::DrawUI([]()
            {
                const UInt32 WindowWidth    = GlobalMainWindow->GetWidth();
                const UInt32 WindowHeight    = GlobalMainWindow->GetHeight();
                const Float Width            = 300.0f;
                const Float Height            = WindowHeight * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

                ImGui::SetNextWindowPos(
                    ImVec2(Float(WindowWidth), 10.0f),
                    ImGuiCond_Always,
                    ImVec2(1.0f, 0.0f));

                ImGui::SetNextWindowSize(
                    ImVec2(Width, Height),
                    ImGuiCond_Always);

                ImGui::Begin(
                    "Renderer Window",
                    nullptr,
                    ImGuiWindowFlags_NoMove                |
                    ImGuiWindowFlags_NoDecoration        |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoSavedSettings);

                ImGui::Text("Renderer Status:");
                ImGui::Separator();

                ImGui::Columns(2, nullptr, false);
                ImGui::SetColumnWidth(0, 100.0f);

                const std::string AdapterName = RenderLayer::GetAdapterName();
                ImGui::Text("Adapter: ");
                ImGui::NextColumn();

                ImGui::Text("%s", AdapterName.c_str());
                ImGui::NextColumn();

                ImGui::Text("DrawCalls: ");
                ImGui::NextColumn();

                ImGui::Text("%d", GlobalRenderer->LastFrameNumDrawCalls);
                ImGui::NextColumn();

                ImGui::Text("DispatchCalls: ");
                ImGui::NextColumn();

                ImGui::Text("%d", GlobalRenderer->LastFrameNumDispatchCalls);
                ImGui::NextColumn();

                ImGui::Text("Command Count: ");
                ImGui::NextColumn();

                ImGui::Text("%d", GlobalRenderer->LastFrameNumCommands);

                ImGui::Columns(1);

                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::End();
            });
        }

        DebugUI::Render(CmdList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End UI Render");

    // Finalize Commandlist
    CmdList.TransitionTexture(
        BackBuffer, 
        EResourceState::ResourceState_RenderTarget, 
        EResourceState::ResourceState_Present);
    
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--END FRAME--");

    CmdList.End();

    LastFrameNumDrawCalls        = CmdList.GetNumDrawCalls();
    LastFrameNumDispatchCalls    = CmdList.GetNumDispatchCalls();
    LastFrameNumCommands        = CmdList.GetNumCommands();

    {
        TRACE_SCOPE("ExecuteCommandList");
        GlobalCmdListExecutor.ExecuteCommandList(CmdList);
    }

    {
        TRACE_SCOPE("Present");
        MainWindowViewport->Present(GlobalVSyncEnabled);
    }
}

void Renderer::TraceRays(Texture2D* BackBuffer, CommandList& InCmdList)
{
    TRACE_SCOPE("TraceRays");

    InCmdList.TransitionTexture(
        Resources.ReflectionTexture.Get(),
        EResourceState::ResourceState_CopySource, 
        EResourceState::ResourceState_UnorderedAccess);

    UNREFERENCED_VARIABLE(BackBuffer);

    //D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
    //raytraceDesc.Width    = static_cast<UInt32>(ReflectionTexture->GetDesc().Width);
    //raytraceDesc.Height = static_cast<UInt32>(ReflectionTexture->GetDesc().Height);
    //raytraceDesc.Depth    = 1;

    // Set shader tables
    InCmdList.BindRayTracingScene(RayTracingScene.Get());

    // Bind the empty root signature
    // InCommandList->SetComputeRootSignature(GlobalRootSignature->GetRootSignature());
    // InCommandList->SetComputeRootDescriptorTable(GlobalDescriptorTable->GetGPUTableStartHandle(), 0);

    // Dispatch
    InCmdList.BindRayTracingPipelineState(RaytracingPSO.Get());

    // TODO: Input width and height
    InCmdList.DispatchRays(0, 0, 1);

    // Copy the results to the back-buffer
    InCmdList.TransitionTexture(
        ReflectionTexture.Get(), 
        EResourceState::ResourceState_UnorderedAccess, 
        EResourceState::ResourceState_CopySource);
}

void Renderer::SetLightSettings(const LightSettings& InLightSettings)
{
    CurrentLightSettings = InLightSettings;

    GlobalCmdListExecutor.WaitForGPU();

    CreateShadowMaps();

#if ENABLE_VSM
    CmdList.TransitionTexture(
        CurrentRenderer->VSMDirLightShadowMaps.Get(), 
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);
#endif
    CmdList.TransitionTexture(
        DirLightShadowMaps.Get(), 
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);
        
    CmdList.TransitionTexture(
        PointLightShadowMaps.Get(),
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);

    GlobalCmdListExecutor.ExecuteCommandList(CmdList);
}

Bool Renderer::Init()
{
    INIT_CONSOLE_VARIABLE("DrawTextureDebugger", GlobalDrawTextureDebugger);
    GlobalDrawTextureDebugger.SetBool(false);

    INIT_CONSOLE_VARIABLE("DrawRendererInfo", GlobalDrawRendererInfo);
    GlobalDrawRendererInfo.SetBool(false);

    INIT_CONSOLE_VARIABLE("EnableSSAO", GlobalEnableSSAO);
    GlobalEnableSSAO.SetBool(true);

    INIT_CONSOLE_VARIABLE("EnableFXAA", GlobalEnableFXAA);
    GlobalEnableFXAA.SetBool(true);

    INIT_CONSOLE_VARIABLE("SSAOKernelSize", GlobalSSAOKernelSize);
    GlobalSSAOKernelSize.SetInt32(16);

    INIT_CONSOLE_VARIABLE("SSAOBias", GlobalSSAOBias);
    GlobalSSAOBias.SetFloat(0.03f);

    INIT_CONSOLE_VARIABLE("SSAORadius", GlobalSSAORadius);
    GlobalSSAORadius.SetFloat(0.3f);

    Resources.MainWindowViewport = RenderLayer::CreateViewport(
        GlobalMainWindow,
        0, 0,
        EFormat::Format_R8G8B8A8_Unorm,
        EFormat::Format_Unknown);
    if (!Resources.MainWindowViewport)
    {
        return false;
    }
    else
    {
        Resources.MainWindowViewport->SetName("Main Window Viewport");
    }

    Resources.CameraBuffer = RenderLayer::CreateConstantBuffer<CameraBufferDesc>(
        nullptr, 
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!Resources.CameraBuffer)
    {
        LOG_ERROR("[Renderer]: Failed to create camerabuffer");
        return false;
    }
    else
    {
        Resources.CameraBuffer->SetName("CameraBuffer");
    }

    if (!SkyboxRenderPass.Init(Resources))
    {
        return false;
    }

    if (!LightProbeRenderPass.Init(Resources))
    {
        return false;
    }

    CmdList.Begin();

    LightProbeRenderPass.Render(CmdList, Resources);

    CmdList.End();
    GlobalCmdListExecutor.ExecuteCommandList(CmdList);

    if (!PointLightShadowRenderPass.Init(Resources))
    {
        return false;
    }

    // Init standard inputlayout
    InputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::Format_R32G32B32_Float,    0, 0,    EInputClassification::InputClassification_Vertex, 0 },
        { "NORMAL",   0, EFormat::Format_R32G32B32_Float,    0, 12,    EInputClassification::InputClassification_Vertex, 0 },
        { "TANGENT",  0, EFormat::Format_R32G32B32_Float,    0, 24,    EInputClassification::InputClassification_Vertex, 0 },
        { "TEXCOORD", 0, EFormat::Format_R32G32_Float,    0, 36,    EInputClassification::InputClassification_Vertex, 0 },
    };

    Resources.StdInputLayout = RenderLayer::CreateInputLayout(InputLayout);
    if (!Resources.StdInputLayout)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.StdInputLayout->SetName("Standard InputLayoutState");
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressV = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressW = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipPoint;

        Resources.ShadowMapSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!Resources.ShadowMapSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.ShadowMapSampler->SetName("ShadowMap Sampler");
        }
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressV       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.AddressW       = ESamplerMode::SamplerMode_Wrap;
        CreateInfo.Filter         = ESamplerFilter::SamplerFilter_Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = EComparisonFunc::ComparisonFunc_LessEqual;

        Resources.ShadowMapCompSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!Resources.ShadowMapCompSampler)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            Resources.ShadowMapCompSampler->SetName("ShadowMap Comparison Sampler");
        }
    }

    if (!InitRayTracingTexture())
    {
        return false;
    }

    CmdList.Begin();

    CmdList.TransitionTexture(
        Resources.PointLightShadowMaps.Get(), 
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        Resources.DirLightShadowMaps.Get(),
        EResourceState::ResourceState_Common, 
        EResourceState::ResourceState_PixelShaderResource);

    if (Resources.VSMDirLightShadowMaps)
    {
        CmdList.TransitionTexture(
            Resources.VSMDirLightShadowMaps.Get(),
            EResourceState::ResourceState_Common, 
            EResourceState::ResourceState_PixelShaderResource);
    }
    
    CmdList.End();
    GlobalCmdListExecutor.ExecuteCommandList(CmdList);

    if (!InitAA())
    {
        return false;
    }

    // Init RayTracing if supported
    if (RenderLayer::IsRayTracingSupported() && GlobalRayTracingEnabled)
    {
        if (!InitRayTracing())
        {
            return false;
        }
    }

    auto Callback = [](const Event& Event)->Bool
    {
        if (!IsEventOfType<WindowResizeEvent>(Event))
        {
            return false;
        }

        const WindowResizeEvent& ResizeEvent = CastEvent<WindowResizeEvent>(Event);
        const UInt32 Width = ResizeEvent.Width;
        const UInt32 Height = ResizeEvent.Height;

        GlobalCmdListExecutor.WaitForGPU();

        GlobalRenderer->Resources.MainWindowViewport->Resize(Width, Height);

        //GlobalRenderer->InitGBuffer();
        //GlobalRenderer->InitSSAO_RenderTarget();

        return true;
    };

    // Register EventFunc
    GlobalEventDispatcher->RegisterEventHandler(Callback, EEventCategory::EventCategory_Window);

    return true;
}

Bool Renderer::InitRayTracing()
{
    // Create RootSignatures
    //TUniquePtr<D3D12RootSignature> RayGenLocalRoot;
    //{
    //    D3D12_DESCRIPTOR_RANGE Ranges[1] = {};
    //    Ranges[0].BaseShaderRegister                = 0;
    //    Ranges[0].NumDescriptors                    = 1;
    //    Ranges[0].RegisterSpace                        = 0;
    //    Ranges[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    //    Ranges[0].OffsetInDescriptorsFromTableStart    = 0;

    //    D3D12_ROOT_PARAMETER RootParams = { };
    //    RootParams.ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams.ShaderVisibility                        = D3D12_SHADER_VISIBILITY_ALL;
    //    RootParams.DescriptorTable.NumDescriptorRanges    = 1;
    //    RootParams.DescriptorTable.pDescriptorRanges    = Ranges;

    //    D3D12_ROOT_SIGNATURE_DESC RayGenLocalRootDesc = {};
    //    RayGenLocalRootDesc.NumParameters    = 1;
    //    RayGenLocalRootDesc.pParameters        = &RootParams;
    //    RayGenLocalRootDesc.Flags            = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    //    RayGenLocalRoot = RenderLayer::Get().CreateRootSignature(RayGenLocalRootDesc);
    //    if (RayGenLocalRoot)
    //    {
    //        RayGenLocalRoot->SetDebugName("RayGen Local RootSignature");
    //    }
    //    else
    //    {
    //        return false;
    //    }
    //}

    //TUniquePtr<D3D12RootSignature> HitLocalRoot;
    //{
    //    constexpr UInt32 NumRanges0 = 7;
    //    D3D12_DESCRIPTOR_RANGE Ranges0[NumRanges0] = {};
    //    // Albedo
    //    Ranges0[0].BaseShaderRegister                    = 0;
    //    Ranges0[0].NumDescriptors                        = 1;
    //    Ranges0[0].RegisterSpace                        = 1;
    //    Ranges0[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[0].OffsetInDescriptorsFromTableStart    = 0;

    //    // Normal
    //    Ranges0[1].BaseShaderRegister                    = 1;
    //    Ranges0[1].NumDescriptors                        = 1;
    //    Ranges0[1].RegisterSpace                        = 1;
    //    Ranges0[1].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[1].OffsetInDescriptorsFromTableStart    = 1;

    //    // Roughness
    //    Ranges0[2].BaseShaderRegister                    = 2;
    //    Ranges0[2].NumDescriptors                        = 1;
    //    Ranges0[2].RegisterSpace                        = 1;
    //    Ranges0[2].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[2].OffsetInDescriptorsFromTableStart    = 2;

    //    // Height
    //    Ranges0[3].BaseShaderRegister                    = 3;
    //    Ranges0[3].NumDescriptors                        = 1;
    //    Ranges0[3].RegisterSpace                        = 1;
    //    Ranges0[3].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[3].OffsetInDescriptorsFromTableStart    = 3;

    //    // Metallic
    //    Ranges0[4].BaseShaderRegister                    = 4;
    //    Ranges0[4].NumDescriptors                        = 1;
    //    Ranges0[4].RegisterSpace                        = 1;
    //    Ranges0[4].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[4].OffsetInDescriptorsFromTableStart    = 4;

    //    // AO
    //    Ranges0[5].BaseShaderRegister                    = 5;
    //    Ranges0[5].NumDescriptors                        = 1;
    //    Ranges0[5].RegisterSpace                        = 1;
    //    Ranges0[5].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges0[5].OffsetInDescriptorsFromTableStart    = 5;

    //    // MaterialBuffer
    //    Ranges0[6].BaseShaderRegister                    = 0;
    //    Ranges0[6].NumDescriptors                        = 1;
    //    Ranges0[6].RegisterSpace                        = 1;
    //    Ranges0[6].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    //    Ranges0[6].OffsetInDescriptorsFromTableStart    = 6;

    //    constexpr UInt32 NumRanges1 = 2;
    //    D3D12_DESCRIPTOR_RANGE Ranges1[NumRanges1] = {};
    //    // VertexBuffer
    //    Ranges1[0].BaseShaderRegister                    = 6;
    //    Ranges1[0].NumDescriptors                        = 1;
    //    Ranges1[0].RegisterSpace                        = 1;
    //    Ranges1[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges1[0].OffsetInDescriptorsFromTableStart    = 0;

    //    // IndexBuffer
    //    Ranges1[1].BaseShaderRegister                    = 7;
    //    Ranges1[1].NumDescriptors                        = 1;
    //    Ranges1[1].RegisterSpace                        = 1;
    //    Ranges1[1].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges1[1].OffsetInDescriptorsFromTableStart    = 1;

    //    const UInt32 NumRootParams = 2;
    //    D3D12_ROOT_PARAMETER RootParams[NumRootParams];
    //    RootParams[0].ParameterType                            = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams[0].DescriptorTable.NumDescriptorRanges    = NumRanges0;
    //    RootParams[0].DescriptorTable.pDescriptorRanges        = Ranges0;
    //    RootParams[0].ShaderVisibility                        = D3D12_SHADER_VISIBILITY_ALL;

    //    RootParams[1].ParameterType                            = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams[1].DescriptorTable.NumDescriptorRanges    = NumRanges1;
    //    RootParams[1].DescriptorTable.pDescriptorRanges        = Ranges1;
    //    RootParams[1].ShaderVisibility                        = D3D12_SHADER_VISIBILITY_ALL;

    //    D3D12_ROOT_SIGNATURE_DESC HitLocalRootDesc = {};
    //    HitLocalRootDesc.NumParameters    = NumRootParams;
    //    HitLocalRootDesc.pParameters    = RootParams;
    //    HitLocalRootDesc.Flags            = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    //    HitLocalRoot = RenderLayer::Get().CreateRootSignature(HitLocalRootDesc);
    //    if (HitLocalRoot)
    //    {
    //        HitLocalRoot->SetDebugName("Closest Hit Local RootSignature");
    //    }
    //    else
    //    {
    //        return false;
    //    }
    //}

    //TUniquePtr<D3D12RootSignature> MissLocalRoot;
    //{
    //    D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
    //    MissLocalRootDesc.NumParameters    = 0;
    //    MissLocalRootDesc.pParameters    = nullptr;
    //    MissLocalRootDesc.Flags            = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    //TUniquePtr<D3D12RootSignature> HitLocalRoot;
    //{
    //    D3D12_DESCRIPTOR_RANGE Ranges[2] = {};
    //    // VertexBuffer
    //    Ranges[0].BaseShaderRegister                = 2;
    //    Ranges[0].NumDescriptors                    = 1;
    //    Ranges[0].RegisterSpace                        = 0;
    //    Ranges[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[0].OffsetInDescriptorsFromTableStart = 0;

    //    // IndexBuffer
    //    Ranges[1].BaseShaderRegister                = 3;
    //    Ranges[1].NumDescriptors                    = 1;
    //    Ranges[1].RegisterSpace                        = 0;
    //    Ranges[1].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[1].OffsetInDescriptorsFromTableStart    = 1;

    //    D3D12_ROOT_PARAMETER RootParams = { };
    //    RootParams.ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams.DescriptorTable.NumDescriptorRanges    = 2;
    //    RootParams.DescriptorTable.pDescriptorRanges    = Ranges;

    //    D3D12_ROOT_SIGNATURE_DESC HitLocalRootDesc = {};
    //    HitLocalRootDesc.NumParameters    = 1;
    //    HitLocalRootDesc.pParameters    = &RootParams;
    //    HitLocalRootDesc.Flags            = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    //    HitLocalRoot = RenderLayer::Get().CreateRootSignature(HitLocalRootDesc);
    //    if (HitLocalRoot)
    //    {
    //        HitLocalRoot->SetName("Closest Hit Local RootSignature");
    //    }
    //    else
    //    {
    //        return false;
    //    }
    //}

    // Global RootSignature
    //{
    //    constexpr UInt32 NumRanges = 5;
    //    D3D12_DESCRIPTOR_RANGE Ranges[NumRanges] = {};
    //    // AccelerationStructure
    //    Ranges[0].BaseShaderRegister                = 0;
    //    Ranges[0].NumDescriptors                    = 1;
    //    Ranges[0].RegisterSpace                        = 0;
    //    Ranges[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[0].OffsetInDescriptorsFromTableStart    = 0;

    //    // Camera Buffer
    //    Ranges[1].BaseShaderRegister                = 0;
    //    Ranges[1].NumDescriptors                    = 1;
    //    Ranges[1].RegisterSpace                        = 0;
    //    Ranges[1].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    //    Ranges[1].OffsetInDescriptorsFromTableStart    = 1;

    //    // GBuffer NormalMap
    //    Ranges[2].BaseShaderRegister                = 6;
    //    Ranges[2].NumDescriptors                    = 1;
    //    Ranges[2].RegisterSpace                        = 0;
    //    Ranges[2].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[2].OffsetInDescriptorsFromTableStart = 2;

    //    // GBuffer Depth
    //    Ranges[3].BaseShaderRegister                = 7;
    //    Ranges[3].NumDescriptors                    = 1;
    //    Ranges[3].RegisterSpace                        = 0;
    //    Ranges[3].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[3].OffsetInDescriptorsFromTableStart = 3;

    //    // Skybox
    //    Ranges[4].BaseShaderRegister                = 1;
    //    Ranges[4].NumDescriptors                    = 1;
    //    Ranges[4].RegisterSpace                        = 0;
    //    Ranges[4].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[4].OffsetInDescriptorsFromTableStart    = 4;

    //    D3D12_ROOT_PARAMETER RootParams = { };
    //    RootParams.ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams.ShaderVisibility                        = D3D12_SHADER_VISIBILITY_ALL;
    //    RootParams.DescriptorTable.NumDescriptorRanges    = NumRanges;
    //    RootParams.DescriptorTable.pDescriptorRanges    = Ranges;

    //    D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
    //    // Generic Sampler
    //    Samplers[0].ShaderVisibility    = D3D12_SHADER_VISIBILITY_ALL;
    //    Samplers[0].AddressU            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].AddressV            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].AddressW            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].Filter                = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    //    Samplers[0].ShaderRegister        = 0;
    //    Samplers[0].RegisterSpace        = 0;
    //    Samplers[0].MinLOD                = 0.0f;
    //    Samplers[0].MaxLOD                = FLT_MAX;
 //
    //    // GBuffer Sampler
    //    Samplers[1].ShaderVisibility    = D3D12_SHADER_VISIBILITY_ALL;
    //    Samplers[1].AddressU            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].AddressV            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].AddressW            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].Filter                = D3D12_FILTER_MIN_MAG_MIP_POINT;
    //    Samplers[1].ShaderRegister        = 1;
    //    Samplers[1].RegisterSpace        = 0;
    //    Samplers[1].MinLOD                = 0.0f;
    //    Samplers[1].MaxLOD                = FLT_MAX;

    //    D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
    //    GlobalRootDesc.NumStaticSamplers    = 2;
    //    GlobalRootDesc.pStaticSamplers        = Samplers;
    //    GlobalRootDesc.NumParameters        = 1;
    //    GlobalRootDesc.pParameters            = &RootParams;
    //    GlobalRootDesc.Flags                = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    //    GlobalRootSignature = RenderLayer::Get().CreateRootSignature(GlobalRootDesc);
    //    if (!GlobalRootSignature)
    //    {
    //        return false;
    //    }
    //}

    //// Global RootSignature
    //{
    //    D3D12_DESCRIPTOR_RANGE Ranges[7] = {};
    //    // AccelerationStructure
    //    Ranges[0].BaseShaderRegister                = 0;
    //    Ranges[0].NumDescriptors                    = 1;
    //    Ranges[0].RegisterSpace                        = 0;
    //    Ranges[0].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[0].OffsetInDescriptorsFromTableStart    = 0;

    //    // Camera Buffer
    //    Ranges[1].BaseShaderRegister                = 0;
    //    Ranges[1].NumDescriptors                    = 1;
    //    Ranges[1].RegisterSpace                        = 0;
    //    Ranges[1].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    //    Ranges[1].OffsetInDescriptorsFromTableStart    = 1;

    //    // Skybox
    //    Ranges[2].BaseShaderRegister                = 1;
    //    Ranges[2].NumDescriptors                    = 1;
    //    Ranges[2].RegisterSpace                        = 0;
    //    Ranges[2].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[2].OffsetInDescriptorsFromTableStart    = 2;

    //    // Albedo
    //    Ranges[3].BaseShaderRegister                = 4;
    //    Ranges[3].NumDescriptors                    = 1;
    //    Ranges[3].RegisterSpace                        = 0;
    //    Ranges[3].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[3].OffsetInDescriptorsFromTableStart    = 3;

    //    // Normal
    //    Ranges[4].BaseShaderRegister                = 5;
    //    Ranges[4].NumDescriptors                    = 1;
    //    Ranges[4].RegisterSpace                        = 0;
    //    Ranges[4].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[4].OffsetInDescriptorsFromTableStart    = 4;

    //    // GBuffer NormalMap
    //    Ranges[5].BaseShaderRegister                = 6;
    //    Ranges[5].NumDescriptors                    = 1;
    //    Ranges[5].RegisterSpace                        = 0;
    //    Ranges[5].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[5].OffsetInDescriptorsFromTableStart = 5;

    //    // GBuffer Depth
    //    Ranges[6].BaseShaderRegister                = 7;
    //    Ranges[6].NumDescriptors                    = 1;
    //    Ranges[6].RegisterSpace                        = 0;
    //    Ranges[6].RangeType                            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    //    Ranges[6].OffsetInDescriptorsFromTableStart = 6;

    //    D3D12_ROOT_PARAMETER RootParams = { };
    //    RootParams.ParameterType                        = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    RootParams.DescriptorTable.NumDescriptorRanges    = 7;
    //    RootParams.DescriptorTable.pDescriptorRanges    = Ranges;

    //    D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
    //    // Generic Sampler
    //    Samplers[0].ShaderVisibility    = D3D12_SHADER_VISIBILITY_ALL;
    //    Samplers[0].AddressU            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].AddressV            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].AddressW            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    Samplers[0].Filter                = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    //    Samplers[0].ShaderRegister        = 0;
    //    Samplers[0].RegisterSpace        = 0;
    //    Samplers[0].MinLOD                = 0.0f;
    //    Samplers[0].MaxLOD                = FLT_MAX;
 //
    //    // GBuffer Sampler
    //    Samplers[1].ShaderVisibility    = D3D12_SHADER_VISIBILITY_ALL;
    //    Samplers[1].AddressU            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].AddressV            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].AddressW            = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    //    Samplers[1].Filter                = D3D12_FILTER_MIN_MAG_MIP_POINT;
    //    Samplers[1].ShaderRegister        = 1;
    //    Samplers[1].RegisterSpace        = 0;
    //    Samplers[1].MinLOD                = 0.0f;
    //    Samplers[1].MaxLOD                = FLT_MAX;

    //    D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
    //    GlobalRootDesc.NumStaticSamplers    = 2;
    //    GlobalRootDesc.pStaticSamplers        = Samplers;
    //    GlobalRootDesc.NumParameters        = 1;
    //    GlobalRootDesc.pParameters            = &RootParams;
    //    GlobalRootDesc.Flags                = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    //    GlobalRootSignature = RenderLayer::Get().CreateRootSignature(GlobalRootDesc);
    //    if (!GlobalRootSignature)
    //    {
    //        return false;
    //    }
    //}

    //// Create Pipeline
    //RayTracingPipelineStateProperties PipelineProperties;
    //PipelineProperties.DebugName                = "RayTracing PipelineState";
    //PipelineProperties.RayGenRootSignature        = RayGenLocalRoot.Get();
    //PipelineProperties.HitGroupRootSignature    = HitLocalRoot.Get();
    //PipelineProperties.MissRootSignature        = MissLocalRoot.Get();
    //PipelineProperties.GlobalRootSignature        = GlobalRootSignature.Get();
    //PipelineProperties.MaxRecursions            = 4;

    //RaytracingPSO = RenderLayer::Get().CreateRayTracingPipelineState(PipelineProperties);
    //if (!RaytracingPSO)
    //{
    //    return false;
    //}

    //// Build Acceleration Structures
    //RenderLayer::Get().GetImmediateCommandList()->TransitionBarrier(CameraBuffer.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_VertexAndConstantBuffer);

    // Create DescriptorTables
    //RayGenDescriptorTable = TSharedPtr(RenderLayer::Get().CreateDescriptorTable(1));
    //GlobalDescriptorTable = TSharedPtr(RenderLayer::Get().CreateDescriptorTable(7));

    // Create TLAS
    //RayTracingScene = RenderLayer::CreateRayTracingScene();
    //if (!RayTracingScene)
    //{
    //    return false;
    //}

    // Populate descriptors
    //RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).Get(), 0);
    //RayGenDescriptorTable->CopyDescriptors();

    //GlobalDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 1);
    //GlobalDescriptorTable->SetShaderResourceView(GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(0).Get(), 2);
    //GlobalDescriptorTable->SetShaderResourceView(GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(0).Get(), 3);
    //GlobalDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).Get(), 4);
    //GlobalDescriptorTable->CopyDescriptors();

    return true;
}

Bool Renderer::InitRayTracingTexture()
{
    const UInt32 Width  = Resources.MainWindowViewport->GetWidth();
    const UInt32 Height = Resources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RWTexture;
    Resources.ReflectionTexture = RenderLayer::CreateTexture2D(
        nullptr, 
        EFormat::Format_R8G8B8A8_Unorm, 
        Usage, 
        Width, 
        Height, 
        1, 1);
    if (!Resources.ReflectionTexture)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        Resources.ReflectionTexture->SetName("DXR Reflection Texture");
    }

    Resources.ReflectionTextureUAV = RenderLayer::CreateUnorderedAccessView(
        Resources.ReflectionTexture.Get(), 
        EFormat::Format_R8G8B8A8_Unorm, 0);
    if (!Resources.ReflectionTextureUAV)
    {
        Debug::DebugBreak();
        return false;
    }

    Resources.ReflectionTextureSRV = RenderLayer::CreateShaderResourceView(
        Resources.ReflectionTexture.Get(), 
        EFormat::Format_R8G8B8A8_Unorm, 0, 1);
    if (!Resources.ReflectionTextureSRV)
    {
        Debug::DebugBreak();
        return false;
    }

    return true;
}

Bool Renderer::InitAA()
{
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/FullscreenVS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Fullscreen VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/PostProcessPS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("PostProcess PixelShader");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_Always;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_Zero;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("PostProcess DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("PostProcess RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable      = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("PostProcess BlendState");
    }

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.InputLayoutState                       = nullptr;
    PSOProperties.BlendState                             = BlendState.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::Format_R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = EFormat::Format_Unknown;

    PostPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!PostPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PostPSO->SetName("PostProcess PipelineState");
    }

    // FXAA
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/FXAA_PS.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("FXAA PixelShader");
    }

    PSOProperties.ShaderState.PixelShader = PShader.Get();

    FXAAPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!FXAAPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FXAAPSO->SetName("FXAA PipelineState");
    }

    return true;
}