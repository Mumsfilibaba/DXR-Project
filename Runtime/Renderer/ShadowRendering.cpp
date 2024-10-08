#include "ShadowRendering.h"
#include "Scene.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "Renderer/Debug/GPUProfiler.h"

static TAutoConsoleVariable<bool> CVarCSMDebugCascades(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableGeometryShaderInstancing(
    "Renderer.CSM.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableViewInstancing(
    "Renderer.CSM.EnableViewInstancing",
    "Enables view-instancing for Cascade rendering, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightsEnableGeometryShaderInstancing(
    "Renderer.PointLights.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cube-map drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightsEnableViewInstancing(
    "Renderer.PointLights.EnableViewInstancing",
    "Enables view-instancing for cube-map rendering, enabling two-pass cube-map drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterFunction(
    "Renderer.CSM.FilterFunction",
    "Select function to use to filer Cascaded Shadow Maps. 0: Grid PCF 1: Poisson Disc PCF",
    1,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterSize(
    "Renderer.CSM.FilterSize",
    "Size of the filter for the Cascaded Shadow Maps",
    196,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMMaxFilterSize(
    "Renderer.CSM.MaxFilterSize",
    "Maximum size of the filter for the Cascaded Shadow Maps",
    512,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarNumPoissonDiscSamples(
    "Renderer.CSM.NumPoissonDiscSamples",
    "Number Poisson Samples to use when sampling the Cascaded Shadow Maps using a Poisson Disc",
    128,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMRotateSamples(
    "Renderer.CSM.RotateSamples",
    "Rotate Poisson samples before using them to sample the Cascades",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMBlendCascades(
    "Renderer.CSM.BlendCascades",
    "Blend between cascades",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMSelectCascadeFromProjection(
    "Renderer.CSM.SelectCascadeFromProjection",
    "Select what cascade to use based on projection",
    true,
    EConsoleVariableFlags::Default);

FPointLightRenderPass::FPointLightRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerShadowMapBuffer(nullptr)
    , SinglePassShadowMapBuffer(nullptr)
{
}

FPointLightRenderPass::~FPointLightRenderPass()
{
    MaterialPSOs.Clear();
    PerShadowMapBuffer.Reset();
    SinglePassShadowMapBuffer.Reset();
    TwoPassShadowMapBuffer.Reset();
}

void FPointLightRenderPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FGraphicsPipelineStateInstance* CachedPointLightPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedPointLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");
        }

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");
        }

        if (MaterialFlags & MaterialFlag_EnableAlpha)
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");
        }

        const bool bUseViewInstancing = GRHISupportsViewInstancing && GRHIMaxViewInstanceCount >= 4 && CVarPointLightsEnableViewInstancing.GetValue();
        const bool bUseGeometryShaders = !bUseViewInstancing && GRHISupportsGeometryShaders && CVarPointLightsEnableGeometryShaderInstancing.GetValue();
        if (bUseViewInstancing)
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VIEW_INSTANCING", "(1)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(0)");
        }
        else if (bUseGeometryShaders)
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VIEW_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VIEW_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(0)");
        }

        FShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        if (bUseGeometryShaders)
        {
            CompileInfo = FShaderCompileInfo("Point_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }

            NewPipelineStateInstance.GeometryShader = RHICreateGeometryShader(ShaderCode);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        CompileInfo = FShaderCompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        // Initialize standard input layout
        FRHIVertexInputLayoutInitializer InputLayoutInitializer;
        if (MaterialFlags & (MaterialFlag_EnableHeight | MaterialFlag_EnableAlpha))
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexMasked), 0, 0,  EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexMasked), 0, 12, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
        if (!NewPipelineStateInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = NewPipelineStateInstance.BlendState.Get();
        PSOInitializer.DepthStencilState                  = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable            = false;
        PSOInitializer.VertexInputLayout                  = NewPipelineStateInstance.InputLayout.Get();
        PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
        PSOInitializer.RasterizerState                    = NewPipelineStateInstance.RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = FRHIGraphicsPipelineStateInitializer::DefaultSampleMask;
        PSOInitializer.ShaderState.VertexShader           = NewPipelineStateInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = NewPipelineStateInstance.PixelShader.Get();
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.ShadowMapFormat;

        if (bUseViewInstancing)
        {
            PSOInitializer.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
            PSOInitializer.ViewInstancingInfo.NumArraySlices = RHI_NUM_CUBE_FACES / 2;
        }
        else if (bUseGeometryShaders)
        {
            PSOInitializer.ShaderState.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("Point ShadowMap PipelineState %d", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }
}

bool FPointLightRenderPass::Initialize(FFrameResources& Resources)
{
    FRHIBufferInfo PerShadowMapBufferInfo(sizeof(FPerShadowMapHLSL), sizeof(FPerShadowMapHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    PerShadowMapBuffer = RHICreateBuffer(PerShadowMapBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!PerShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetDebugName("Per ShadowMap Buffer");
    }

    FRHIBufferInfo SinglePassShadowMapBufferInfo(sizeof(FSinglePassPointLightBufferHLSL), sizeof(FSinglePassPointLightBufferHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    SinglePassShadowMapBuffer = RHICreateBuffer(SinglePassShadowMapBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!SinglePassShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SinglePassShadowMapBuffer->SetDebugName("Single-Pass ShadowMap Buffer");
    }

    FRHIBufferInfo TwoPassShadowMapBufferInfo(sizeof(FTwoPassPointLightBufferHLSL), sizeof(FTwoPassPointLightBufferHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    TwoPassShadowMapBuffer = RHICreateBuffer(TwoPassShadowMapBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!TwoPassShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TwoPassShadowMapBuffer->SetDebugName("Two-Pass ShadowMap Buffer");
    }

    return CreateResources(Resources);
}

bool FPointLightRenderPass::CreateResources(FFrameResources& Resources)
{
    const FClearValue DepthClearValue(Resources.ShadowMapFormat, 1.0f, 0);

    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo PointLightInfo = FRHITextureInfo::CreateTextureCubeArray(Resources.ShadowMapFormat, Resources.PointLightShadowSize, Resources.MaxPointLightShadows, 1, 1, Flags, DepthClearValue);
    Resources.PointLightShadowMaps = RHICreateTexture(PointLightInfo, EResourceAccess::PixelShaderResource);
    if (Resources.PointLightShadowMaps)
    {
        Resources.PointLightShadowMaps->SetDebugName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    return true;
}

void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    // PerObject Structs
    struct FShadowPerObject
    {
        FMatrix4 Matrix;
    } ShadowPerObjectBuffer;

    const bool bUseViewInstancing = GRHISupportsViewInstancing && GRHIMaxViewInstanceCount >= 4 && CVarPointLightsEnableViewInstancing.GetValue();
    const bool bUseGeometryShaders = !bUseViewInstancing && GRHISupportsGeometryShaders && CVarPointLightsEnableGeometryShaderInstancing.GetValue();
    if (bUseViewInstancing)
    {
        FTwoPassPointLightBufferHLSL TwoPassPointLightBuffer;
        for (int32 LightIndex = 0; LightIndex < Scene->PointLights.Size(); ++LightIndex)
        {
            FScenePointLight* ScenePointLight = Scene->PointLights[LightIndex];

            constexpr uint32 FacesPerPass = RHI_NUM_CUBE_FACES / 2;
            for (int32 PassIndex = 0, FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex += FacesPerPass, PassIndex++)
            {
                FScenePointLight::FShadowData& Data0 = ScenePointLight->ShadowData[FaceIndex + 0];
                TwoPassPointLightBuffer.LightProjections[0] = Data0.Matrix;

                FScenePointLight::FShadowData& Data1 = ScenePointLight->ShadowData[FaceIndex + 1];
                TwoPassPointLightBuffer.LightProjections[1] = Data1.Matrix;
                
                FScenePointLight::FShadowData& Data2 = ScenePointLight->ShadowData[FaceIndex + 2];
                TwoPassPointLightBuffer.LightProjections[2] = Data2.Matrix;
                TwoPassPointLightBuffer.LightPosition       = Data0.Position;
                TwoPassPointLightBuffer.LightFarPlane       = Data0.FarPlane;

                CommandList.TransitionBuffer(TwoPassShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
                CommandList.UpdateBuffer(TwoPassShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FTwoPassPointLightBufferHLSL)), &TwoPassPointLightBuffer);
                CommandList.TransitionBuffer(TwoPassShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

                FRHIBeginRenderPassInfo RenderPass;
                RenderPass.DepthStencilView                               = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get());
                RenderPass.DepthStencilView.ArrayIndex                    = static_cast<uint16>(LightIndex * RHI_NUM_CUBE_FACES + FaceIndex);
                RenderPass.DepthStencilView.NumArraySlices                = FacesPerPass;
                RenderPass.DepthStencilView.LoadAction                    = EAttachmentLoadAction::Clear;
                RenderPass.DepthStencilView.ClearValue                    = FDepthStencilValue(1.0f, 0);
                RenderPass.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
                RenderPass.ViewInstancingInfo.NumArraySlices              = FacesPerPass;

                CommandList.BeginRenderPass(RenderPass);

                const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
                FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                for (const FMeshBatch& Batch : ScenePointLight->TwoPassMeshBatches[PassIndex])
                {
                    FMaterial* Material = Batch.Material;
                    FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                    if (!Instance)
                    {
                        DEBUG_BREAK();
                    }

                    FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                    CHECK(PipelineState  != nullptr);

                    CommandList.SetGraphicsPipelineState(PipelineState);
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), TwoPassShadowMapBuffer.Get(), 0);
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), TwoPassShadowMapBuffer.Get(), 0);

                    if (Material->HasAlphaMask())
                    {
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                    }
                    if (Material->HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                    }

                    for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                    {
                        FProxySceneComponent* Component = MeshReference.Primitive;
                        if (Material->HasHeightMap() || Material->HasAlphaMask())
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                        }
                        else
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                        }

                        CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                        ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                        CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                    }
                }

                CommandList.EndRenderPass();
            }
        }
    }
    else if (bUseGeometryShaders)
    {
        FSinglePassPointLightBufferHLSL SinglePassPointLightBuffer;
        for (int32 LightIndex = 0; LightIndex < Scene->PointLights.Size(); ++LightIndex)
        {
            FScenePointLight* ScenePointLight = Scene->PointLights[LightIndex];
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
                SinglePassPointLightBuffer.LightProjections[FaceIndex] = Data.Matrix;
                SinglePassPointLightBuffer.LightPosition               = Data.Position;
                SinglePassPointLightBuffer.LightFarPlane               = Data.FarPlane;
            }

            CommandList.TransitionBuffer(SinglePassShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(SinglePassShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FSinglePassPointLightBufferHLSL)), &SinglePassPointLightBuffer);
            CommandList.TransitionBuffer(SinglePassShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            FRHIBeginRenderPassInfo RenderPass;
            RenderPass.DepthStencilView                = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get());
            RenderPass.DepthStencilView.ArrayIndex     = static_cast<uint16>(LightIndex * RHI_NUM_CUBE_FACES);
            RenderPass.DepthStencilView.NumArraySlices = RHI_NUM_CUBE_FACES;
            RenderPass.DepthStencilView.LoadAction     = EAttachmentLoadAction::Clear;
            RenderPass.DepthStencilView.ClearValue     = FDepthStencilValue(1.0f, 0);

            CommandList.BeginRenderPass(RenderPass);

            const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
            FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : ScenePointLight->SinglePassMeshBatch)
            {
                FMaterial* Material = Batch.Material;

                FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState  != nullptr);

                CommandList.SetGraphicsPipelineState(PipelineState);
                CommandList.SetConstantBuffer(Instance->GeometryShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                CommandList.SetConstantBuffer(Instance->PixelShader.Get(), SinglePassShadowMapBuffer.Get(), 0);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                {
                    FProxySceneComponent* Component = MeshReference.Primitive;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                    }
                    else
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
    }
    else
    {
        FPerShadowMapHLSL PerShadowMapData;
        for (int32 LightIndex = 0; LightIndex < Scene->PointLights.Size(); ++LightIndex)
        {
            FScenePointLight* ScenePointLight = Scene->PointLights[LightIndex];
            for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
            {
                FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
                PerShadowMapData.Matrix   = Data.Matrix;
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
                CommandList.UpdateBuffer(PerShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FPerShadowMapHLSL)), &PerShadowMapData);
                CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

                const uint32 ArrayIndex = (LightIndex * RHI_NUM_CUBE_FACES) + FaceIndex;
                FRHIBeginRenderPassInfo RenderPass;
                RenderPass.DepthStencilView            = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);
                RenderPass.DepthStencilView.LoadAction = EAttachmentLoadAction::Clear;
                RenderPass.DepthStencilView.ClearValue = FDepthStencilValue(1.0f, 0);

                CommandList.BeginRenderPass(RenderPass);

                const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
                FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                for (const FMeshBatch& Batch : ScenePointLight->MeshBatches[FaceIndex])
                {
                    FMaterial* Material = Batch.Material;

                    FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                    if (!Instance)
                    {
                        DEBUG_BREAK();
                    }

                    FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                    CHECK(PipelineState  != nullptr);

                    CommandList.SetGraphicsPipelineState(PipelineState);
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                    if (Material->HasAlphaMask())
                    {
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                    }
                    if (Material->HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                    }

                    for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                    {
                        FProxySceneComponent* Component = MeshReference.Primitive;
                        if (Material->HasHeightMap() || Material->HasAlphaMask())
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                        }
                        else
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                        }

                        CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                        ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                        CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                    }
                }

                CommandList.EndRenderPass();
            }
        }
    }

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");
}

FCascadeGenerationPass::FCascadeGenerationPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CascadeGen()
    , CascadeGenShader()
{
}

FCascadeGenerationPass::~FCascadeGenerationPass()
{
    CascadeGen.Reset();
    CascadeGenShader.Reset();
}

bool FCascadeGenerationPass::Initialize(FFrameResources& Resources)
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/CascadeMatrixGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    CascadeGenShader = RHICreateComputeShader(ShaderCode);
    if (!CascadeGenShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer CascadePSO;
    CascadePSO.Shader = CascadeGenShader.Get();

    CascadeGen = RHICreateComputePipelineState(CascadePSO);
    if (!CascadeGen)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CascadeGen->SetDebugName("CascadeGen PSO");
    }

    FRHIBufferInfo CascadeMatrixBufferInfo(sizeof(FCascadeMatricesHLSL) * NUM_SHADOW_CASCADES, sizeof(FCascadeMatricesHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
    Resources.CascadeMatrixBuffer = RHICreateBuffer(CascadeMatrixBufferInfo, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeMatrixBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
    }

    FRHIBufferSRVDesc SRVInitializer(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferSRV = RHICreateShaderResourceView(SRVInitializer);
    if (!Resources.CascadeMatrixBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferUAVDesc UAVInitializer(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Resources.CascadeMatrixBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferInfo CascadeSplitsBufferInfo(sizeof(FCascadeSplitHLSL) * NUM_SHADOW_CASCADES, sizeof(FCascadeSplitHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
    Resources.CascadeSplitsBuffer = RHICreateBuffer(CascadeSplitsBufferInfo, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeSplitsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
    }

    SRVInitializer = FRHIBufferSRVDesc(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferSRV = RHICreateShaderResourceView(SRVInitializer);
    if (!Resources.CascadeSplitsBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    UAVInitializer = FRHIBufferUAVDesc(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Resources.CascadeSplitsBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FCascadeGenerationPass::Execute(FRHICommandList& CommandList, FFrameResources& Resources)
{
    GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

    CommandList.TransitionBuffer(Resources.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
    CommandList.TransitionBuffer(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(CascadeGen.Get());

    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 1);

    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeMatrixBufferUAV.Get(), 0);
    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeSplitsBufferUAV.Get(), 1);

    CommandList.SetShaderResourceView(CascadeGenShader.Get(), Resources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

    CommandList.Dispatch(1, 1, 1);

    CommandList.TransitionBuffer(Resources.CascadeMatrixBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionBuffer(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
}

FCascadedShadowsRenderPass::FCascadedShadowsRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerCascadeBuffer(nullptr)
{
}

FCascadedShadowsRenderPass::~FCascadedShadowsRenderPass()
{
    MaterialPSOs.Clear();
    PerCascadeBuffer.Reset();
}

void FCascadedShadowsRenderPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    // Cascaded shadow map
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FGraphicsPipelineStateInstance* CachedDirectionalLightPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedDirectionalLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");
        }

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");
        }

        if (MaterialFlags & MaterialFlag_EnableAlpha)
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        }
        else
        {            
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");
        }

        const bool bUseViewInstancing = GRHISupportsViewInstancing && GRHIMaxViewInstanceCount >= 4 && CVarCSMEnableViewInstancing.GetValue();
        const bool bUseGeometryShaders = !bUseViewInstancing && GRHISupportsGeometryShaders && CVarCSMEnableGeometryShaderInstancing.GetValue();
        if (bUseViewInstancing)
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(1)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(0)");
        }
        else if (bUseGeometryShaders)
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(0)");
        }

        FShaderCompileInfo CompileInfo("Cascade_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        if (bUseGeometryShaders)
        {
            CompileInfo = FShaderCompileInfo("Cascade_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }

            NewPipelineStateInstance.GeometryShader = RHICreateGeometryShader(ShaderCode);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        constexpr EMaterialFlags PSFlags = MaterialFlag_EnableHeight | MaterialFlag_PackedDiffuseAlpha | MaterialFlag_EnableAlpha;
        const bool bWantPixelShader = (MaterialFlags & PSFlags) != MaterialFlag_None;
        if (bWantPixelShader)
        {
            CompileInfo = FShaderCompileInfo("Cascade_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }

            NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
            if (!NewPipelineStateInstance.PixelShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        // Initialize standard input layout
        FRHIVertexInputLayoutInitializer InputLayoutInitializer;
        if (MaterialFlags & (MaterialFlag_EnableHeight | MaterialFlag_EnableAlpha))
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexMasked), 0, 0,  EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexMasked), 0, 12, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.bEnableDepthBias     = true;
        RasterizerStateInitializer.DepthBias            = 1.0f;
        RasterizerStateInitializer.DepthBiasClamp       = 0.05f;
        RasterizerStateInitializer.SlopeScaledDepthBias = 1.0f;

        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState               = NewPipelineStateInstance.BlendState.Get();
        PSOInitializer.DepthStencilState        = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable  = false;
        PSOInitializer.VertexInputLayout        = NewPipelineStateInstance.InputLayout.Get();
        PSOInitializer.PrimitiveTopology        = EPrimitiveTopology::TriangleList;
        PSOInitializer.RasterizerState          = NewPipelineStateInstance.RasterizerState.Get();
        PSOInitializer.SampleCount              = 1;
        PSOInitializer.SampleQuality            = 0;
        PSOInitializer.SampleMask               = FRHIGraphicsPipelineStateInitializer::DefaultSampleMask;
        PSOInitializer.ShaderState.VertexShader = NewPipelineStateInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader  = NewPipelineStateInstance.PixelShader.Get();

        if (bUseViewInstancing)
        {
            PSOInitializer.ViewInstancingInfo.NumArraySlices = NUM_SHADOW_CASCADES;
            PSOInitializer.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
        }
        else if (bUseGeometryShaders)
        {
            PSOInitializer.ShaderState.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.ShadowMapFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets = 0;

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("CSM PipelineState %d", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }
}

bool FCascadedShadowsRenderPass::Initialize(FFrameResources& Resources)
{
    FRHIBufferInfo PerCascadeBufferInfo(sizeof(FPerCascadeHLSL), sizeof(FPerCascadeHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    PerCascadeBuffer = RHICreateBuffer(PerCascadeBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!PerCascadeBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PerCascadeBuffer->SetDebugName("Per Cascade Buffer");
    }

    return CreateResources(Resources);
}

bool FCascadedShadowsRenderPass::CreateResources(FFrameResources& Resources)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;

    const FClearValue DepthClearValue(Resources.ShadowMapFormat, 1.0f, 0);
    FRHITextureInfo CascadeInfo = FRHITextureInfo::CreateTexture2DArray(Resources.ShadowMapFormat, Resources.CascadeSize, Resources.CascadeSize, NUM_SHADOW_CASCADES, 1, 1, Flags, DepthClearValue);
    Resources.ShadowMapCascades = RHICreateTexture(CascadeInfo, EResourceAccess::NonPixelShaderResource);
    if (Resources.ShadowMapCascades)
    {
        const FString DebugName = FString::CreateFormatted("Shadow Map Cascades");
        Resources.ShadowMapCascades->SetDebugName(DebugName);
    }
    else
    {
        return false;
    }

    return true;
}

void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render DirectionalLight ShadowMaps");

    TRACE_SCOPE("Render DirectionalLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

    CommandList.TransitionTexture(Resources.ShadowMapCascades.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

    // PerObject Structs
    struct FShadowPerObject
    {
        FMatrix4 Matrix;
    } ShadowPerObjectBuffer;

    if (Scene->DirectionalLight)
    {
        const bool bUseViewInstancing = GRHISupportsViewInstancing && GRHIMaxViewInstanceCount >= 4 && CVarCSMEnableViewInstancing.GetValue();
        const bool bUseGeometryShaders = !bUseViewInstancing && GRHISupportsGeometryShaders && CVarCSMEnableGeometryShaderInstancing.GetValue();
        if (bUseViewInstancing)
        {
            FRHIBeginRenderPassInfo RenderPass;
            RenderPass.DepthStencilView                               = FRHIDepthStencilView(Resources.ShadowMapCascades.Get());
            RenderPass.DepthStencilView.ArrayIndex                    = 0;
            RenderPass.DepthStencilView.NumArraySlices                = NUM_SHADOW_CASCADES;
            RenderPass.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
            RenderPass.ViewInstancingInfo.NumArraySlices              = NUM_SHADOW_CASCADES;

            CommandList.BeginRenderPass(RenderPass);

            const float CascadeSize = static_cast<float>(Resources.CascadeSize);
            FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            FSceneDirectionalLight* SceneDirectionalLight = Scene->DirectionalLight;
            for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
            {
                FMaterial* Material = Batch.Material;

                FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);

                CommandList.SetGraphicsPipelineState(PipelineState);
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                {
                    FProxySceneComponent* Component = MeshReference.Primitive;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                    }
                    else
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
        else if (bUseGeometryShaders)
        {
            FRHIBeginRenderPassInfo RenderPass;
            RenderPass.DepthStencilView                = FRHIDepthStencilView(Resources.ShadowMapCascades.Get());
            RenderPass.DepthStencilView.ArrayIndex     = 0;
            RenderPass.DepthStencilView.NumArraySlices = NUM_SHADOW_CASCADES;

            CommandList.BeginRenderPass(RenderPass);

            const float CascadeSize = static_cast<float>(Resources.CascadeSize);
            FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            FSceneDirectionalLight* SceneDirectionalLight = Scene->DirectionalLight;
            for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
            {
                FMaterial* Material = Batch.Material;

                FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);

                CommandList.SetGraphicsPipelineState(PipelineState);
                CommandList.SetShaderResourceView(Instance->GeometryShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                {
                    FProxySceneComponent* Component = MeshReference.Primitive;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                    }
                    else
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
        else
        {
            for (uint32 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
            {
                FPerCascadeHLSL PerCascadeData;
                PerCascadeData.CascadeIndex = static_cast<int32>(Index);;

                CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
                CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascadeHLSL)), &PerCascadeData);
                CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

                FRHIBeginRenderPassInfo RenderPass;
                RenderPass.DepthStencilView            = FRHIDepthStencilView(Resources.ShadowMapCascades.Get());
                RenderPass.DepthStencilView.ArrayIndex = static_cast<uint16>(Index);

                CommandList.BeginRenderPass(RenderPass);

                const float CascadeSize = static_cast<float>(Resources.CascadeSize);
                FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                FSceneDirectionalLight* SceneDirectionalLight = Scene->DirectionalLight;
                for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
                {
                    FMaterial* Material = Batch.Material;

                    FGraphicsPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                    if (!Instance)
                    {
                        DEBUG_BREAK();
                    }

                    FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                    CHECK(PipelineState != nullptr);
                    CommandList.SetGraphicsPipelineState(PipelineState);

                    if (Material->HasAlphaMask())
                    {
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                    }
                    if (Material->HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                    }

                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerCascadeBuffer.Get(), 0);
                    CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

                    for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                    {
                        FProxySceneComponent* Component = MeshReference.Primitive;
                        if (Material->HasHeightMap() || Material->HasAlphaMask())
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                        }
                        else
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                        }

                        CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                        ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                        CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                    }
                }

                CommandList.EndRenderPass();
            }
        }
    }

    CommandList.TransitionTexture(Resources.ShadowMapCascades.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render DirectionalLight ShadowMaps");
}


FShadowMaskRenderPass::FShadowMaskRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , PipelineStates()
    , ShadowSettingsBuffer(nullptr)
{
}

FShadowMaskRenderPass::~FShadowMaskRenderPass()
{
}

bool FShadowMaskRenderPass::Initialize(FFrameResources& Resources)
{
    if (!CreateResources(Resources, Resources.CurrentWidth, Resources.CurrentHeight))
    {
        return false;
    }

    FShadowMaskShaderCombination Combination;
    RetrieveCurrentCombinationBasedOnCVar(Combination);

    FComputePipelineStateInstance Instance;
    if (!RetrievePipelineState(Combination, Instance))
    {
        return false;
    }

    if (!Instance.Shader)
    {
        return false;
    }

    if (!Instance.PipelineState)
    {
        return false;
    }

    FRHIBufferInfo SettingsBufferInfo(sizeof(FDirectionalShadowSettingsHLSL), sizeof(FDirectionalShadowSettingsHLSL), EBufferUsageFlags::ConstantBuffer);
    ShadowSettingsBuffer = RHICreateBuffer(SettingsBufferInfo, EResourceAccess::ConstantBuffer);
    if (!ShadowSettingsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ShadowSettingsBuffer->SetDebugName("ShadowSettingsBuffer");
    }

    return true;
}

bool FShadowMaskRenderPass::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;

    FRHITextureInfo ShadowMaskInfo = FRHITextureInfo::CreateTexture2D(Resources.ShadowMaskFormat, Width, Height, 1, 1, Flags);
    Resources.DirectionalShadowMask = RHICreateTexture(ShadowMaskInfo, EResourceAccess::NonPixelShaderResource);
    if (Resources.DirectionalShadowMask)
    {
        Resources.DirectionalShadowMask->SetDebugName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureInfo CascadeIndexBufferInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, Flags);
    Resources.CascadeIndexBuffer = RHICreateTexture(CascadeIndexBufferInfo, EResourceAccess::NonPixelShaderResource);
    if (Resources.CascadeIndexBuffer)
    {
        Resources.CascadeIndexBuffer->SetDebugName("Cascade Index Debug Buffer");
    }
    else
    {
        return false;
    }

    return true;
}

void FShadowMaskRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render ShadowMasks");

    TRACE_SCOPE("Render ShadowMasks");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

    // Send the settings to the GPU
    FDirectionalShadowSettingsHLSL ShadowSettings;
    FMemory::Memzero(&ShadowSettings);

    ShadowSettings.FilterSize    = FMath::Max<float>(static_cast<float>(CVarCSMFilterSize.GetValue()), 1.0f);
    ShadowSettings.MaxFilterSize = FMath::Max<float>(static_cast<float>(CVarCSMMaxFilterSize.GetValue()), 1.0f);

    CommandList.TransitionBuffer(ShadowSettingsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(ShadowSettingsBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalShadowSettingsHLSL)), &ShadowSettings);
    CommandList.TransitionBuffer(ShadowSettingsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    FShadowMaskShaderCombination Combination;
    RetrieveCurrentCombinationBasedOnCVar(Combination);

    FComputePipelineStateInstance PipelineStateInstance;
    if (!RetrievePipelineState(Combination, PipelineStateInstance))
    {
        DEBUG_BREAK();
        return;
    }

    if (CVarCSMDebugCascades.GetValue())
    {
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
    }

    CommandList.SetComputePipelineState(PipelineStateInstance.PipelineState.Get());

    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.DirectionalLightDataBuffer.Get(), 1);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), ShadowSettingsBuffer.Get(), 2);

    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeSplitsBufferSRV.Get(), 1);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.ShadowMapCascades->GetShaderResourceView(), 4);

    CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.DirectionalShadowMask->GetUnorderedAccessView(), 0);
    if (CVarCSMDebugCascades.GetValue())
    {
        CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
    }

    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerPointCmp.Get(), 0);
    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerLinearCmp.Get(), 1);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = FMath::DivideByMultiple(Resources.DirectionalShadowMask->GetWidth(), NumThreads);
    const uint32 ThreadsY = FMath::DivideByMultiple(Resources.DirectionalShadowMask->GetHeight(), NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    if (CVarCSMDebugCascades.GetValue())
    {
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render ShadowMasks");
}

bool FShadowMaskRenderPass::RetrievePipelineState(const FShadowMaskShaderCombination& Combination, FComputePipelineStateInstance& OutPSO)
{
    if (FComputePipelineStateInstance* PipelineState = PipelineStates.Find(Combination))
    {
        OutPSO = *PipelineState;
        return true;
    }

    TArray<uint8> ShaderCode;
    TArray<FShaderDefine> Defines;

    FString DebugName = "ShadowMask PSO (";
    if (Combination.FilterFunction == CSMFilterFunction_GridPCF)
    {
        Defines.Emplace("FILTER_MODE_PCF_GRID", "1");
        Defines.Emplace("FILTER_MODE_PCF_POISSION_DISC", "0");
        DebugName += " GridPCF ";
    }
    else if (Combination.FilterFunction == CSMFilterFunction_PoissonDiscPCF)
    {
        Defines.Emplace("FILTER_MODE_PCF_GRID", "0");
        Defines.Emplace("FILTER_MODE_PCF_POISSION_DISC", "1");
        DebugName += " PoissonDiscPCF ";
    }
    else
    {
        Defines.Emplace("FILTER_MODE_PCF_GRID", "0");
        Defines.Emplace("FILTER_MODE_PCF_POISSION_DISC", "0");
    }

    if (Combination.bDebugMode)
    {
        Defines.Emplace("ENABLE_DEBUG", "1");
        DebugName += " Debug ";
    }
    else
    {
        Defines.Emplace("ENABLE_DEBUG", "0");
    }

    if (Combination.bRotateSamples)
    {
        Defines.Emplace("ROTATE_SAMPLES", "1");
        DebugName += " RotateSamples ";
    }
    else
    {
        Defines.Emplace("ROTATE_SAMPLES", "0");
    }

    if (Combination.bSelectCascadeFromProjection)
    {
        Defines.Emplace("SELECT_CASCADE_FROM_PROJECTION", "1");
        DebugName += " CascadeFromProjection ";
    }
    else
    {
        Defines.Emplace("SELECT_CASCADE_FROM_PROJECTION", "0");
    }

    if (Combination.bBlendCascades)
    {
        Defines.Emplace("BLEND_CASCADES", "1");
        DebugName += " BlendCascades ";
    }
    else
    {
        Defines.Emplace("BLEND_CASCADES", "0");
    }

    if (Combination.NumPoissonSamples <= 16)
    {
        Defines.Emplace("NUM_PCF_SAMPLES", "16");
        DebugName += " NumPoissonSamples=16 ";
    }
    else if (Combination.NumPoissonSamples <= 32)
    {
        Defines.Emplace("NUM_PCF_SAMPLES", "32");
        DebugName += " NumPoissonSamples=32 ";
    }
    else if (Combination.NumPoissonSamples <= 64)
    {
        Defines.Emplace("NUM_PCF_SAMPLES", "64");
        DebugName += " NumPoissonSamples=64 ";
    }
    else if (Combination.NumPoissonSamples <= 128)
    {
        Defines.Emplace("NUM_PCF_SAMPLES", "128");
        DebugName += " NumPoissonSamples=128 ";
    }

    DebugName += ")";

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FComputePipelineStateInstance PipelineStateInstance;
    PipelineStateInstance.Shader = RHICreateComputeShader(ShaderCode);
    if (!PipelineStateInstance.Shader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer MaskPSOInitializer(PipelineStateInstance.Shader.Get());
    PipelineStateInstance.PipelineState = RHICreateComputePipelineState(MaskPSOInitializer);
    if (!PipelineStateInstance.PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineStateInstance.PipelineState->SetDebugName(DebugName);
    }

    PipelineStates.Add(Combination, PipelineStateInstance);
    OutPSO = PipelineStateInstance;
    return true;
}

void FShadowMaskRenderPass::RetrieveCurrentCombinationBasedOnCVar(FShadowMaskShaderCombination& OutCombination)
{
    OutCombination.FilterFunction               = CVarCSMFilterFunction.GetValue();
    OutCombination.bDebugMode                   = CVarCSMDebugCascades.GetValue();
    OutCombination.bBlendCascades               = CVarCSMBlendCascades.GetValue();
    OutCombination.bSelectCascadeFromProjection = CVarCSMSelectCascadeFromProjection.GetValue();
    OutCombination.bRotateSamples               = CVarCSMRotateSamples.GetValue();
    OutCombination.NumPoissonSamples            = CVarNumPoissonDiscSamples.GetValue();
}
