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
    , AABB_Depth_PSO(nullptr)
    , AABB_VS(nullptr)
    , AABB_PS(nullptr)
    , AABBInputLayout(nullptr)
    , AABB_NoDepthStencilState(nullptr)
    , AABB_DepthStencilState(nullptr)
    , AABBRasterizerState(nullptr)
    , AABBBlendState(nullptr)
    , AABBSolid_PSO(nullptr)
    , AABBSolid_VS(nullptr)
    , AABBSolid_PS(nullptr)
    , AABBSolidInputLayout(nullptr)
    , AABBSolidDepthStencilState(nullptr)
    , AABBSolidRasterizerState(nullptr)
    , AABBSolidBlendState(nullptr)
    , LightDebug_PSO(nullptr)
    , LightDebug_VS(nullptr)
    , LightDebug_PS(nullptr)
    , DebugSphereInputLayout(nullptr)
    , LightDebugDepthStencilState(nullptr)
    , LightDebugRasterizerState(nullptr)
    , LightDebugBlendState(nullptr)
    , ProbeDebug_PSO(nullptr)
    , ProbeDebug_VS(nullptr)
    , ProbeDebug_PS(nullptr)
    , ProbeDebugDepthStencilState(nullptr)
    , ProbeDebugRasterizerState(nullptr)
    , ProbeDebugBlendState(nullptr)
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
    AABB_Depth_PSO.Reset();
    AABB_VS.Reset();
    AABB_PS.Reset();
    AABBInputLayout.Reset();
    AABB_NoDepthStencilState.Reset();
    AABB_DepthStencilState.Reset();
    AABBRasterizerState.Reset();
    AABBBlendState.Reset();

    AABBSolid_PSO.Reset();
    AABBSolid_VS.Reset();
    AABBSolid_PS.Reset();
    AABBSolidInputLayout.Reset();
    AABBSolidDepthStencilState.Reset();
    AABBSolidRasterizerState.Reset();
    AABBSolidBlendState.Reset();

    LightDebug_PSO.Reset();
    LightDebug_VS.Reset();
    LightDebug_PS.Reset();
    DebugSphereInputLayout.Reset();
    LightDebugDepthStencilState.Reset();
    LightDebugRasterizerState.Reset();
    LightDebugBlendState.Reset();

    ProbeDebug_PSO.Reset();
    ProbeDebug_VS.Reset();
    ProbeDebug_PS.Reset();
    ProbeDebugDepthStencilState.Reset();
    ProbeDebugRasterizerState.Reset();
    ProbeDebugBlendState.Reset();
}

bool FDebugRenderer::Initialize(FFrameResources& /*Resources*/)
{
    FMeshCreateInfo SphereMesh = MeshFactory::CreateSphere(2, 0.35f);

    // VertexBuffer
    FRHIBufferDesc VertexBufferDesc;
    VertexBufferDesc.Stride = sizeof(FVertex);
    VertexBufferDesc.Size   = SphereMesh.Vertices.SizeInBytes();
    VertexBufferDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

    SphereVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, EResourceAccess::Common, SphereMesh.Vertices.Data());

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

    FRHIBufferDesc IndexBufferDesc;
    IndexBufferDesc.Stride = sizeof(uint16);
    IndexBufferDesc.Size   = SphereMeshSmallIndicies.SizeInBytes();
    IndexBufferDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    SphereIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, EResourceAccess::Common, SphereMeshSmallIndicies.Data());
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

    VertexBufferDesc.Stride = sizeof(FVector3);
    VertexBufferDesc.Size   = AABBVertices.SizeInBytes();
    VertexBufferDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

    AABBVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, EResourceAccess::Common, AABBVertices.Data());

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

    IndexBufferDesc.Stride = sizeof(uint16);
    IndexBufferDesc.Size   = AABBWireframeIndices.SizeInBytes();
    IndexBufferDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Wireframe = RHI::CreateBuffer(IndexBufferDesc, EResourceAccess::Common, AABBWireframeIndices.Data());

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

	IndexBufferDesc.Stride = sizeof(uint16);
	IndexBufferDesc.Size   = AABBSolidIndices.SizeInBytes();
	IndexBufferDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Solid = RHI::CreateBuffer(IndexBufferDesc, EResourceAccess::Common, AABBSolidIndices.Data());

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

        AABB_VS = RHI::CreateVertexShader(ShaderCode);
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

        AABB_PS = RHI::CreatePixelShader(ShaderCode);
        if (!AABB_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        AABBInputLayout = RHI::CreateInputLayout(InputElements);
        if (!AABBInputLayout)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = false;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        AABB_NoDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
        if (!AABB_NoDepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        DepthStencilStateDesc.bDepthEnable = true;

        AABB_DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
        if (!AABB_DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        AABBRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
        if (!AABBRasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        AABBBlendState = RHI::CreateBlendState(BlendStateDesc);
        if (!AABBBlendState)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    // Position-only input layout for debug sphere meshes (interleaved FVertex buffer, only Position used by shaders)
    TArray<FRHIInputElementDesc> DebugSphereElements =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertex), 0, 0, 0, EVertexInputClass::Vertex, 0 },
    };

    DebugSphereInputLayout = RHI::CreateInputLayout(DebugSphereElements);
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

        LightDebug_VS = RHI::CreateVertexShader(ShaderCode);
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

        LightDebug_PS = RHI::CreatePixelShader(ShaderCode);
        if (!LightDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        LightDebugDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
        if (!LightDebugDepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        LightDebugRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
        if (!LightDebugRasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        LightDebugBlendState = RHI::CreateBlendState(BlendStateDesc);
        if (!LightDebugBlendState)
        {
            DEBUG_BREAK();
            return false;
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

        AABBSolid_VS = RHI::CreateVertexShader(ShaderCode);
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

        AABBSolid_PS = RHI::CreatePixelShader(ShaderCode);
        if (!AABBSolid_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        AABBSolidInputLayout = RHI::CreateInputLayout(InputElements);
        if (!AABBSolidInputLayout)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        AABBSolidDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
        if (!AABBSolidDepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        AABBSolidRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
        if (!AABBSolidRasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.bIndependentBlendEnable        = false;
        BlendStateDesc.NumRenderTargets               = 1;
        BlendStateDesc.RenderTargets[0].bBlendEnable  = true;
        BlendStateDesc.RenderTargets[0].SrcBlend      = EBlendType::SrcAlpha;
        BlendStateDesc.RenderTargets[0].SrcBlendAlpha = EBlendType::InvSrcAlpha;
        BlendStateDesc.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
        BlendStateDesc.RenderTargets[0].DstBlendAlpha = EBlendType::Zero;
        BlendStateDesc.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
        BlendStateDesc.RenderTargets[0].BlendOp       = EBlendOp::Add;

        AABBSolidBlendState = RHI::CreateBlendState(BlendStateDesc);
        if (!AABBSolidBlendState)
        {
            DEBUG_BREAK();
            return false;
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

        ProbeDebug_VS = RHI::CreateVertexShader(ShaderCode);
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

        ProbeDebug_PS = RHI::CreatePixelShader(ShaderCode);
        if (!ProbeDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        ProbeDebugDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
        if (!ProbeDebugDepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::Back;

        ProbeDebugRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
        if (!ProbeDebugRasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        ProbeDebugBlendState = RHI::CreateBlendState(BlendStateDesc);
        if (!ProbeDebugBlendState)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    return true;
}

void FDebugRenderer::PreparePipelineState(EFormat OutputFormat)
{
    if (!AABB_NoDepth_PSO || AABB_NoDepth_PSOFormat != OutputFormat)
    {
        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = AABBBlendState.Get();
        PSODesc.DepthStencilState                              = AABB_NoDepthStencilState.Get();
        PSODesc.InputLayout                                    = AABBInputLayout.Get();
        PSODesc.RasterizerState                                = AABBRasterizerState.Get();
        PSODesc.VertexShader                                   = AABB_VS.Get();
        PSODesc.PixelShader                                    = AABB_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::LineList;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("AABB Wireframe Debug PSO (No Depth)");
            AABB_NoDepth_PSO       = NewPSO;
            AABB_NoDepth_PSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!AABB_Depth_PSO || AABB_Depth_PSOFormat != OutputFormat)
    {
        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = AABBBlendState.Get();
        PSODesc.DepthStencilState                              = AABB_DepthStencilState.Get();
        PSODesc.InputLayout                                    = AABBInputLayout.Get();
        PSODesc.RasterizerState                                = AABBRasterizerState.Get();
        PSODesc.VertexShader                                   = AABB_VS.Get();
        PSODesc.PixelShader                                    = AABB_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::LineList;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("AABB Wireframe Debug PSO (Depth)");
            AABB_Depth_PSO       = NewPSO;
            AABB_Depth_PSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!LightDebug_PSO || LightDebug_PSOFormat != OutputFormat)
    {
        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = LightDebugBlendState.Get();
        PSODesc.DepthStencilState                              = LightDebugDepthStencilState.Get();
        PSODesc.InputLayout                                    = DebugSphereInputLayout.Get();
        PSODesc.RasterizerState                                = LightDebugRasterizerState.Get();
        PSODesc.VertexShader                                   = LightDebug_VS.Get();
        PSODesc.PixelShader                                    = LightDebug_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("Light Debug PSO");
            LightDebug_PSO       = NewPSO;
            LightDebug_PSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!AABBSolid_PSO || AABBSolid_PSOFormat != OutputFormat)
    {
        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = AABBSolidBlendState.Get();
        PSODesc.DepthStencilState                              = AABBSolidDepthStencilState.Get();
        PSODesc.InputLayout                                    = AABBSolidInputLayout.Get();
        PSODesc.RasterizerState                                = AABBSolidRasterizerState.Get();
        PSODesc.VertexShader                                   = AABBSolid_VS.Get();
        PSODesc.PixelShader                                    = AABBSolid_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("AABB Solid Debug PSO");
            AABBSolid_PSO       = NewPSO;
            AABBSolid_PSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!ProbeDebug_PSO || ProbeDebug_PSOFormat != OutputFormat)
    {
        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = ProbeDebugBlendState.Get();
        PSODesc.DepthStencilState                              = ProbeDebugDepthStencilState.Get();
        PSODesc.InputLayout                                    = DebugSphereInputLayout.Get();
        PSODesc.RasterizerState                                = ProbeDebugRasterizerState.Get();
        PSODesc.VertexShader                                   = ProbeDebug_VS.Get();
        PSODesc.PixelShader                                    = ProbeDebug_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = RendererTextureFormats::DepthBufferFormat;

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("LightProbe Debug PSO");
            ProbeDebug_PSO       = NewPSO;
            ProbeDebug_PSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }
}

void FDebugRenderer::RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget, FRHITexture* InDepthTarget)
{
    RHI_EVENT_SCOPE(CommandList, "AABB DebugPass");

    TRACE_SCOPE("AABB DebugPass");

    FRHITexture* RenderTarget = InRenderTarget ? InRenderTarget : Resources.SceneTarget.Get();
    FRHITexture* DepthTex = InDepthTarget ? InDepthTarget : Resources.GBuffer[EGBufferIndex::Depth].Get();

    FRHIRenderTargetView* RenderTargetView = RenderTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = DepthTex->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(AABB_NoDepth_PSO.Get());
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

    FRHITexture* RenderTarget = InRenderTarget ? InRenderTarget : Resources.SceneTarget.Get();
    FRHITexture* DepthTex = InDepthTarget ? InDepthTarget : Resources.GBuffer[EGBufferIndex::Depth].Get();

    FRHIRenderTargetView* RenderTargetView = RenderTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = DepthTex->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(LightDebug_PSO.Get());
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

    FRHITexture* RenderTarget = InRenderTarget ? InRenderTarget : Resources.SceneTarget.Get();
    FRHITexture* DepthTex     = InDepthTarget ? InDepthTarget : Resources.GBuffer[EGBufferIndex::Depth].Get();

    FRHIRenderTargetView* RenderTargetView = RenderTarget->GetRenderTargetView();
    FRHIDepthStencilView* DepthStencilView = DepthTex->GetDepthStencilView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0]       = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets       = 1;
    RenderPassDesc.DepthStencilAttachment = FRHIDepthStencilAttachment(DepthStencilView, EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    // Draw box for the light-probe
    for (FSceneLightProbe* LightProbe : Scene->LightProbes)
    {
        // Only draw the box if we have enabled box-projection for this probe
        if (LightProbe->bBoxProjection)
        {
            // Draw the solid AABB
            CommandList.SetGraphicsPipelineState(AABBSolid_PSO.Get());
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
    CommandList.SetGraphicsPipelineState(ProbeDebug_PSO.Get());

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