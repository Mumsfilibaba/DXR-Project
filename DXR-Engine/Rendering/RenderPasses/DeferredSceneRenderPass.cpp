#include "DeferredSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"
#include "RenderLayer/Viewport.h"
#include "RenderLayer/PipelineState.h"

#include "Debug/Profiler.h"

#include "Rendering/Material.h"
#include "Rendering/Mesh.h"

Bool DeferredSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    if (!CreateGBuffer(FrameResources))
    {
        return false;
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressV = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressW = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.Filter = ESamplerFilter::SamplerFilter_MinMagMipPoint;

        FrameResources.GBufferSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!FrameResources.GBufferSampler)
        {
            return false;
        }
    }

    TArray<UInt8> ShaderCode;
    {
        TArray<ShaderDefine> Defines =
        {
            { "ENABLE_PARALLAX_MAPPING", "1" },
            { "ENABLE_NORMAL_MAPPING",   "1" },
        };

        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/GeometryPass.hlsl",
            "VSMain",
            &Defines,
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
            VShader->SetName("GeometryPass VertexShader");
        }

        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/GeometryPass.hlsl",
            "PSMain",
            &Defines,
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
            PShader->SetName("GeometryPass PixelShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = EComparisonFunc::ComparisonFunc_LessEqual;
        DepthStencilStateInfo.DepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

        TSharedRef<DepthStencilState> GeometryDepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
        if (!GeometryDepthStencilState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            GeometryDepthStencilState->SetName("GeometryPass DepthStencilState");
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

        TSharedRef<RasterizerState> GeometryRasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
        if (!GeometryRasterizerState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            GeometryRasterizerState->SetName("GeometryPass RasterizerState");
        }

        BlendStateCreateInfo BlendStateInfo;
        BlendStateInfo.IndependentBlendEnable = false;
        BlendStateInfo.RenderTarget[0].BlendEnable = false;

        TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("GeometryPass BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState                       = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState                             = BlendState.Get();
        PipelineStateInfo.DepthStencilState                      = GeometryDepthStencilState.Get();
        PipelineStateInfo.RasterizerState                        = GeometryRasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader               = VShader.Get();
        PipelineStateInfo.ShaderState.PixelShader                = PShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::Format_R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::Format_R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PipelineStateInfo.PipelineFormats.NumRenderTargets       = 4;

        PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!PipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PipelineState->SetName("GeometryPass PipelineState");
        }
    }

    // PrePass
    {
        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/PrePass.hlsl",
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
            VShader->SetName("PrePass VertexShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_Less;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

        TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
        if (!DepthStencilState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName("Prepass DepthStencilState");
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

        TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName("Prepass RasterizerState");
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
            BlendState->SetName("Prepass BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState                   = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState                         = BlendState.Get();
        PipelineStateInfo.DepthStencilState                  = DepthStencilState.Get();
        PipelineStateInfo.RasterizerState                    = RasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader           = VShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!PrePassPipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PrePassPipelineState->SetName("PrePass PipelineState");
        }
    }

    return true;
}

Bool DeferredSceneRenderPass::ResizeResources(SharedRenderPassResources& FrameResources)
{
    return CreateGBuffer(FrameResources);
}

void DeferredSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
    // Transition FrameResources.GBuffer
    CmdList.TransitionTexture(
        FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    CmdList.TransitionTexture(
        FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);

    CmdList.TransitionTexture(
        FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_RenderTarget);

    // Clear FrameResources.GBuffer
    ColorClearValue BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearDepthStencilView(FrameResources.GBufferDSV.Get(), DepthStencilClearValue(1.0f, 0));

    // Setup view
    const UInt32 RenderWidth  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 RenderHeight = FrameResources.MainWindowViewport->GetHeight();

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
        CmdList.BindRenderTargets(nullptr, 0, FrameResources.GBufferDSV.Get());

        CmdList.BindGraphicsPipelineState(PrePassPipelineState.Get());

        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Vertex,
            FrameResources.CameraBuffer.GetAddressOf(),
            1, 0);

        // Draw all objects to depthbuffer
        for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
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

    // Render all objects to the FrameResources.GBuffer
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

    {
        TRACE_SCOPE("GeometryPass");

        RenderTargetView* RenderTargets[] =
        {
            FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(),
            FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(),
            FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        };
        CmdList.BindRenderTargets(RenderTargets, 4, FrameResources.GBufferDSV.Get());

        // Setup Pipeline
        CmdList.BindGraphicsPipelineState(PipelineState.Get());

        struct TransformBuffer
        {
            XMFLOAT4X4 Transform;
            XMFLOAT4X4 TransformInv;
        } TransformPerObject;

        for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
        {
            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.BindIndexBuffer(Command.IndexBuffer);

            if (Command.Material->IsBufferDirty())
            {
                Command.Material->BuildBuffer(CmdList);
            }

            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Vertex,
                &FrameResources.CameraBuffer,
                1, 0);

            ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
            CmdList.BindConstantBuffers(
                EShaderStage::ShaderStage_Pixel,
                &MaterialBuffer,
                1, 1);

            TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
            TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

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

        // Setup FrameResources.GBuffer for Read
        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            EResourceState::ResourceState_RenderTarget,
            EResourceState::ResourceState_NonPixelShaderResource);

        FrameResources.DebugTextures.EmplaceBack(
            FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX],
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
            EResourceState::ResourceState_RenderTarget,
            EResourceState::ResourceState_NonPixelShaderResource);

        FrameResources.DebugTextures.EmplaceBack(
            FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX],
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            EResourceState::ResourceState_RenderTarget,
            EResourceState::ResourceState_NonPixelShaderResource);

        FrameResources.DebugTextures.EmplaceBack(
            FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX],
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
            EResourceState::ResourceState_RenderTarget,
            EResourceState::ResourceState_NonPixelShaderResource);

        FrameResources.DebugTextures.EmplaceBack(
            FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX],
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX],
            EResourceState::ResourceState_NonPixelShaderResource,
            EResourceState::ResourceState_NonPixelShaderResource);

        CmdList.TransitionTexture(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
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
}

Bool DeferredSceneRenderPass::CreateGBuffer(SharedRenderPassResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RenderTarget;

    FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.AlbedoFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX])
    {
        FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("FrameResources.FrameResources.GBuffer Albedo");

        FrameResources.FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.AlbedoFormat, 0, 1);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.AlbedoFormat, 0);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Normal
    FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.NormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX])
    {
        FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName("FrameResources.FrameResources.GBuffer Normal");

        FrameResources.FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
            FrameResources.NormalFormat,
            0, 1);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
            FrameResources.NormalFormat,
            0);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Material Properties
    FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.MaterialFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX])
    {
        FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("FrameResources.FrameResources.GBuffer Material");

        FrameResources.FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
            FrameResources.MaterialFormat,
            0, 1);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
            FrameResources.MaterialFormat,
            0);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // DepthStencil
    const UInt32 UsageDS = TextureUsage_Default | TextureUsage_DSV | TextureUsage_SRV;
    FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R32_Typeless,
        UsageDS,
        Width,
        Height,
        1, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX])
    {
        FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName("FrameResources.FrameResources.GBuffer DepthStencil");

        FrameResources.FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            EFormat::Format_R32_Float,
            0,
            1);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX])
        {
            return false;
        }

        FrameResources.GBufferDSV = RenderLayer::CreateDepthStencilView(
            FrameResources.FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            FrameResources.DepthBufferFormat,
            0);
        if (!FrameResources.GBufferDSV)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // View Normal
    FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.ViewNormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX])
    {
        FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName("FrameResources.FrameResources.GBuffer View Normal");

        FrameResources.FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            FrameResources.ViewNormalFormat,
            0, 1);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            FrameResources.ViewNormalFormat,
            0);
        if (!FrameResources.FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}
