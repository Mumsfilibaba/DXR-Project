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
#include "Renderer/ShadowRendering.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

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

static TAutoConsoleVariable<bool> CVarCSMStableCascades(
    "Renderer.CSM.StableCascades",
    "Set to true to enable stable cascades when generating shadow cascade matrices",
    true,
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

static TAutoConsoleVariable<bool> CVarCSMEnableDepthClipping(
    "Renderer.CSM.EnableDepthClipping",
    "Enables depth-clipping for cascade rendering.",
    false,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterMode(
    "Renderer.CSM.FilterMode",
    "Select mode when filer Cascaded Shadow Maps. 0: Percentage Closer Filtering (PCF) 1: Percentage Closer Soft Shadows (PCSS)",
    0,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterFunction(
    "Renderer.CSM.FilterFunction",
    "Select function to use to filer Cascaded Shadow Maps. 0: Grid 1: Poisson Disk 2: Vogel Disk",
    1,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMFilterSize(
    "Renderer.CSM.FilterSize",
    "Size of the filter for the Cascaded Shadow Maps",
    256,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMMaxFilterSize(
    "Renderer.CSM.MaxFilterSize",
    "Maximum size of the filter for the Cascaded Shadow Maps",
    512,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarCSMNumPoissonDiscSamples(
    "Renderer.CSM.NumPoissonDiscSamples",
    "Number Poisson Samples to use when sampling the Cascaded Shadow Maps using a Poisson Disc",
    32,
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

FGraphicsPipelineStateInstance* FPointLightRenderPass::CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& /* FrameResources */)
{
    FPointLightShaderCombination ShaderCombination;
    ShaderCombination.MaterialFlags  = static_cast<uint32>(Material->GetMaterialFlags());
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
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/PointLightShadows.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);

        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            CompileInfo = FShaderCompileInfo("Point_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/PointLightShadows.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.GeometryShader = FRHI::Get()->CreateGeometryShader(ShaderCode);
            if (!NewPipelineStateInstance.GeometryShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        CompileInfo = FShaderCompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/PointLightShadows.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        // Initialize standard input layout
        TArray<FRHIInputElementInfo> InputElements;
        if (Material->SupportsPixelDiscard())
        {
            InputElements =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            InputElements =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = FRHI::Get()->CreateInputLayout(InputElements);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        NewPipelineStateInstance.PixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!NewPipelineStateInstance.PixelShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIDepthStencilStateInfo DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        if (Material->IsDoubleSided())
        {
            RasterizerStateInfo.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInfo.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIBlendStateInfo BlendStateInfo;
        NewPipelineStateInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                                 = NewPipelineStateInstance.BlendState.Get();
        PSOInfo.DepthStencilState                          = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInfo.bPrimitiveRestartEnable                    = false;
        PSOInfo.InputLayout                                = NewPipelineStateInstance.InputLayout.Get();
        PSOInfo.PrimitiveTopology                          = EPrimitiveTopology::TriangleList;
        PSOInfo.RasterizerState                            = NewPipelineStateInstance.RasterizerState.Get();
        PSOInfo.MultiSampleState.SampleCount               = 1;
        PSOInfo.MultiSampleState.SampleQuality             = 0;
        PSOInfo.MultiSampleState.SampleMask                = RHI_DEFAULT_SAMPLE_MASK;
        PSOInfo.VertexShader                               = NewPipelineStateInstance.VertexShader.Get();
        PSOInfo.PixelShader                                = NewPipelineStateInstance.PixelShader.Get();
        PSOInfo.RasterizerOutputFormats.NumRenderTargets   = 0;
        PSOInfo.RasterizerOutputFormats.DepthStencilFormat = FGlobalTextureFormats::ShadowMapFormat;

        if (ShaderCombination.RenderPassType == ECubeMapRenderPassType::GeometryShaderSinglePass)
        {
            PSOInfo.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        NewPipelineStateInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
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
    FRHIBufferInfo PerShadowMapBufferInfo;
    PerShadowMapBufferInfo.Stride = sizeof(FPerShadowMapHLSL);
    PerShadowMapBufferInfo.Size   = sizeof(FPerShadowMapHLSL);
    PerShadowMapBufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;

    PerShadowMapBuffer = FRHI::Get()->CreateBuffer(PerShadowMapBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (!PerShadowMapBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetDebugName("Per ShadowMap Buffer");
    }

	FRHIBufferInfo SinglePassShadowMapBufferInfo;
    SinglePassShadowMapBufferInfo.Stride = sizeof(FSinglePassPointLightBufferHLSL);
    SinglePassShadowMapBufferInfo.Size   = sizeof(FSinglePassPointLightBufferHLSL);
    SinglePassShadowMapBufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;

    SinglePassShadowMapBuffer = FRHI::Get()->CreateBuffer(SinglePassShadowMapBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
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
    const FClearValue DepthClearValue(FGlobalTextureFormats::ShadowMapFormat, 1.0f, 0);

    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureInfo PointLightInfo = FRHITextureInfo::CreateTextureCubeArray(FGlobalTextureFormats::ShadowMapFormat, Resources.PointLightShadowSize, Resources.MaxPointLightShadows, 1, 1, Flags, DepthClearValue);
    Resources.PointLightShadowMaps = FRHI::Get()->CreateTexture(PointLightInfo, EResourceAccess::PixelShaderResource);

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
        const bool bUseVSInstancing = RHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader && CVarPointLightsEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing = !bUseVSInstancing && RHIDeviceInfo::SupportsGeometryShaders && CVarPointLightsEnableGeometryShaderInstancing.GetValue();
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
    };

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite));

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

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource));

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");
}

template<ECubeMapRenderPassType RenderPassType>
void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    // Shader-structs
    FShadowPerObjectHLSL ShadowPerObjectBuffer;

    // Clamp the number of shadow-casting point-lights
    const int32 NumPointLights = Math::Min<int32>(Scene->PointLights.Size(), Resources.MaxPointLightShadows);

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
                SinglePassPointLightBuffer.LightProjections[FaceIndex] = Data.ViewProjMatrix;
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

            for (const FMeshBatch& Batch : ScenePointLight->SinglePassShadowView.GetMeshBatches())
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

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
                {
                    FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                        };
                        
                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                    }
                    else
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                        };
                        
                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                    }

                    CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

                    const FTransformBufferHLSL& TransformShaderData = StaticMesh->GetTransformShaderData();
                    ShadowPerObjectBuffer.WorldMatrix = TransformShaderData.Transform;

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
                PerShadowMapData.Matrix   = Data.ViewProjMatrix;
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

                const TArray<FMeshBatch>& MeshBatches = ScenePointLight->ShadowView[FaceIndex].GetMeshBatches();
                for (const FMeshBatch& Batch : MeshBatches)
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

                    for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
                    {
                        FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                        if (Material->HasHeightMap() || Material->HasAlphaMask())
                        {
                            FRHIBuffer* VertexBuffers[] =
                            {
                                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                            };

                            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                        }
                        else
                        {
                            FRHIBuffer* VertexBuffers[] =
                            {
                                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                            };

                            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                        }

                        CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

                        const FTransformBufferHLSL& TransformShaderData = StaticMesh->GetTransformShaderData();
                        ShadowPerObjectBuffer.WorldMatrix = TransformShaderData.Transform;

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
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/CascadeMatrixGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    CascadeGenShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!CascadeGenShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInfo CascadeMatrixGenPSOInfo;
    CascadeMatrixGenPSOInfo.Shader = CascadeGenShader.Get();

    CascadeGen = FRHI::Get()->CreateComputePipelineState(CascadeMatrixGenPSOInfo);
    if (!CascadeGen)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CascadeGen->SetDebugName("CascadeGen PSO");
    }

	FRHIBufferInfo CascadeMatrixBufferInfo;
    CascadeMatrixBufferInfo.Stride = sizeof(FCascadeMatricesHLSL);
    CascadeMatrixBufferInfo.Size   = CascadeMatrixBufferInfo.Stride * NUM_SHADOW_CASCADES;
    CascadeMatrixBufferInfo.Flags  = EBufferFlags::RWBuffer | EBufferFlags::Default;

    Resources.CascadeMatrixBuffer = FRHI::Get()->CreateBuffer(CascadeMatrixBufferInfo, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeMatrixBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
    }

    FRHIBufferSRVInfo SRVInfo(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferSRV = FRHI::Get()->CreateShaderResourceView(SRVInfo);

    if (!Resources.CascadeMatrixBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferUAVInfo UAVInfo(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferUAV = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);

    if (!Resources.CascadeMatrixBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

	FRHIBufferInfo CascadeSplitsBufferInfo;
    CascadeSplitsBufferInfo.Stride = sizeof(FCascadeSplitHLSL);
    CascadeSplitsBufferInfo.Size   = CascadeSplitsBufferInfo.Stride * NUM_SHADOW_CASCADES;
    CascadeSplitsBufferInfo.Flags  = EBufferFlags::RWBuffer | EBufferFlags::Default;

    Resources.CascadeSplitsBuffer = FRHI::Get()->CreateBuffer(CascadeSplitsBufferInfo, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeSplitsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
    }

    SRVInfo = FRHIBufferSRVInfo(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferSRV = FRHI::Get()->CreateShaderResourceView(SRVInfo);

    if (!Resources.CascadeSplitsBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    UAVInfo = FRHIBufferUAVInfo(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferUAV = FRHI::Get()->CreateUnorderedAccessView(UAVInfo);

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

FGraphicsPipelineStateInstance* FCascadedShadowsRenderPass::CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& /* FrameResources */ )
{
    FCascadedShadowsShaderCombination ShaderCombination;
    ShaderCombination.RenderPassType       = RenderPassType;
    ShaderCombination.bEnableDepthClipping = CVarCSMEnableDepthClipping.GetValue();
    ShaderCombination.MaterialFlags        = static_cast<uint32>(Material->GetMaterialFlags());

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
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/CascadedShadows.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FGraphicsPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            CompileInfo = FShaderCompileInfo("Cascade_GSMain", EShaderModel::SM_6_2, EShaderStage::Geometry, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/CascadedShadows.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.GeometryShader = FRHI::Get()->CreateGeometryShader(ShaderCode);
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
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/CascadedShadows.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return nullptr;
            }

            NewPipelineStateInstance.PixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
            if (!NewPipelineStateInstance.PixelShader)
            {
                DEBUG_BREAK();
                return nullptr;
            }
        }

        // Initialize standard input layout
        TArray<FRHIInputElementInfo> InputElements;
        if (Material->SupportsPixelDiscard())
        {
            InputElements =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            InputElements =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = FRHI::Get()->CreateInputLayout(InputElements);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIDepthStencilStateInfo DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        RasterizerStateInfo.bDepthClipEnable = ShaderCombination.bEnableDepthClipping;

        // TODO: Revisit depth-bias
        RasterizerStateInfo.bEnableDepthBias     = true;
        RasterizerStateInfo.DepthBias            = 1.0f;
        RasterizerStateInfo.DepthBiasClamp       = 0.05f;
        RasterizerStateInfo.SlopeScaledDepthBias = 1.0f;

        if (Material->IsDoubleSided())
        {
            RasterizerStateInfo.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInfo.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIBlendStateInfo BlendStateInfo;
        NewPipelineStateInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);

        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return nullptr;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                     = NewPipelineStateInstance.BlendState.Get();
        PSOInfo.DepthStencilState              = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInfo.bPrimitiveRestartEnable        = false;
        PSOInfo.InputLayout                    = NewPipelineStateInstance.InputLayout.Get();
        PSOInfo.PrimitiveTopology              = EPrimitiveTopology::TriangleList;
        PSOInfo.RasterizerState                = NewPipelineStateInstance.RasterizerState.Get();
        PSOInfo.MultiSampleState.SampleCount   = 1;
        PSOInfo.MultiSampleState.SampleQuality = 0;
        PSOInfo.MultiSampleState.SampleMask    = RHI_DEFAULT_SAMPLE_MASK;
        PSOInfo.VertexShader                   = NewPipelineStateInstance.VertexShader.Get();
        PSOInfo.PixelShader                    = NewPipelineStateInstance.PixelShader.Get();

        if (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            PSOInfo.ViewInstancingState.StartRenderTargetArrayIndex = 0;
            PSOInfo.ViewInstancingState.NumArraySlices              = NUM_SHADOW_CASCADES;
            PSOInfo.ViewInstancingState.bEnableViewInstancing       = true;
        }
        else if (RenderPassType == ECascadeRenderPassType::GeometryShaderSinglePass)
        {
            PSOInfo.GeometryShader = NewPipelineStateInstance.GeometryShader.Get();
        }

        PSOInfo.RasterizerOutputFormats.DepthStencilFormat = FGlobalTextureFormats::ShadowMapFormat;
        PSOInfo.RasterizerOutputFormats.NumRenderTargets   = 0;

        NewPipelineStateInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
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
	FRHIBufferInfo PerCascadeBufferInfo;
    PerCascadeBufferInfo.Stride = sizeof(FPerCascadeHLSL);
    PerCascadeBufferInfo.Size   = sizeof(FPerCascadeHLSL);
    PerCascadeBufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;

    PerCascadeBuffer = FRHI::Get()->CreateBuffer(PerCascadeBufferInfo, EResourceAccess::ConstantBuffer, nullptr);
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
    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;

    const FClearValue DepthClearValue(FGlobalTextureFormats::ShadowMapFormat, 1.0f, 0);
    FRHITextureInfo CascadeInfo = FRHITextureInfo::CreateTexture2DArray(FGlobalTextureFormats::ShadowMapFormat, Resources.CascadeSize, Resources.CascadeSize, NUM_SHADOW_CASCADES, 1, 1, Flags, DepthClearValue);
    Resources.ShadowCascades = FRHI::Get()->CreateTexture(CascadeInfo, EResourceAccess::NonPixelShaderResource);

    if (Resources.ShadowCascades)
    {
        const FString DebugName = FString::CreateFormatted("Shadow Map Cascades");
        Resources.ShadowCascades->SetDebugName(DebugName);
    }
    else
    {
        DEBUG_BREAK();
        return false;
    }

    for (uint16 Index = 0; Index < NUM_SHADOW_CASCADES; Index++)
    {
        FRHITextureSRVInfo SRVInfo;
        SRVInfo.Texture         = Resources.ShadowCascades.Get();
        SRVInfo.Format          = CastSRVFormat(Resources.ShadowCascades->GetFormat());
        SRVInfo.NumSlices       = 1;
        SRVInfo.NumMips         = 1;
        SRVInfo.MinLODClamp     = 0;
        SRVInfo.FirstMipLevel   = 0;
        SRVInfo.FirstArraySlice = Index;

        Resources.ShadowCascadesSRVs[Index] = FRHI::Get()->CreateShaderResourceView(SRVInfo);
        if (!Resources.ShadowCascadesSRVs[Index])
        {
            DEBUG_BREAK();
            return false;
        }
    }

    return true;
}

void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    const auto GetRenderMapRenderPassType = []() -> ECascadeRenderPassType
    {
        constexpr uint32 MinViewInstanceCount = 4;

        const bool bUseVSInstancing   = RHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader && CVarCSMEnableSinglePassRendering.GetValue();
        const bool bUseGSInstancing   = !bUseVSInstancing && RHIDeviceInfo::SupportsGeometryShaders && CVarCSMEnableGeometryShaderInstancing.GetValue();
        const bool bUseViewInstancing = !bUseGSInstancing && RHIDeviceInfo::SupportsViewInstancing && RHIDeviceInfo::MaxViewInstanceCount >= MinViewInstanceCount && CVarCSMEnableViewInstancing.GetValue();

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
    };

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render DirectionalLight ShadowMaps");

    TRACE_SCOPE("Render DirectionalLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

    const ECascadeRenderPassType RenderPassType = GetRenderMapRenderPassType();
    if (Scene->DirectionalLight)
    {
        CommandList.TransitionTexture(Resources.ShadowCascades.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite));

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

        CommandList.TransitionTexture(Resources.ShadowCascades.Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource));
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
        RenderPass.DepthStencilView                = FRHIDepthStencilView(Resources.ShadowCascades.Get());
        RenderPass.DepthStencilView.ArrayIndex     = 0;
        RenderPass.DepthStencilView.NumArraySlices = NUM_SHADOW_CASCADES;

        // Setup view-instancing
        if constexpr (RenderPassType == ECascadeRenderPassType::ViewInstancingSinglePass)
        {
            RenderPass.ViewInstancingState.StartRenderTargetArrayIndex = 0;
            RenderPass.ViewInstancingState.NumArraySlices              = NUM_SHADOW_CASCADES;
            RenderPass.ViewInstancingState.bEnableViewInstancing       = true;
        }

        CommandList.BeginRenderPass(RenderPass);

        const float CascadeSize = static_cast<float>(Resources.CascadeSize);
        FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.SetViewport(ViewportRegion);

        FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
        CommandList.SetScissorRect(ScissorRegion);

        for (const FMeshBatch& Batch : SceneDirectionalLight->GetShadowView().GetMeshBatches())
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
            for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
            {
                FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                if (Material->HasHeightMap() || Material->HasAlphaMask())
                {
                    FRHIBuffer* VertexBuffers[] =
                    {
                        StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                        StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                    };

                    CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                }
                else
                {
                    FRHIBuffer* VertexBuffers[] =
                    {
                        StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                    };

                    CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                }

                CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

                const FTransformBufferHLSL& TransformShaderData = StaticMesh->GetTransformShaderData();
                ShadowPerObjectBuffer.WorldMatrix = TransformShaderData.Transform;

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
            PerCascadeData.CascadeIndex = static_cast<int32>(Index);

            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascadeHLSL)), &PerCascadeData);
            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            FRHIBeginRenderPassInfo RenderPass;
            RenderPass.DepthStencilView            = FRHIDepthStencilView(Resources.ShadowCascades.Get());
            RenderPass.DepthStencilView.ArrayIndex = static_cast<uint16>(Index);

            CommandList.BeginRenderPass(RenderPass);

            const float CascadeSize = static_cast<float>(Resources.CascadeSize);
            FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : SceneDirectionalLight->GetShadowView().GetMeshBatches())
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

                for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
                {
                    FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                        };

                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
                    }
                    else
                    {
                        FRHIBuffer* VertexBuffers[] =
                        {
                            StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                        };

                        CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 1), 0);
                    }

                    CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

                    const FTransformBufferHLSL& TransformShaderData = StaticMesh->GetTransformShaderData();
                    ShadowPerObjectBuffer.WorldMatrix = TransformShaderData.Transform;

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
    if (!CreateResources(Resources, Resources.CurrentRenderWidth, Resources.CurrentRenderHeight))
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

	FRHIBufferInfo SettingsBufferInfo;
    SettingsBufferInfo.Stride = sizeof(FDirectionalShadowSettingsHLSL);
    SettingsBufferInfo.Size   = sizeof(FDirectionalShadowSettingsHLSL);
    SettingsBufferInfo.Flags  = EBufferFlags::ConstantBuffer | EBufferFlags::Default;

    ShadowSettingsBuffer = FRHI::Get()->CreateBuffer(SettingsBufferInfo, EResourceAccess::ConstantBuffer);
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
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;

    FRHITextureInfo ShadowMaskInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::ShadowMaskFormat, Width, Height, 1, 1, Flags);
    Resources.DirectionalShadowMask = FRHI::Get()->CreateTexture(ShadowMaskInfo, EResourceAccess::NonPixelShaderResource);

    if (Resources.DirectionalShadowMask)
    {
        Resources.DirectionalShadowMask->SetDebugName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureInfo CascadeIndexBufferInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, Flags);
    Resources.CascadeIndexBuffer = FRHI::Get()->CreateTexture(CascadeIndexBufferInfo, EResourceAccess::NonPixelShaderResource);

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

    ShadowSettings.FilterSize    = Math::Max<float>(static_cast<float>(CVarCSMFilterSize.GetValue()), 1.0f);
    ShadowSettings.MaxFilterSize = Math::Max<float>(static_cast<float>(CVarCSMMaxFilterSize.GetValue()), 1.0f);
    ShadowSettings.ShadowMapSize = Resources.ShadowCascades->GetWidth();
    ShadowSettings.FrameIndex    = GetRenderer()->GetFrameCounter().GetFrameIndex();

    CommandList.TransitionBuffer(ShadowSettingsBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(ShadowSettingsBuffer.Get(), FBufferRegion(0, sizeof(FDirectionalShadowSettingsHLSL)), &ShadowSettings);
    CommandList.TransitionBuffer(ShadowSettingsBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

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
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
    }

    CommandList.SetComputePipelineState(PipelineStateInstance.PipelineState.Get());

    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), Resources.DirectionalLightDataBuffer.Get(), 1);
    CommandList.SetConstantBuffer(PipelineStateInstance.Shader.Get(), ShadowSettingsBuffer.Get(), 2);

    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.CascadeSplitsBufferSRV.Get(), 1);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(PipelineStateInstance.Shader.Get(), Resources.ShadowCascades->GetShaderResourceView(), 4);

    CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.DirectionalShadowMask->GetUnorderedAccessView(), 0);

    if (CVarCSMDebugCascades.GetValue())
    {
        CommandList.SetUnorderedAccessView(PipelineStateInstance.Shader.Get(), Resources.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
    }

    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerPointCmp.Get(), 0);
    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerLinearCmp.Get(), 1);
    CommandList.SetSamplerState(PipelineStateInstance.Shader.Get(), Resources.ShadowSamplerPoint.Get(), 2);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = Math::DivideByMultiple(Resources.DirectionalShadowMask->GetWidth(), NumThreads);
    const uint32 ThreadsY = Math::DivideByMultiple(Resources.DirectionalShadowMask->GetHeight(), NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
    if (CVarCSMDebugCascades.GetValue())
    {
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
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

    // Filter function
    if (Combination.FilterFunction == ECSMFilterFunction::Grid)
    {
        Defines.Emplace("FILTER_FUNCTION_GRID", "1");
        Defines.Emplace("FILTER_FUNCTION_POISSON_DISK", "0");
        Defines.Emplace("FILTER_FUNCTION_VOGEL_DISK", "0");
        DebugName += "Grid ";
    }
    else if (Combination.FilterFunction == ECSMFilterFunction::PoissonDisk)
    {
        Defines.Emplace("FILTER_FUNCTION_GRID", "0");
        Defines.Emplace("FILTER_FUNCTION_POISSON_DISK", "1");
        Defines.Emplace("FILTER_FUNCTION_VOGEL_DISK", "0");
        DebugName += "Poisson-Disk ";
    }
    else if (Combination.FilterFunction == ECSMFilterFunction::VogelDisk)
    {
        Defines.Emplace("FILTER_FUNCTION_GRID", "0");
        Defines.Emplace("FILTER_FUNCTION_POISSON_DISK", "0");
        Defines.Emplace("FILTER_FUNCTION_VOGEL_DISK", "1");
        DebugName += "Vogel-Disk ";
    }
    else
    {
        Defines.Emplace("FILTER_FUNCTION_GRID", "0");
        Defines.Emplace("FILTER_FUNCTION_POISSON_DISK", "0");
        Defines.Emplace("FILTER_FUNCTION_VOGEL_DISK", "0");
    }

    // Filter mode
    if (Combination.FilterMode == ECSMFilterMode::PCF)
    {
        Defines.Emplace("FILTER_MODE_PCF", "1");
        Defines.Emplace("FILTER_MODE_PCSS", "0");
        DebugName += " (PCF) ";
    }
    else if (Combination.FilterMode == ECSMFilterMode::PCSS)
    {
        Defines.Emplace("FILTER_MODE_PCF", "0");
        Defines.Emplace("FILTER_MODE_PCSS", "1");
        DebugName += " (PCSS) ";
    }
    else
    {
        Defines.Emplace("FILTER_MODE_PCF", "0");
        Defines.Emplace("FILTER_MODE_PCSS", "0");
    }

    // Debug-mode
    if (Combination.bDebugMode)
    {
        Defines.Emplace("ENABLE_DEBUG", "1");
        DebugName += " Debug ";
    }
    else
    {
        Defines.Emplace("ENABLE_DEBUG", "0");
    }

    // Rotate samples
    if (Combination.bRotateSamples)
    {
        Defines.Emplace("ROTATE_SAMPLES", "1");
        DebugName += " RotateSamples ";
    }
    else
    {
        Defines.Emplace("ROTATE_SAMPLES", "0");
    }

    // Select cascades from projection
    if (Combination.bSelectCascadeFromProjection)
    {
        Defines.Emplace("SELECT_CASCADE_FROM_PROJECTION", "1");
        DebugName += " CascadeFromProjection ";
    }
    else
    {
        Defines.Emplace("SELECT_CASCADE_FROM_PROJECTION", "0");
    }

    // Blend cascades
    if (Combination.bBlendCascades)
    {
        Defines.Emplace("ENABLE_CASCADE_BLENDING", "1");
        DebugName += " BlendCascades ";
    }
    else
    {
        Defines.Emplace("ENABLE_CASCADE_BLENDING", "0");
    }

    // Number of samples
    if (Combination.NumSamples <= 16)
    {
        Defines.Emplace("NUM_SAMPLES", "16");
        DebugName += " NumSamples=16";
    }
    else if (Combination.NumSamples <= 32)
    {
        Defines.Emplace("NUM_SAMPLES", "32");
        DebugName += " NumSamples=32";
    }
    else if (Combination.NumSamples <= 64)
    {
        Defines.Emplace("NUM_SAMPLES", "64");
        DebugName += " NumSamples=64";
    }
    else if (Combination.NumSamples <= 128)
    {
        Defines.Emplace("NUM_SAMPLES", "128");
        DebugName += " NumSamples=128";
    }

    DebugName += ")";

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Shadows/ShadowMaskGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FComputePipelineStateInstance PipelineStateInstance;
    PipelineStateInstance.Shader = FRHI::Get()->CreateComputeShader(ShaderCode);

    if (!PipelineStateInstance.Shader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInfo ShadowMaskGenPSOInfo;
    ShadowMaskGenPSOInfo.Shader = PipelineStateInstance.Shader.Get();

    PipelineStateInstance.PipelineState = FRHI::Get()->CreateComputePipelineState(ShadowMaskGenPSOInfo);
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
    OutCombination.FilterMode                   = static_cast<ECSMFilterMode>(Math::Clamp<int32>(CVarCSMFilterMode.GetValue(), 0, 1));
    OutCombination.FilterFunction               = static_cast<ECSMFilterFunction>(Math::Clamp<int32>(CVarCSMFilterFunction.GetValue(), 0, 2));
    OutCombination.bDebugMode                   = CVarCSMDebugCascades.GetValue();
    OutCombination.bBlendCascades               = CVarCSMBlendCascades.GetValue();
    OutCombination.bSelectCascadeFromProjection = CVarCSMSelectCascadeFromProjection.GetValue();
    OutCombination.bRotateSamples               = CVarCSMRotateSamples.GetValue();
    OutCombination.NumSamples                   = CVarCSMNumPoissonDiscSamples.GetValue();
}
