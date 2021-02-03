#include "ShadowMapRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Rendering/Mesh.h"
#include "Rendering/MeshDrawCommand.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

struct PerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool ShadowMapRenderer::Init(SceneLightSetup& LightSetup, FrameResources& FrameResources)
{
    if (!CreateShadowMaps(LightSetup))
    {
        return false;
    }

    {
        const UInt32 SizeInBytes = sizeof(PointLightProperties) * LightSetup.MaxPointLights;
        LightSetup.PointLightBuffer = RenderLayer::CreateConstantBuffer(BufferFlag_Default, SizeInBytes, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!LightSetup.PointLightBuffer)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.PointLightBuffer->SetName("PointLight Buffer");
        }
    }

    {
        const UInt32 SizeInBytes = sizeof(PointLightProperties) * LightSetup.MaxPointLightShadows;
        LightSetup.ShadowPointLightBuffer = RenderLayer::CreateConstantBuffer(BufferFlag_Default, SizeInBytes, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!LightSetup.ShadowPointLightBuffer)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.ShadowPointLightBuffer->SetName("Shadow PointLight Buffer");
        }
    }

    PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<PerShadowMap>(BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!PerShadowMapBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetName("PerShadowMap Buffer");
    }

    // Linear Shadow Maps
    TArray<UInt8> ShaderCode;
    {
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
            VShader->SetName("Linear ShadowMap VertexShader");
        }

        TSharedRef<PixelShader> PShader;
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
            PShader->SetName("Linear ShadowMap PixelShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
        if (!DepthStencilState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName("Shadow DepthStencilState");
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName("Shadow RasterizerState");
        }

        BlendStateCreateInfo BlendStateInfo;

        TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("Shadow BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.BlendState                         = BlendState.Get();
        PipelineStateInfo.DepthStencilState                  = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue                    = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState                   = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType              = EPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState                    = RasterizerState.Get();
        PipelineStateInfo.SampleCount                        = 1;
        PipelineStateInfo.SampleQuality                      = 0;
        PipelineStateInfo.SampleMask                         = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader           = VShader.Get();
        PipelineStateInfo.ShaderState.PixelShader            = PShader.Get();
        PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        PointLightPipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!PointLightPipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPipelineState->SetName("Linear ShadowMap PipelineState");
        }
    }

    {
        const UInt32 SizeInBytes = sizeof(PointLightProperties) * LightSetup.MaxDirectionalLights;
        LightSetup.DirectionalLightBuffer = RenderLayer::CreateConstantBuffer(BufferFlag_Default, SizeInBytes, EResourceState::VertexAndConstantBuffer, nullptr);
        if (!LightSetup.DirectionalLightBuffer)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.DirectionalLightBuffer->SetName("DirectionalLight Buffer");
        }
    }

    {
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
            VShader->SetName("ShadowMap VertexShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
        if (!DepthStencilState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName("Shadow DepthStencilState");
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName("Shadow RasterizerState");
        }

        BlendStateCreateInfo BlendStateInfo;
        TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("Shadow BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.BlendState                         = BlendState.Get();
        PipelineStateInfo.DepthStencilState                  = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue                    = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState                   = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType              = EPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState                    = RasterizerState.Get();
        PipelineStateInfo.SampleCount                        = 1;
        PipelineStateInfo.SampleQuality                      = 0;
        PipelineStateInfo.SampleMask                         = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader           = VShader.Get();
        PipelineStateInfo.ShaderState.PixelShader            = nullptr;
        PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirLightPipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!DirLightPipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirLightPipelineState->SetName("ShadowMap PipelineState");
        }
    }

    return true;
}

void ShadowMapRenderer::RenderPointLightShadows(CommandList& CmdList, const SceneLightSetup& LightSetup, const Scene& Scene)
{
    PointLightFrame++;
    if (PointLightFrame > 6)
    {
        UpdatePointLight = true;
        PointLightFrame = 0;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update PointLightBuffer");

    CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);

    if (UpdatePointLight)
    {
        TRACE_SCOPE("Update LightBuffers");

        CmdList.TransitionBuffer(LightSetup.PointLightBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

        UInt32 NumPointLights = 0;
        for (Light* Light : Scene.GetLights())
        {
            Float Intensity = Light->GetIntensity();
            XMFLOAT3 Color  = Light->GetColor();
            if (IsSubClassOf<PointLight>(Light))
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
                const Float Dot = Properties.Color.x * 0.2126f + Properties.Color.y * 0.7152f + Properties.Color.z * 0.0722f;

                Float Radius = sqrt(Dot / MinLuma);
                Properties.Radius = Radius;

                constexpr UInt32 SizeInBytes = sizeof(PointLightProperties);
                CmdList.UpdateBuffer(LightSetup.PointLightBuffer.Get(), NumPointLights * SizeInBytes, SizeInBytes, &Properties);

                NumPointLights++;
            }
        }

        CmdList.TransitionBuffer(LightSetup.PointLightBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update PointLightBuffer");

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");

    if (UpdatePointLight)
    {
        TRACE_SCOPE("Render PointLight ShadowMaps");

        const UInt32 PointLightShadowSize = LightSetup.PointLightShadowSize;
        CmdList.BindViewport(static_cast<Float>(PointLightShadowSize), static_cast<Float>(PointLightShadowSize), 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.BindScissorRect(static_cast<Float>(PointLightShadowSize), static_cast<Float>(PointLightShadowSize), 0, 0);

        CmdList.BindGraphicsPipelineState(PointLightPipelineState.Get());

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4 Matrix;
            Float      ShadowOffset;
        } ShadowPerObjectBuffer;

        UInt32 PointLightShadowIndex = 0;
        PerShadowMap PerShadowMapData;
        for (Light* Light : Scene.GetLights())
        {
            if (IsSubClassOf<PointLight>(Light))
            {
                PointLight* CurrentLight = Cast<PointLight>(Light);
                for (UInt32 Face = 0; Face < 6; Face++)
                {
                    auto& Cube = LightSetup.PointLightShadowMapDSVs[PointLightShadowIndex];
                    CmdList.ClearDepthStencilView(Cube[Face].Get(), DepthStencilF(1.0f, 0));
                    CmdList.BindRenderTargets(nullptr, 0, Cube[Face].Get());

                    PerShadowMapData.Matrix   = CurrentLight->GetMatrix(Face);
                    PerShadowMapData.Position = CurrentLight->GetPosition();
                    PerShadowMapData.FarPlane = CurrentLight->GetShadowFarPlane();

                    CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

                    CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(PerShadowMap), &PerShadowMapData);

                    CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

                    CmdList.BindConstantBuffers(EShaderStage::Vertex, &PerShadowMapBuffer, 1, 0);

                    // Draw all objects to depthbuffer
                    ConsoleVariable* GlobalFrustumCullEnabled = gConsole.FindVariable("r.EnableFrustumCulling");
                    if (GlobalFrustumCullEnabled->GetBool())
                    {
                        Frustum CameraFrustum = Frustum(
                            CurrentLight->GetShadowFarPlane(), 
                            CurrentLight->GetViewMatrix(Face), 
                            CurrentLight->GetProjectionMatrix(Face));
                        for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
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
                                CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                                CmdList.BindIndexBuffer(Command.IndexBuffer);

                                ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                                ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                                CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &ShadowPerObjectBuffer, 17);

                                CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                        {
                            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.BindIndexBuffer(Command.IndexBuffer);

                            ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                            ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                            CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &ShadowPerObjectBuffer, 17);

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

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource);
}

void ShadowMapRenderer::RenderDirectionalLightShadows(CommandList& CmdList, const SceneLightSetup& LightSetup, const Scene& Scene)
{
    DirLightFrame++;
    if (DirLightFrame > 6)
    {
        UpdateDirLight = true;
        DirLightFrame = 0;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update DirectionalLightBuffer");

    CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);

    if (UpdateDirLight)
    {
        TRACE_SCOPE("Update Directional LightBuffers");

        CmdList.TransitionBuffer(LightSetup.DirectionalLightBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

        // TODO: Not efficent to loop through all lights twice 
        UInt32 NumDirLights = 0;
        for (Light* Light : Scene.GetLights())
        {
            XMFLOAT3 Color  = Light->GetColor();
            Float Intensity = Light->GetIntensity();
            if (IsSubClassOf<DirectionalLight>(Light) && UpdateDirLight)
            {
                DirectionalLight* CurrentLight = Cast<DirectionalLight>(Light);
                VALIDATE(CurrentLight != nullptr);

                DirectionalLightProperties Properties;
                Properties.Color         = XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
                Properties.ShadowBias    = CurrentLight->GetShadowBias();
                Properties.Direction     = CurrentLight->GetDirection();

                // TODO: Should not be the done in the renderer
                XMFLOAT3 CameraPosition = Scene.GetCamera()->GetPosition();
                XMFLOAT3 CameraForward  = Scene.GetCamera()->GetForward();

                Float Near       = Scene.GetCamera()->GetNearPlane();
                Float DirFrustum = 35.0f;
                XMFLOAT3 LookAt  = CameraPosition + (CameraForward * (DirFrustum + Near));
                CurrentLight->SetLookAt(LookAt);

                Properties.LightMatrix   = CurrentLight->GetMatrix();
                Properties.MaxShadowBias = CurrentLight->GetMaxShadowBias();

                constexpr UInt32 SizeInBytes = sizeof(DirectionalLightProperties);
                CmdList.UpdateBuffer(LightSetup.DirectionalLightBuffer.Get(), NumDirLights * SizeInBytes, SizeInBytes, &Properties);

                NumDirLights++;
            }
        }

        CmdList.TransitionBuffer(LightSetup.DirectionalLightBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update DirectionalLightBuffer");

    CmdList.TransitionTexture(LightSetup.DirLightShadowMaps.Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");

    if (UpdateDirLight)
    {
        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        DepthStencilView* DirLightDSV = LightSetup.DirLightShadowMaps->GetDepthStencilView();
        CmdList.ClearDepthStencilView(DirLightDSV, DepthStencilF(1.0f, 0));

        CmdList.BindRenderTargets(nullptr, 0, DirLightDSV);
        CmdList.BindGraphicsPipelineState(DirLightPipelineState.Get());

        CmdList.BindViewport(static_cast<Float>(LightSetup.ShadowMapWidth), static_cast<Float>(LightSetup.ShadowMapHeight), 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.BindScissorRect(LightSetup.ShadowMapWidth, LightSetup.ShadowMapHeight, 0, 0);

        CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4    Matrix;
            Float        ShadowOffset;
        } ShadowPerObjectBuffer;

        PerShadowMap PerShadowMapData;
        for (Light* Light : Scene.GetLights())
        {
            if (IsSubClassOf<DirectionalLight>(Light))
            {
                DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
                PerShadowMapData.Matrix    = DirLight->GetMatrix();
                PerShadowMapData.Position  = DirLight->GetShadowMapPosition();
                PerShadowMapData.FarPlane  = DirLight->GetShadowFarPlane();

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

                CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(PerShadowMap), &PerShadowMapData);

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

                CmdList.BindConstantBuffers(EShaderStage::Vertex, &PerShadowMapBuffer, 1, 0);

                // Draw all objects to depthbuffer
                for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                {
                    CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                    CmdList.BindIndexBuffer(Command.IndexBuffer);

                    ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                    ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                    CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &ShadowPerObjectBuffer, 17);

                    CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                }

                break;
            }
        }

        UpdateDirLight = false;
    }
    else
    {
        CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");

    CmdList.TransitionTexture(LightSetup.DirLightShadowMaps.Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource);
}

void ShadowMapRenderer::Release()
{
    PerShadowMapBuffer.Reset();
    DirLightPipelineState.Reset();
    PointLightPipelineState.Reset();
}

Bool ShadowMapRenderer::CreateShadowMaps(SceneLightSetup& LightSetup)
{
    LightSetup.PointLightShadowMaps = RenderLayer::CreateTextureCubeArray(
        LightSetup.ShadowMapFormat, 
        LightSetup.PointLightShadowSize, 
        1, LightSetup.MaxPointLightShadows, 
        TextureFlags_ShadowMap, 
        EResourceState::PixelShaderResource,
        nullptr);
    if (LightSetup.PointLightShadowMaps)
    {
        LightSetup.PointLightShadowMaps->SetName("PointLight ShadowMaps");

        LightSetup.PointLightShadowMapDSVs.Resize(LightSetup.MaxPointLightShadows);
        for (UInt32 i = 0; i < LightSetup.MaxPointLightShadows; i++)
        {
            for (UInt32 Face = 0; Face < 6; Face++)
            {
                TStaticArray<TSharedRef<DepthStencilView>, 6>& DepthCube = LightSetup.PointLightShadowMapDSVs[i];
                DepthCube[Face] = RenderLayer::CreateDepthStencilView(
                    LightSetup.PointLightShadowMaps.Get(), 
                    LightSetup.ShadowMapFormat, 
                    GetCubeFaceFromIndex(Face), 0, i);
                if (!DepthCube[Face])
                {
                    Debug::DebugBreak();
                    return false;
                }
            }
        }
    }
    else
    {
        return false;
    }

    LightSetup.DirLightShadowMaps = RenderLayer::CreateTexture2D(
        LightSetup.ShadowMapFormat,
        LightSetup.ShadowMapWidth,
        LightSetup.ShadowMapHeight,
        1, 1, TextureFlags_ShadowMap,
        EResourceState::PixelShaderResource,
        nullptr);
    if (LightSetup.DirLightShadowMaps)
    {
        LightSetup.DirLightShadowMaps->SetName("Directional Light ShadowMaps");
    }
    else
    {
        return false;
    }

    return true;
}
