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
    FRHIBufferInfo VBInfo(SphereMesh.Vertices.SizeInBytes(), sizeof(FVertex), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
    SphereVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::Common, SphereMesh.Vertices.Data());

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

    FRHIBufferInfo IBInfo(SphereMeshSmallIndicies.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
    SphereIndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::Common, SphereMeshSmallIndicies.Data());
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

    VBInfo = FRHIBufferInfo(AABBVertices.SizeInBytes(), sizeof(FVector3), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
    AABBVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::Common, AABBVertices.Data());

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

    IBInfo = FRHIBufferInfo(AABBWireframeIndices.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
    AABBIndexBuffer_Wireframe = RHICreateBuffer(IBInfo, EResourceAccess::Common, AABBWireframeIndices.Data());

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

    IBInfo = FRHIBufferInfo(AABBSolidIndices.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
    AABBIndexBuffer_Solid = RHICreateBuffer(IBInfo, EResourceAccess::Common, AABBSolidIndices.Data());

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

        AABB_VS = RHICreateVertexShader(ShaderCode);
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

        AABB_PS = RHICreatePixelShader(ShaderCode);
        if (!AABB_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIVertexLayoutInitializerList VertexElementList =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIVertexLayoutRef InputLayoutState = RHICreateVertexLayout(VertexElementList);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilInitializer;
        DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable      = false;
        DepthStencilInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState_NoDepth = RHICreateDepthStencilState(DepthStencilInitializer);
        if (!DepthStencilState_NoDepth)
        {
            DEBUG_BREAK();
            return false;
        }

        DepthStencilInitializer.bDepthEnable = true;

        FRHIDepthStencilStateRef DepthStencilState_Depth = RHICreateDepthStencilState(DepthStencilInitializer);
        if (!DepthStencilState_Depth)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = DepthStencilState_NoDepth.Get();
        PSOInitializer.VertexInputLayout                      = InputLayoutState.Get();
        PSOInitializer.RasterizerState                        = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = AABB_VS.Get();
        PSOInitializer.ShaderState.PixelShader                = AABB_PS.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::LineList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        AABB_NoDepth_PSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!AABB_NoDepth_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABB_NoDepth_PSO->SetDebugName("AABB Wireframe Debug PSO (No Depth)");
        }

        PSOInitializer.DepthStencilState = DepthStencilState_Depth.Get();

        AABB_Depth_PSO = RHICreateGraphicsPipelineState(PSOInitializer);
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

        LightDebug_VS = RHICreateVertexShader(ShaderCode);
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

        LightDebug_PS = RHICreatePixelShader(ShaderCode);
        if (!LightDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
        PSOInitializer.VertexInputLayout                      = Resources.MeshInputLayout.Get();
        PSOInitializer.RasterizerState                        = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = LightDebug_VS.Get();
        PSOInitializer.ShaderState.PixelShader                = LightDebug_PS.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        LightDebug_PSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!LightDebug_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightDebug_PSO->SetDebugName("Light Debug PSO");
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

        AABBSolid_VS = RHICreateVertexShader(ShaderCode);
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

        AABBSolid_PS = RHICreatePixelShader(ShaderCode);
        if (!AABBSolid_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIVertexLayoutInitializerList VertexElementList =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIVertexLayoutRef InputLayoutState = RHICreateVertexLayout(VertexElementList);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.bIndependentBlendEnable        = false;
        BlendStateInitializer.NumRenderTargets               = 1;
        BlendStateInitializer.RenderTargets[0].bBlendEnable  = true;
        BlendStateInitializer.RenderTargets[0].SrcBlend      = EBlendType::SrcAlpha;
        BlendStateInitializer.RenderTargets[0].SrcBlendAlpha = EBlendType::InvSrcAlpha;
        BlendStateInitializer.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
        BlendStateInitializer.RenderTargets[0].DstBlendAlpha = EBlendType::Zero;
        BlendStateInitializer.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;
        BlendStateInitializer.RenderTargets[0].BlendOp       = EBlendOp::Add;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
        PSOInitializer.VertexInputLayout                      = InputLayoutState.Get();
        PSOInitializer.RasterizerState                        = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = AABBSolid_VS.Get();
        PSOInitializer.ShaderState.PixelShader                = AABBSolid_PS.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        AABBSolid_PSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!AABBSolid_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABBSolid_PSO->SetDebugName("AABB Solid Debug PSO");
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

        ProbeDebug_VS = RHICreateVertexShader(ShaderCode);
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

        ProbeDebug_PS = RHICreatePixelShader(ShaderCode);
        if (!ProbeDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
        PSOInitializer.VertexInputLayout                      = Resources.MeshInputLayout.Get();
        PSOInitializer.RasterizerState                        = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = ProbeDebug_VS.Get();
        PSOInitializer.ShaderState.PixelShader                = ProbeDebug_PS.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        ProbeDebug_PSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!ProbeDebug_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ProbeDebug_PSO->SetDebugName("LightProbe Debug PSO");
        }
    }

    return true;
}

void FDebugRenderer::RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin AABB DebugPass");

    TRACE_SCOPE("AABB DebugPass");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(AABB_NoDepth_PSO.Get());
    CommandList.SetConstantBuffer(AABB_VS.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(AABBIndexBuffer_Wireframe.Get(), EIndexFormat::uint16);

    for (const FSceneStaticMesh* StaticMesh : Scene->CameraView.GetStaticMeshes())
    {
        const FAABB& Box = StaticMesh->Mesh->GetAABB();
        FVector3 Scale    = FVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        FVector3 Position = Box.GetCenter();

        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.X, Position.Y, Position.Z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.X, Scale.Y, Scale.Z);
        FMatrix4 TransformMatrix   = StaticMesh->Actor->GetTransform().GetTransformMatrix();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;

        FAABBShaderInfoHLSL ShaderData;
        ShaderData.WorldMatrix = TransformMatrix.GetTranspose();
        ShaderData.Color       = FVector4(1.0f, 0.0f, 0.0f, 1.0f);

        constexpr uint32 NumConstants = sizeof(FAABBShaderInfoHLSL) / sizeof(uint32);
        CommandList.Set32BitShaderConstants(AABB_VS.Get(), &ShaderData, NumConstants);

        CommandList.DrawIndexedInstanced(AABBIndexCount_Wireframe, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End AABB DebugPass");
}

void FDebugRenderer::RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin PointLight DebugPass");

    TRACE_SCOPE("PointLight DebugPass");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

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
        CommandList.Set32BitShaderConstants(LightDebug_VS.Get(), &PointLightData, NumConstants);

        CommandList.DrawIndexedInstanced(SphereIndexCount, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End PointLight DebugPass");
}

void FDebugRenderer::RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin LightProbe DebugPass");

    TRACE_SCOPE("LightProbe DebugPass");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPass.NumRenderTargets = 1;
    RenderPass.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

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
            Scale.X = FMath::Max<float>(Scale.X, 0.005f);
            Scale.Y = FMath::Max<float>(Scale.Y, 0.005f);
            Scale.Z = FMath::Max<float>(Scale.Z, 0.005f);

            FVector3 Position          = BoundingBox.GetCenter();
            FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.X, Position.Y, Position.Z);
            FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.X, Scale.Y, Scale.Z);
            FMatrix4 TransformMatrix   = ScaleMatrix * TranslationMatrix;

            FAABBShaderInfoHLSL ShaderData;
            ShaderData.WorldMatrix = TransformMatrix.GetTranspose();
            ShaderData.Color       = FVector4(0.8f, 0.8f, 0.8f, 0.4f);

            CommandList.SetConstantBuffer(AABBSolid_VS.Get(), Resources.CameraBuffer.Get(), 0);

            constexpr uint32 NumConstants = sizeof(FAABBShaderInfoHLSL) / sizeof(uint32);
            CommandList.Set32BitShaderConstants(AABBSolid_VS.Get(), &ShaderData, NumConstants);

            CommandList.DrawIndexedInstanced(AABBIndexCount_Solid, 1, 0, 0, 0);

            // Draw the wireframe AABB
            CommandList.SetGraphicsPipelineState(AABB_Depth_PSO.Get());
            CommandList.SetConstantBuffer(AABB_VS.Get(), Resources.CameraBuffer.Get(), 0);
            CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
            CommandList.SetIndexBuffer(AABBIndexBuffer_Wireframe.Get(), EIndexFormat::uint16);

            ShaderData.Color = FVector4(0.7f, 0.7f, 0.7f, 1.0f);

            CommandList.Set32BitShaderConstants(AABB_VS.Get(), &ShaderData, NumConstants);

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
        CommandList.Set32BitShaderConstants(ProbeDebug_VS.Get(), &LightProbeData, NumConstants);

        if (LightProbe->SpecularCubeMap)
        {
            FRHIShaderResourceView* CubeMapSRV = LightProbe->SpecularCubeMap->GetShaderResourceView();
            CommandList.SetShaderResourceView(ProbeDebug_PS.Get(), CubeMapSRV, 0);
        }

        CommandList.SetSamplerState(ProbeDebug_PS.Get(), Resources.LightProbeSampler.Get(), 0);

        CommandList.DrawIndexedInstanced(SphereIndexCount, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End LightProbe DebugPass");
}