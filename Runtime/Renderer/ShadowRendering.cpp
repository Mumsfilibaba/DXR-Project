#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RHIPipelineState.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "Renderer/ShadowRendering.h"
#include "Renderer/Scene.h"
#include "Renderer/Performance/GPUProfiler.h"

static TAutoConsoleVariable<bool> CVarPointLightsEnableSinglePassRendering(
    "Renderer.PointLights.EnableSinglePassRendering",
    "Enables instancing for cube-map rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarPointLightsEnableGeometryShaderInstancing(
    "Renderer.PointLights.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cube-map drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMDebugCascades(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableSinglePassRendering(
    "Renderer.CSM.EnableSinglePassRendering",
    "Enables instancing for cascade rendering via VertexShaders, enabling a single-pass for rendering a full cube-map, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableGeometryShaderInstancing(
    "Renderer.CSM.EnableGeometryShaderInstancing",
    "Enables instancing in a geometry shader, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarCSMEnableViewInstancing(
    "Renderer.CSM.EnableViewInstancing",
    "Enables view-instancing for cascade rendering, enabling single-pass cascade drawing, which creates less overhead on the CPU",
    false,
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

static TAutoConsoleVariable<int32> CVarCSMNumPoissonDiscSamples(
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
}

FGraphicsPipelineStateInstance* FPointLightRenderPass::CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& FrameResources)
{
    FPointLightShaderCombination ShaderCombination;
    ShaderCombination.MaterialFlags  = static_cast<int32>(Material->GetMaterialFlags());
    ShaderCombination.RenderPassType = RenderPassType;
    
    FGraphicsPipelineStateInstance* CachedPointLightPSO = MaterialPSOs.Find(ShaderCombination);
    if (!CachedPointLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (Material->HasHeightMap())
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");
        }

        if (Material->HasPackedDiffuseAlpha())
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");
        }

        if (Material->HasAlphaMask())
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");
        }
        
        if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::SinglePass)
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VS_INSTANCING", "(1)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(0)");
        }
        else if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_VS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_POINTLIGHT_GS_INSTANCING", "(0)");
        }

        FShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            CompileInfo = FShaderCompileInfo("Point_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.GeometryShader = RHICreateGeometryShader(ShaderCode);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        CompileInfo = FShaderCompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        // Initialize standard input layout
        FRHIVertexLayoutInitializerList VertexElementList;
        if (Material->SupportsPixelDiscard())
        {
            VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = RHICreateVertexLayout(VertexElementList);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
        if (!NewPipelineStateInstance.PixelShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (Material->IsDoubleSided())
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
            return nullptr;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return nullptr;
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

        if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            PSOInitializer.ShaderState.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return nullptr;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("Point ShadowMap PipelineState %d", ShaderCombination.MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        // Return the new instance
        FGraphicsPipelineStateInstance& NewInstance = MaterialPSOs.Add(ShaderCombination, Move(NewPipelineStateInstance));
        return &NewInstance;
    }
    else
    {
        // Return the existing instance
        return CachedPointLightPSO;
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
    const auto GetRenderMapRenderPassType = []() -> ECubeMapRenderPassType
    {
        const bool bUseVSInstancing = GRHISupportRenderTargetArrayIndexFromVertexShader && CVarPointLightsEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing = !bUseVSInstancing && GRHISupportsGeometryShaders && CVarPointLightsEnableGeometryShaderInstancing.GetValue();
        if (bUseVSInstancing)
        {
            return ECubeMapRenderPassType::SinglePass;
        }
        else if (bUseGSInstancing)
        {
            return ECubeMapRenderPassType::GeometryShaderSinglePass;
        }
        else
        {
            return ECubeMapRenderPassType::MultiPass;
        }
        
        return ECubeMapRenderPassType::Unknown;
    };

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    const ECubeMapRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (RenderPassType == ECubeMapRenderPassType::SinglePass)
    {
        Execute<ECubeMapRenderPassType::SinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
    {
        Execute<ECubeMapRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
    }
    else if (RenderPassType == ECubeMapRenderPassType::MultiPass)
    {
        Execute<ECubeMapRenderPassType::MultiPass>(CommandList, Resources, Scene);
    }

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");
}

template<ECubeMapRenderPassType RenderPassType>
void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    // Shader-structs
    FShadowPerObjectHLSL ShadowPerObjectBuffer;

    // Clamp the number of shadow-casting point-lights
    const int32 NumPointLights = FMath::Min<int32>(Scene->PointLights.Size(), Resources.MaxPointLightShadows);

    constexpr bool bIsSinglePass = RenderPassType == ECubeMapRenderPassType::SinglePass || RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass;
    if constexpr (bIsSinglePass)
    {
        // Per light data
        FSinglePassPointLightBufferHLSL SinglePassPointLightBuffer;

        for (int32 LightIndex = 0; LightIndex < NumPointLights; ++LightIndex)
        {
            FScenePointLight* ScenePointLight = Scene->PointLights[LightIndex];
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                const FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
                SinglePassPointLightBuffer.LightPosition               = Data.Position;
                SinglePassPointLightBuffer.LightFarPlane               = Data.FarPlane;
                SinglePassPointLightBuffer.LightProjections[FaceIndex] = Data.Matrix;
            }

            CommandList.TransitionBuffer(SinglePassShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(SinglePassShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FSinglePassPointLightBufferHLSL)), &SinglePassPointLightBuffer);
            CommandList.TransitionBuffer(SinglePassShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            FRHIBeginRenderPassInfo RenderPass;
            RenderPass.DepthStencilView                = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get());
            RenderPass.DepthStencilView.ArrayIndex     = static_cast<uint16>(LightIndex * RHI_NUM_CUBE_FACES);
            RenderPass.DepthStencilView.NumArraySlices = RHI_NUM_CUBE_FACES;
            RenderPass.DepthStencilView.LoadAction     = EAttachmentLoadAction::Clear;
            RenderPass.DepthStencilView.StoreAction    = EAttachmentStoreAction::Store;
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
                FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Material, Resources);
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);

                CommandList.SetGraphicsPipelineState(PipelineState);
                
                // If we are using geometry-shaders for a single-pass, then bind the matrices to the geometry-shader,
                // otherwise we bind the matrices to the vertex-shader.
                if constexpr (RenderPassType == ECubeMapRenderPassType::SinglePass)
                {
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                }
                else
                {
                    CommandList.SetConstantBuffer(Instance->GeometryShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                }
                
                // If we have a pixel-shader, bind all resources that shader needs
                if (Instance->PixelShader)
                {
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), SinglePassShadowMapBuffer.Get(), 0);
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
    
                    if (Material->HasAlphaMask())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                    }
                    if (Material->HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                    }
                }

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                {
                    FProxySceneComponent* Component = MeshReference.Primitive;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                            Component->Mesh->GetVertexBuffer(EVertexStream::TexCoords),
                        };
                        
                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                    }
                    else
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                        };
                        
                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.WorldMatrix = Component->CurrentActor->GetTransform().GetTransformMatrix();
                    ShadowPerObjectBuffer.WorldMatrix = ShadowPerObjectBuffer.WorldMatrix.GetTranspose();

                    constexpr uint32 NumConstants = sizeof(FShadowPerObjectHLSL) / sizeof(uint32);
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, NumConstants);

                    if constexpr (RenderPassType == ECubeMapRenderPassType::SinglePass)
                    {
                        // One instance per face
                        constexpr uint32 SinglePassInstanceCount = 6;
                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                    }
                    else
                    {
                        // When using a geometry shader we just have a single instance
                        constexpr uint32 SinglePassInstanceCount = 1;
                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                    }
                }
            }

            CommandList.EndRenderPass();
        }
    }
    else
    {
        // Per cube-face data
        FPerShadowMapHLSL PerShadowMapData;

        for (int32 LightIndex = 0; LightIndex < NumPointLights; ++LightIndex)
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
                RenderPass.DepthStencilView             = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);
                RenderPass.DepthStencilView.LoadAction  = EAttachmentLoadAction::Clear;
                RenderPass.DepthStencilView.StoreAction = EAttachmentStoreAction::Store;
                RenderPass.DepthStencilView.ClearValue  = FDepthStencilValue(1.0f, 0);

                CommandList.BeginRenderPass(RenderPass);

                const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
                FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                for (const FMeshBatch& Batch : ScenePointLight->MeshBatches[FaceIndex])
                {
                    FMaterial* Material = Batch.Material;
                    FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Material, Resources);
                    if (!Instance)
                    {
                        DEBUG_BREAK();
                    }

                    FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                    CHECK(PipelineState != nullptr);

                    CommandList.SetGraphicsPipelineState(PipelineState);
                    CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                    
                    // If we have a pixel-shader, bind all resources that shader needs
                    if (Instance->PixelShader)
                    {
                        CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);
                        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                        if (Material->HasAlphaMask())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                        }
                        if (Material->HasHeightMap())
                        {
                            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                        }
                    }

                    for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                    {
                        FProxySceneComponent* Component = MeshReference.Primitive;
                        if (Material->HasHeightMap() || Material->HasAlphaMask())
                        {
                            FRHIBuffer* VertexBuffers[] =
                            {
                                Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                                Component->Mesh->GetVertexBuffer(EVertexStream::TexCoords),
                            };
                            
                            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                        }
                        else
                        {
                            FRHIBuffer* VertexBuffers[] =
                            {
                                Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                            };
                            
                            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                        }

                        CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                        ShadowPerObjectBuffer.WorldMatrix = Component->CurrentActor->GetTransform().GetTransformMatrix();
                        ShadowPerObjectBuffer.WorldMatrix = ShadowPerObjectBuffer.WorldMatrix.GetTranspose();

                        constexpr uint32 NumConstants = sizeof(FShadowPerObjectHLSL) / sizeof(uint32);
                        CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, NumConstants);

                        CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                    }
                }

                CommandList.EndRenderPass();
            }
        }
    }
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

FGraphicsPipelineStateInstance* FCascadedShadowsRenderPass::CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& FrameResources)
{
    FCascadedShadowsShaderCombination ShaderCombination;
    ShaderCombination.RenderPassType = RenderPassType;
    ShaderCombination.MaterialFlags  = static_cast<int32>(Material->GetMaterialFlags());

    FGraphicsPipelineStateInstance* CachedDirectionalLightPSO = MaterialPSOs.Find(ShaderCombination);
    if (!CachedDirectionalLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (Material->HasHeightMap())
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");
        }

        if (Material->HasPackedDiffuseAlpha())
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");
        }

        if (Material->HasAlphaMask())
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");
        }

        if (RenderPassType == ECascadeRenderPassType::SinglePass)
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VS_INSTANCING", "(1)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(0)");
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(1)");
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(0)");
        }
        else if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_CASCADE_VS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_GS_INSTANCING", "(0)");
            ShaderDefines.Emplace("ENABLE_CASCADE_VIEW_INSTANCING", "(0)");
        }

        FShaderCompileInfo CompileInfo("Cascade_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            CompileInfo = FShaderCompileInfo("Cascade_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.GeometryShader = RHICreateGeometryShader(ShaderCode);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        const bool bWantPixelShader = Material->HasHeightMap() || Material->HasAlphaMask() || Material->HasPackedDiffuseAlpha();
        if (bWantPixelShader)
        {
            CompileInfo = FShaderCompileInfo("Cascade_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
            if (!NewPipelineStateInstance.PixelShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        // Initialize standard input layout
        FRHIVertexLayoutInitializerList VertexElementList;
        if (Material->SupportsPixelDiscard())
        {
            VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = RHICreateVertexLayout(VertexElementList);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.bEnableDepthBias     = true;
        RasterizerStateInitializer.DepthBias            = 1.0f;
        RasterizerStateInitializer.DepthBiasClamp       = 0.05f;
        RasterizerStateInitializer.SlopeScaledDepthBias = 1.0f;

        if (Material->IsDoubleSided())
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
            return nullptr;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return nullptr;
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

        if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            PSOInitializer.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
            PSOInitializer.ViewInstancingInfo.NumArraySlices              = NUM_SHADOW_CASCADES;
            PSOInitializer.ViewInstancingInfo.bEnableViewInstancing       = true;
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            PSOInitializer.ShaderState.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.ShadowMapFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return nullptr;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("CSM PipelineState %d", ShaderCombination.MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        // Return the new instance
        FGraphicsPipelineStateInstance& NewInstance = MaterialPSOs.Add(ShaderCombination, Move(NewPipelineStateInstance));
        return &NewInstance;
    }
    else
    {
        return CachedDirectionalLightPSO;
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
    const auto GetRenderMapRenderPassType = []() -> ECascadeRenderPassType
    {
        constexpr uint32 MinViewInstanceCount = 4;

        const bool bUseVSInstancing   = GRHISupportRenderTargetArrayIndexFromVertexShader && CVarCSMEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing   = !bUseVSInstancing && GRHISupportsGeometryShaders && CVarCSMEnableGeometryShaderInstancing.GetValue();
        const bool bUseViewInstancing = !bUseGSInstancing && GRHISupportsViewInstancing && GRHIMaxViewInstanceCount >= MinViewInstanceCount && CVarCSMEnableViewInstancing.GetValue();

        if (bUseVSInstancing)
        {
            return ECascadeRenderPassType::SinglePass;
        }
        else if (bUseGSInstancing)
        {
            return ECascadeRenderPassType::GeometryShaderSinglePass;
        }
        else if (bUseViewInstancing)
        {
            return ECascadeRenderPassType::ViewInstancingSinglePass;
        }
        else
        {
            return ECascadeRenderPassType::MultiPass;
        }

        return ECascadeRenderPassType::Unknown;
    };

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render DirectionalLight ShadowMaps");

    TRACE_SCOPE("Render DirectionalLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

    const ECascadeRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (Scene->DirectionalLight)
    {
        CommandList.TransitionTexture(Resources.ShadowMapCascades.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        if (RenderPassType == ECascadeRenderPassType::SinglePass)
        {
            Execute<ECascadeRenderPassType::SinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            Execute<ECascadeRenderPassType::GeometryShaderSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            Execute<ECascadeRenderPassType::ViewInstancingSinglePass>(CommandList, Resources, Scene);
        }
        else if (RenderPassType == ECascadeRenderPassType::MultiPass)
        {
            Execute<ECascadeRenderPassType::MultiPass>(CommandList, Resources, Scene);
        }

        CommandList.TransitionTexture(Resources.ShadowMapCascades.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render DirectionalLight ShadowMaps");
}

template<ECascadeRenderPassType RenderPassType>
void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    // PerObject Structs
    FShadowPerObjectHLSL ShadowPerObjectBuffer;

    constexpr bool bIsSinglePass = 
        RenderPassType == ECascadeRenderPassType::SinglePass ||
        RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass ||
        RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass;

    FSceneDirectionalLight* SceneDirectionalLight = Scene->DirectionalLight;
    if constexpr (bIsSinglePass)
    {
        FRHIBeginRenderPassInfo RenderPass;
        RenderPass.DepthStencilView                = FRHIDepthStencilView(Resources.ShadowMapCascades.Get());
        RenderPass.DepthStencilView.ArrayIndex     = 0;
        RenderPass.DepthStencilView.NumArraySlices = NUM_SHADOW_CASCADES;

        // Setup view-instancing
        if constexpr (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            RenderPass.ViewInstancingInfo.StartRenderTargetArrayIndex = 0;
            RenderPass.ViewInstancingInfo.NumArraySlices              = NUM_SHADOW_CASCADES;
            RenderPass.ViewInstancingInfo.bEnableViewInstancing       = true;
        }

        CommandList.BeginRenderPass(RenderPass);

        const float CascadeSize = static_cast<float>(Resources.CascadeSize);
        FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.SetViewport(ViewportRegion);

        FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
        CommandList.SetScissorRect(ScissorRegion);

        for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
        {
            FMaterial* Material = Batch.Material;
            FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Material, Resources);
            if (!Instance)
            {
                DEBUG_BREAK();
            }

            FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
            CHECK(PipelineState != nullptr);

            CommandList.SetGraphicsPipelineState(PipelineState);

            // If we are using geometry-shaders bind the necessary buffer to the geometry-shader otherwise to the vertex-shader
            if constexpr (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
            {
                CommandList.SetShaderResourceView(Instance->GeometryShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
            }
            else
            {
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
            }

            // If this material require a pixel-shader, bind necessary pixel-shader resources
            if (Instance->PixelShader)
            {
                CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }
            }

            // Draw all the objects
            for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
            {
                FProxySceneComponent* Component = MeshReference.Primitive;
                if (Material->HasHeightMap() || Material->HasAlphaMask())
                {
                    FRHIBuffer* VertexBuffers[] =
                    {
                        Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                        Component->Mesh->GetVertexBuffer(EVertexStream::TexCoords),
                    };

                    CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                }
                else
                {
                    FRHIBuffer* VertexBuffers[] =
                    {
                        Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                    };

                    CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                }

                CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                ShadowPerObjectBuffer.WorldMatrix = Component->CurrentActor->GetTransform().GetTransformMatrix();
                ShadowPerObjectBuffer.WorldMatrix = ShadowPerObjectBuffer.WorldMatrix.GetTranspose();

                constexpr uint32 NumConstants = sizeof(FShadowPerObjectHLSL) / sizeof(uint32);
                CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, NumConstants);

                // If we use vertex-shader instancing, we need to create our own instances and use instanced rendering
                if constexpr (RenderPassType == ECascadeRenderPassType::SinglePass)
                {
                    // One instance per cascade
                    constexpr uint32 SinglePassInstanceCount = 4;
                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                }
                else
                {
                    // When using a geometry-shader or view-instance we just have a single instance
                    constexpr uint32 SinglePassInstanceCount = 1;
                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, SinglePassInstanceCount, MeshReference.StartIndex, 0, 0);
                }
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

            for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
            {
                FMaterial* Material = Batch.Material;
                FGraphicsPipelineStateInstance* Instance = CompilePipelineStateInstance(RenderPassType, Material, Resources);
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);
                CommandList.SetGraphicsPipelineState(PipelineState);

                // Bind pixel-shader resources if there are any
                if (Instance->PixelShader)
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

                    if (Material->HasAlphaMask())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                    }
                    if (Material->HasHeightMap())
                    {
                        CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                    }
                }

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerCascadeBuffer.Get(), 0);
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.Primitives)
                {
                    FProxySceneComponent* Component = MeshReference.Primitive;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                            Component->Mesh->GetVertexBuffer(EVertexStream::TexCoords),
                        };

                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                    }
                    else
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            Component->Mesh->GetVertexBuffer(EVertexStream::Positions),
                        };

                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.WorldMatrix = Component->CurrentActor->GetTransform().GetTransformMatrix();
                    ShadowPerObjectBuffer.WorldMatrix = ShadowPerObjectBuffer.WorldMatrix.GetTranspose();

                    constexpr uint32 NumConstants = sizeof(FShadowPerObjectHLSL) / sizeof(uint32);
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, NumConstants);

                    CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
    }
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
    ShadowSettings.ShadowMapSize = Resources.ShadowMapCascades->GetWidth();
    ShadowSettings.FrameIndex    = GetRenderer()->GetFrameCounter().GetFrameIndex();

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
    OutCombination.NumPoissonSamples            = CVarCSMNumPoissonDiscSamples.GetValue();
}
