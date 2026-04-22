#include "Core/Math/AABB.h"
#include "Core/Math/Matrix4.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Lights/PointLight.h"
#include "Renderer/DebugRendering.h"
#include "Renderer/Scene/Scene.h"
#include "RendererCore/RenderSettings.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneStaticMesh.h"

struct FAABBShaderInfoHLSL
{
    FMatrix4 WorldMatrix;
    FVector4 Color;
};

FDebugRenderer::FDebugRenderer(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , AABBVertexBuffer(nullptr)
    , AABBIndexBuffer_Wireframe(nullptr)
    , AABBIndexBuffer_Solid(nullptr)
    , AABBIndexCount_Wireframe(0)
    , AABBIndexCount_Solid(0)
    , SphereVertexBuffer(nullptr)
    , SphereIndexBuffer(nullptr)
    , SphereIndexCount(0)
    , AABB_NoDepth_PSO(nullptr)
    , AABB_VS(nullptr)
    , AABB_PS(nullptr)
    , AABBSolid_PSO(nullptr)
    , AABBSolid_VS(nullptr)
    , AABBSolid_PS(nullptr)
    , LightDebug_PSO(nullptr)
    , LightDebug_VS(nullptr)
    , LightDebug_PS(nullptr)
    , ProbeDebug_PSO(nullptr)
    , ProbeDebug_VS(nullptr)
    , ProbeDebug_PS(nullptr)
{
}

FDebugRenderer::~FDebugRenderer()
{
    AABBVertexBuffer.Reset();
    AABBIndexBuffer_Wireframe.Reset();
    AABBIndexBuffer_Solid.Reset();

    SphereVertexBuffer.Reset();
    SphereIndexBuffer.Reset();

    AABB_NoDepth_PSO.Reset();
    AABB_VS.Reset();
    AABB_PS.Reset();

    AABBSolid_PSO.Reset();
    AABBSolid_VS.Reset();
    AABBSolid_PS.Reset();

    LightDebug_PSO.Reset();
    LightDebug_VS.Reset();
    LightDebug_PS.Reset();

    ProbeDebug_PSO.Reset();
    ProbeDebug_VS.Reset();
    ProbeDebug_PS.Reset();
}

bool FDebugRenderer::Initialize(FFrameResources& Resources)
{
    FMeshCreateInfo SphereMesh = FMeshFactory::CreateSphere(2, 0.35f);

    // VertexBuffer
    FRHIBufferInfo VBInfo;
    VBInfo.Stride = sizeof(FVertex);
    VBInfo.Size   = SphereMesh.Vertices.SizeInBytes();
    VBInfo.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

    SphereVertexBuffer = FRHI::Get()->CreateBuffer(VBInfo, EResourceAccess::Common, SphereMesh.Vertices.Data());

    if (!SphereVertexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SphereVertexBuffer->SetDebugName("Debug-Sphere VertexBuffer");
    }

    // Create IndexBuffer
    TArray<uint16> SphereMeshSmallIndicies = SphereMesh.GetSmallIndices();
    SphereIndexCount = SphereMeshSmallIndicies.Size();

    FRHIBufferInfo IBInfo;
    IBInfo.Stride = sizeof(uint16);
    IBInfo.Size   = SphereMeshSmallIndicies.SizeInBytes();
    IBInfo.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    SphereIndexBuffer = FRHI::Get()->CreateBuffer(IBInfo, EResourceAccess::Common, SphereMeshSmallIndicies.Data());
    if (!SphereIndexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SphereIndexBuffer->SetDebugName("Debug-Sphere IndexBuffer");
    }

    TStaticArray<FVector3, 8> AABBVertices =
    {
        FVector3(-0.5f, -0.5f,  0.5f),
        FVector3( 0.5f, -0.5f,  0.5f),
        FVector3(-0.5f,  0.5f,  0.5f),
        FVector3( 0.5f,  0.5f,  0.5f),
        FVector3( 0.5f, -0.5f, -0.5f),
        FVector3(-0.5f, -0.5f, -0.5f),
        FVector3( 0.5f,  0.5f, -0.5f),
        FVector3(-0.5f,  0.5f, -0.5f)
    };

    VBInfo.Stride = sizeof(FVector3);
    VBInfo.Size   = AABBVertices.SizeInBytes();
    VBInfo.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

    AABBVertexBuffer = FRHI::Get()->CreateBuffer(VBInfo, EResourceAccess::Common, AABBVertices.Data());

    if (!AABBVertexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetDebugName("Debug-AABB VertexBuffer");
    }

    // Create IndexBuffer
    TStaticArray<uint16, 24> AABBWireframeIndices =
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        1, 4,
        3, 6,
        6, 4,
        4, 5,
        5, 7,
        7, 6,
        0, 5,
        2, 7,
    };

    IBInfo.Stride = sizeof(uint16);
    IBInfo.Size   = AABBWireframeIndices.SizeInBytes();
    IBInfo.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Wireframe = FRHI::Get()->CreateBuffer(IBInfo, EResourceAccess::Common, AABBWireframeIndices.Data());

    if (!AABBIndexBuffer_Wireframe)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBIndexBuffer_Wireframe->SetDebugName("Debug-AABB IndexBuffer (Wireframe)");
        AABBIndexCount_Wireframe = AABBWireframeIndices.Size();
    }

    // Create IndexBuffer
    TStaticArray<uint16, 36> AABBSolidIndices =
    {
        // Front face
        0, 1, 2, 1, 3, 2,
        // Back face
        5, 4, 6, 5, 6, 7,
        // Left face
        5, 7, 0, 7, 2, 0,
        // Right face
        1, 4, 6, 1, 6, 3,
        // Top face
        2, 3, 6, 2, 6, 7,
        // Bottom face
        0, 5, 4, 0, 4, 1
    };

	IBInfo.Stride = sizeof(uint16);
	IBInfo.Size   = AABBSolidIndices.SizeInBytes();
	IBInfo.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Solid = FRHI::Get()->CreateBuffer(IBInfo, EResourceAccess::Common, AABBSolidIndices.Data());

    if (!AABBIndexBuffer_Solid)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBIndexBuffer_Solid->SetDebugName("Debug-AABB IndexBuffer (Solid)");
        AABBIndexCount_Solid = AABBSolidIndices.Size();
    }

    // Create PipelineStates
    TArray<uint8> ShaderCode;

    // AABB Wireframe
    {
        TArray<FShaderDefine> AABBDefines =
        {
            { "AABB_DEBUG", "(1)" }
        };

        FShaderCompileInfo CompileInfo("AABB_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, AABBDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        AABB_VS = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!AABB_VS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FShaderCompileInfo("AABB_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, AABBDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        AABB_PS = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!AABB_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FRHIInputElementInfo> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIInputLayoutRef InputLayoutState = FRHI::Get()->CreateInputLayout(InputElements);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInfo DepthStencilInfo;
        DepthStencilInfo.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInfo.bDepthEnable      = false;
        DepthStencilInfo.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState_NoDepth = FRHI::Get()->CreateDepthStencilState(DepthStencilInfo);
        if (!DepthStencilState_NoDepth)
        {
            DEBUG_BREAK();
            return false;
        }

        DepthStencilInfo.bDepthEnable = true;

        FRHIDepthStencilStateRef DepthStencilState_Depth = FRHI::Get()->CreateDepthStencilState(DepthStencilInfo);
        if (!DepthStencilState_Depth)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInfo BlendStateInfo;
        BlendStateInfo.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                                     = BlendState.Get();
        PSOInfo.DepthStencilState                              = DepthStencilState_NoDepth.Get();
        PSOInfo.InputLayout                                    = InputLayoutState.Get();
        PSOInfo.RasterizerState                                = RasterizerState.Get();
        PSOInfo.VertexShader                                   = AABB_VS.Get();
        PSOInfo.PixelShader                                    = AABB_PS.Get();
        PSOInfo.PrimitiveTopology                              = EPrimitiveTopology::LineList;
        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        AABB_NoDepth_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!AABB_NoDepth_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABB_NoDepth_PSO->SetDebugName("AABB Wireframe Debug PSO (No Depth)");
        }

        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
        AABB_NoDepth_PSO_BB = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!AABB_NoDepth_PSO_BB)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABB_NoDepth_PSO_BB->SetDebugName("AABB Wireframe Debug PSO (No Depth, BB)");
        }

        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInfo.DepthStencilState = DepthStencilState_Depth.Get();

        AABB_Depth_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!AABB_Depth_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABB_Depth_PSO->SetDebugName("AABB Wireframe Debug PSO (Depth)");
        }
    }

    // Position-only input layout for debug sphere meshes (interleaved FVertex buffer, only Position used by shaders)
    TArray<FRHIInputElementInfo> DebugSphereElements =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertex), 0, 0, 0, EVertexInputClass::Vertex, 0 },
    };

    FRHIInputLayoutRef DebugSphereInputLayout = FRHI::Get()->CreateInputLayout(DebugSphereElements);
    if (!DebugSphereInputLayout)
    {
        DEBUG_BREAK();
        return false;
    }

    // Point-Light Debug
    {
        TArray<FShaderDefine> PointLightDefines =
        {
            { "POINTLIGHT_DEBUG", "(1)" }
        };

        FShaderCompileInfo CompileInfo("Light_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, PointLightDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        LightDebug_VS = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!LightDebug_VS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FShaderCompileInfo("Light_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, PointLightDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        LightDebug_PS = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!LightDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInfo DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInfo BlendStateInfo;
        BlendStateInfo.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                                     = BlendState.Get();
        PSOInfo.DepthStencilState                              = DepthStencilState.Get();
        PSOInfo.InputLayout                                    = DebugSphereInputLayout.Get();
        PSOInfo.RasterizerState                                = RasterizerState.Get();
        PSOInfo.VertexShader                                   = LightDebug_VS.Get();
        PSOInfo.PixelShader                                    = LightDebug_PS.Get();
        PSOInfo.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        LightDebug_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!LightDebug_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightDebug_PSO->SetDebugName("Light Debug PSO");
        }

        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
        LightDebug_PSO_BB = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!LightDebug_PSO_BB)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightDebug_PSO_BB->SetDebugName("Light Debug PSO (BB)");
        }
    }

    // AABB Solid
    {
        TArray<FShaderDefine> AABBSolidDebugDefines =
        {
            { "AABB_SOLID_DEBUG", "(1)" }
        };

        FShaderCompileInfo CompileInfo("AABBSolidDebug_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, AABBSolidDebugDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        AABBSolid_VS = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!AABBSolid_VS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FShaderCompileInfo("AABBSolidDebug_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, AABBSolidDebugDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        AABBSolid_PS = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!AABBSolid_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FRHIInputElementInfo> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIInputLayoutRef InputLayoutState = FRHI::Get()->CreateInputLayout(InputElements);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInfo DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInfo BlendStateInfo;
        BlendStateInfo.bIndependentBlendEnable        = false;
        BlendStateInfo.NumRenderTargets               = 1;
        BlendStateInfo.RenderTargets[0].bBlendEnable  = true;
        BlendStateInfo.RenderTargets[0].SrcBlend      = EBlendType::SrcAlpha;
        BlendStateInfo.RenderTargets[0].SrcBlendAlpha = EBlendType::InvSrcAlpha;
        BlendStateInfo.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
        BlendStateInfo.RenderTargets[0].DstBlendAlpha = EBlendType::Zero;
        BlendStateInfo.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
        BlendStateInfo.RenderTargets[0].BlendOp       = EBlendOp::Add;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                                     = BlendState.Get();
        PSOInfo.DepthStencilState                              = DepthStencilState.Get();
        PSOInfo.InputLayout                                    = InputLayoutState.Get();
        PSOInfo.RasterizerState                                = RasterizerState.Get();
        PSOInfo.VertexShader                                   = AABBSolid_VS.Get();
        PSOInfo.PixelShader                                    = AABBSolid_PS.Get();
        PSOInfo.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        AABBSolid_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!AABBSolid_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABBSolid_PSO->SetDebugName("AABB Solid Debug PSO");
        }

        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
        AABBSolid_PSO_BB = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!AABBSolid_PSO_BB)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABBSolid_PSO_BB->SetDebugName("AABB Solid Debug PSO (BB)");
        }
    }

    // Light-Probe Debug
    {
        TArray<FShaderDefine> LightProbeDefines =
        {
            { "LIGHTPROBE_DEBUG", "(1)" }
        };

        FShaderCompileInfo CompileInfo("Probe_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, LightProbeDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        ProbeDebug_VS = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!ProbeDebug_VS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FShaderCompileInfo("Probe_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, LightProbeDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        ProbeDebug_PS = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!ProbeDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInfo DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInfo BlendStateInfo;
        BlendStateInfo.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInfo PSOInfo;
        PSOInfo.BlendState                                     = BlendState.Get();
        PSOInfo.DepthStencilState                              = DepthStencilState.Get();
        PSOInfo.InputLayout                                    = DebugSphereInputLayout.Get();
        PSOInfo.RasterizerState                                = RasterizerState.Get();
        PSOInfo.VertexShader                                   = ProbeDebug_VS.Get();
        PSOInfo.PixelShader                                    = ProbeDebug_PS.Get();
        PSOInfo.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        ProbeDebug_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!ProbeDebug_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ProbeDebug_PSO->SetDebugName("LightProbe Debug PSO");
        }

        PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
        ProbeDebug_PSO_BB = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
        if (!ProbeDebug_PSO_BB)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ProbeDebug_PSO_BB->SetDebugName("LightProbe Debug PSO (BB)");
        }
    }

    return true;
}

void FDebugRenderer::RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget, FRHITexture* InDepthTarget)
{
    RHI_EVENT_SCOPE(CommandList, "AABB DebugPass");

    TRACE_SCOPE("AABB DebugPass");

    FRHITexture* RT = InRenderTarget ? InRenderTarget : Resources.FinalTarget.Get();
    FRHITexture* DepthTex = InDepthTarget ? InDepthTarget : Resources.GBuffer[GBufferIndex_Depth].Get();
    const bool bBackBuffer = (RT->GetFormat() == RenderSettings::GetBackBufferFormat());

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(RT, EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(DepthTex, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(bBackBuffer ? AABB_NoDepth_PSO_BB.Get() : AABB_NoDepth_PSO.Get());
    CommandList.SetConstantBuffer(AABB_VS.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(AABBIndexBuffer_Wireframe.Get(), EIndexFormat::uint16);

    for (const FSceneStaticMesh* StaticMesh : Scene->CameraView.GetStaticMeshes())
    {
        const FAABB& WorldBounds = StaticMesh->GetWorldBounds();

        FVector3 Scale    = FVector3(WorldBounds.GetWidth(), WorldBounds.GetHeight(), WorldBounds.GetDepth());
        FVector3 Position = WorldBounds.GetCenter();

        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.X, Position.Y, Position.Z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.X, Scale.Y, Scale.Z);
        FMatrix4 TransformMatrix   = ScaleMatrix * TranslationMatrix;

        FAABBShaderInfoHLSL ShaderData;
        ShaderData.WorldMatrix = TransformMatrix.GetTranspose();
        ShaderData.Color       = FVector4(1.0f, 0.0f, 0.0f, 1.0f);

        constexpr uint32 NumConstants = sizeof(FAABBShaderInfoHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(AABB_VS.Get(), &ShaderData, NumConstants);

        CommandList.DrawIndexedInstanced(AABBIndexCount_Wireframe, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();
}

void FDebugRenderer::RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget, FRHITexture* InDepthTarget)
{
    RHI_EVENT_SCOPE(CommandList, "PointLight DebugPass");

    TRACE_SCOPE("PointLight DebugPass");

    FRHITexture* RT = InRenderTarget ? InRenderTarget : Resources.FinalTarget.Get();
    FRHITexture* DepthTex = InDepthTarget ? InDepthTarget : Resources.GBuffer[GBufferIndex_Depth].Get();
    const bool bBackBuffer = (RT->GetFormat() == RenderSettings::GetBackBufferFormat());

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(RT, EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(DepthTex, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(bBackBuffer ? LightDebug_PSO_BB.Get() : LightDebug_PSO.Get());
    CommandList.SetConstantBuffer(LightDebug_VS.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetVertexBuffers(MakeArrayView(&SphereVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SphereIndexBuffer.Get(), EIndexFormat::uint16);

    struct FPointlightDebugData
    {
        FVector4 Color;
        FVector3 WorldPosition;
        float    Padding;
    } PointLightData;

    for (FScenePointLight* PointLight : Scene->PointLights)
    {
        PointLightData.Color         = FVector4(PointLight->Color.X, PointLight->Color.Y, PointLight->Color.Z, 1.0f);
        PointLightData.WorldPosition = PointLight->Position;

        constexpr uint32 NumConstants = sizeof(FPointlightDebugData) / sizeof(uint32);
        CommandList.SetShaderConstants(LightDebug_VS.Get(), &PointLightData, NumConstants);

        CommandList.DrawIndexedInstanced(SphereIndexCount, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();
}

void FDebugRenderer::RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget, FRHITexture* InDepthTarget)
{
    RHI_EVENT_SCOPE(CommandList, "LightProbe DebugPass");

    TRACE_SCOPE("LightProbe DebugPass");

    FRHITexture* RT = InRenderTarget ? InRenderTarget : Resources.FinalTarget.Get();
    FRHITexture* DepthTex = InDepthTarget ? InDepthTarget : Resources.GBuffer[GBufferIndex_Depth].Get();
    const bool bBackBuffer = (RT->GetFormat() == RenderSettings::GetBackBufferFormat());

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(RT, EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(DepthTex, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    // Draw box for the light-probe
    for (FSceneLightProbe* LightProbe : Scene->LightProbes)
    {
        // Only draw the box if we have enabled box-projection for this probe
        if (LightProbe->bBoxProjection)
        {
            // Draw the solid AABB
            CommandList.SetGraphicsPipelineState(bBackBuffer ? AABBSolid_PSO_BB.Get() : AABBSolid_PSO.Get());
            CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
            CommandList.SetIndexBuffer(AABBIndexBuffer_Solid.Get(), EIndexFormat::uint16);

            FAABB BoundingBox(LightProbe->BoxMin, LightProbe->BoxMax);

            FVector3 Scale = FVector3(BoundingBox.GetWidth(), BoundingBox.GetHeight(), BoundingBox.GetDepth());
            Scale.X = Math::Max<float>(Scale.X, 0.005f);
            Scale.Y = Math::Max<float>(Scale.Y, 0.005f);
            Scale.Z = Math::Max<float>(Scale.Z, 0.005f);

            FVector3 Position          = BoundingBox.GetCenter();
            FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.X, Position.Y, Position.Z);
            FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.X, Scale.Y, Scale.Z);
            FMatrix4 TransformMatrix   = ScaleMatrix * TranslationMatrix;

            FAABBShaderInfoHLSL ShaderData;
            ShaderData.WorldMatrix = TransformMatrix.GetTranspose();
            ShaderData.Color       = FVector4(0.8f, 0.8f, 0.8f, 0.4f);

            CommandList.SetConstantBuffer(AABBSolid_VS.Get(), Resources.CameraBuffer.Get(), 0);

            constexpr uint32 NumConstants = sizeof(FAABBShaderInfoHLSL) / sizeof(uint32);
            CommandList.SetShaderConstants(AABBSolid_VS.Get(), &ShaderData, NumConstants);

            CommandList.DrawIndexedInstanced(AABBIndexCount_Solid, 1, 0, 0, 0);

            // Draw the wireframe AABB
            CommandList.SetGraphicsPipelineState(AABB_Depth_PSO.Get());
            CommandList.SetConstantBuffer(AABB_VS.Get(), Resources.CameraBuffer.Get(), 0);
            CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
            CommandList.SetIndexBuffer(AABBIndexBuffer_Wireframe.Get(), EIndexFormat::uint16);

            ShaderData.Color = FVector4(0.7f, 0.7f, 0.7f, 1.0f);

            CommandList.SetShaderConstants(AABB_VS.Get(), &ShaderData, NumConstants);

            CommandList.DrawIndexedInstanced(AABBIndexCount_Wireframe, 1, 0, 0, 0);
        }
    }

    // Draw the probe
    CommandList.SetGraphicsPipelineState(bBackBuffer ? ProbeDebug_PSO_BB.Get() : ProbeDebug_PSO.Get());

    CommandList.SetConstantBuffer(ProbeDebug_VS.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(ProbeDebug_PS.Get(), Resources.CameraBuffer.Get(), 0);

    CommandList.SetVertexBuffers(MakeArrayView(&SphereVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(SphereIndexBuffer.Get(), EIndexFormat::uint16);

    struct FLightProbeDebugData
    {
        FVector3 WorldPosition;
        float    Padding;
    } LightProbeData;

    for (FSceneLightProbe* LightProbe : Scene->LightProbes)
    {
        LightProbeData.WorldPosition = LightProbe->Origin;

        constexpr uint32 NumConstants = sizeof(FLightProbeDebugData) / sizeof(uint32);
        CommandList.SetShaderConstants(ProbeDebug_VS.Get(), &LightProbeData, NumConstants);

        if (LightProbe->SpecularCubeMap)
        {
            FRHIShaderResourceView* CubeMapSRV = LightProbe->SpecularCubeMap->GetShaderResourceView();
            CommandList.SetShaderResourceView(ProbeDebug_PS.Get(), CubeMapSRV, 0);
        }

        CommandList.SetSamplerState(ProbeDebug_PS.Get(), Resources.LightProbeSampler.Get(), 0);

        CommandList.DrawIndexedInstanced(SphereIndexCount, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();
}