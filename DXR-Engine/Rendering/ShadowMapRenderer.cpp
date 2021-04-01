#include "ShadowMapRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console/Console.h"

#include "Rendering/Resources/Mesh.h"

#include "Rendering/MeshDrawCommand.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

struct PerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    float      FarPlane;
};

bool ShadowMapRenderer::Init(LightSetup& LightSetup, FrameResources& FrameResources)
{
    if (!CreateShadowMaps(LightSetup))
    {
        return false;
    }

    PerShadowMapBuffer = CreateConstantBuffer<PerShadowMap>(BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
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
    TArray<uint8> ShaderCode;
    {
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        PointLightVertexShader = CreateVertexShader(ShaderCode);
        if (!PointLightVertexShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightVertexShader->SetName("Linear ShadowMap VertexShader");
        }

        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        PointLightPixelShader = CreatePixelShader(ShaderCode);
        if (!PointLightPixelShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPixelShader->SetName("Linear ShadowMap PixelShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
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

        TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
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

        TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
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
        PipelineStateInfo.ShaderState.VertexShader           = PointLightVertexShader.Get();
        PipelineStateInfo.ShaderState.PixelShader            = PointLightPixelShader.Get();
        PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        PointLightPipelineState = CreateGraphicsPipelineState(PipelineStateInfo);
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
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/ShadowMap.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        DirLightShader = CreateVertexShader(ShaderCode);
        if (!DirLightShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirLightShader->SetName("ShadowMap VertexShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
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

        TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
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
        TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
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
        PipelineStateInfo.ShaderState.VertexShader           = DirLightShader.Get();
        PipelineStateInfo.ShaderState.PixelShader            = nullptr;
        PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirLightPipelineState = CreateGraphicsPipelineState(PipelineStateInfo);
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

void ShadowMapRenderer::RenderPointLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene)
{
    PointLightFrame++;
    if (PointLightFrame > 6)
    {
        UpdatePointLight = true;
        PointLightFrame = 0;
    }

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");

    if (UpdatePointLight)
    {
        TRACE_SCOPE("Render PointLight ShadowMaps");

        const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
        CmdList.SetViewport(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.SetScissorRect(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);

        CmdList.SetGraphicsPipelineState(PointLightPipelineState.Get());

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4 Matrix;
            float      ShadowOffset;
        } ShadowPerObjectBuffer;

        PerShadowMap PerShadowMapData;
        for (uint32 i = 0; i < LightSetup.PointLightShadowMapsGenerationData.Size(); i++)
        {
            for (uint32 Face = 0; Face < 6; Face++)
            {
                auto& Cube = LightSetup.PointLightShadowMapDSVs[i];
                CmdList.ClearDepthStencilView(Cube[Face].Get(), DepthStencilF(1.0f, 0));
                CmdList.SetRenderTargets(nullptr, 0, Cube[Face].Get());

                auto& Data = LightSetup.PointLightShadowMapsGenerationData[i];
                PerShadowMapData.Matrix   = Data.Matrix[Face];
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

                CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(PerShadowMap), &PerShadowMapData);

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

                CmdList.SetConstantBuffer(PointLightVertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CmdList.SetConstantBuffer(PointLightPixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                // Draw all objects to depthbuffer
                ConsoleVariable* GlobalFrustumCullEnabled = GConsole.FindVariable("r.EnableFrustumCulling");
                if (GlobalFrustumCullEnabled->GetBool())
                {
                    Frustum CameraFrustum = Frustum(Data.FarPlane, Data.ViewMatrix[Face], Data.ProjMatrix[Face]);
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
                            CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.SetIndexBuffer(Command.IndexBuffer);

                            ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                            ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                            CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 17);

                            CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
                        }
                    }
                }
                else
                {
                    for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                        CmdList.SetIndexBuffer(Command.IndexBuffer);

                        ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                        ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                        CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 17);

                        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
                    }
                }
            }
        }

        UpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource);
}

void ShadowMapRenderer::RenderDirectionalLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene)
{
    //DirLightFrame++;
    //if (DirLightFrame > 6)
    //{
    //    UpdateDirLight = true;
    //    DirLightFrame = 0;
    //}

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update DirectionalLightBuffer");

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");

    //if (UpdateDirLight)
    //{
        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        CmdList.TransitionTexture(LightSetup.DirLightShadowMaps.Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite);

        DepthStencilView* DirLightDSV = LightSetup.DirLightShadowMaps->GetDepthStencilView();
        CmdList.ClearDepthStencilView(DirLightDSV, DepthStencilF(1.0f, 0));

        CmdList.SetRenderTargets(nullptr, 0, DirLightDSV);
        CmdList.SetGraphicsPipelineState(DirLightPipelineState.Get());

        CmdList.SetViewport(static_cast<float>(LightSetup.ShadowMapWidth), static_cast<float>(LightSetup.ShadowMapHeight), 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.SetScissorRect(LightSetup.ShadowMapWidth, LightSetup.ShadowMapHeight, 0, 0);

        CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4 Matrix;
            float      ShadowOffset;
        } ShadowPerObjectBuffer;

        PerShadowMap PerShadowMapData;
        for (uint32 i = 0; i < LightSetup.DirLightShadowMapsGenerationData.Size(); i++)
        {
            auto& Data = LightSetup.DirLightShadowMapsGenerationData[i];
            PerShadowMapData.Matrix   = Data.Matrix;
            PerShadowMapData.Position = Data.Position;
            PerShadowMapData.FarPlane = Data.FarPlane;

            CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);

            CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(PerShadowMap), &PerShadowMapData);

            CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

            CmdList.SetConstantBuffers(DirLightShader.Get(), &PerShadowMapBuffer, 1, 0);

            // Draw all objects to depthbuffer
            for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
            {
                CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                CmdList.SetIndexBuffer(Command.IndexBuffer);

                ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                CmdList.Set32BitShaderConstants(DirLightShader.Get(), &ShadowPerObjectBuffer, 17);

                CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
            }
        }

    //    UpdateDirLight = false;
    //}
    //else
    //{
    //    CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);
    //}

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");

    CmdList.TransitionTexture(LightSetup.DirLightShadowMaps.Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource);
}

void ShadowMapRenderer::Release()
{
    PerShadowMapBuffer.Reset();
    DirLightPipelineState.Reset();
    PointLightPipelineState.Reset();
    DirLightShader.Reset();
    PointLightVertexShader.Reset();
    PointLightPixelShader.Reset();
}

bool ShadowMapRenderer::CreateShadowMaps(LightSetup& LightSetup)
{
    LightSetup.PointLightShadowMaps = CreateTextureCubeArray(
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
        for (uint32 i = 0; i < LightSetup.MaxPointLightShadows; i++)
        {
            for (uint32 Face = 0; Face < 6; Face++)
            {
                TStaticArray<TRef<DepthStencilView>, 6>& DepthCube = LightSetup.PointLightShadowMapDSVs[i];
                DepthCube[Face] = CreateDepthStencilView(
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

    LightSetup.DirLightShadowMaps = CreateTexture2D(
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
