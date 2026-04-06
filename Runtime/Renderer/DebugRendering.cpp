#include "Core/Math/AABB.h"
#include "Core/Math/Matrix4.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RHIFence.h"
#include "Engine/World/Camera.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Lights/PointLight.h"
#include "Renderer/ShadowRendering.h"
#include "Renderer/DebugRendering.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include <cstring>
#include <cstddef>

struct FAABBShaderInfoHLSL
{
    FMatrix4 WorldMatrix;
    FVector4 Color;
};

static void BuildCameraFrustumPlaneCorners(const FCamera& Camera, float Depth, FVector3 OutCorners[4])
{
    const float ClampedDepth = Math::Max(Depth, 0.001f);
    float Aspect = Camera.GetAspectRatio();
    if (Camera.GetHeight() > 0.0f)
    {
        Aspect = Camera.GetWidth() / Camera.GetHeight();
    }

    if (Aspect <= 0.0f || Math::IsNaN(Aspect) || Math::IsInfinity(Aspect))
    {
        Aspect = 1.0f;
    }

    const float HalfFovDeg = Math::Clamp(Camera.GetFieldOfView() * 0.5f, 1.0f, 89.0f);
    const float HalfFovRad = Math::DegreesToRadians(HalfFovDeg);
    const float HalfHeight   = Math::Tan(HalfFovRad) * ClampedDepth;
    const float HalfWidth    = HalfHeight * Aspect;

    const FVector3 Center = Camera.GetPosition() + (Camera.GetForwardVector() * ClampedDepth);
    const FVector3 Right  = Camera.GetRightVector();
    const FVector3 Up     = Camera.GetUpVector();

    OutCorners[0] = Center - (Right * HalfWidth) - (Up * HalfHeight);
    OutCorners[1] = Center + (Right * HalfWidth) - (Up * HalfHeight);
    OutCorners[2] = Center + (Right * HalfWidth) + (Up * HalfHeight);
    OutCorners[3] = Center - (Right * HalfWidth) + (Up * HalfHeight);
}

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
    , CascadeFrustumDebug_PSO(nullptr)
    , CascadeFrustumDebug_VS(nullptr)
    , CascadeFrustumDebug_PS(nullptr)
    , CascadeSplitReadbackBuffer(nullptr)
    , CascadeSplitReadbackFence(nullptr)
    , bCascadeSplitReadbackInFlight(false)
    , bHasCascadeSplitDistances(false)
    , CascadeSplitReadbackSize(0)
    , CascadeSplitStrideBytes(0)
{
    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        CachedCascadeSplitStart[CascadeIndex] = 0.0f;
        CachedCascadeSplitEnd[CascadeIndex]   = 0.0f;
    }
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

    CascadeFrustumDebug_PSO.Reset();
    CascadeFrustumDebug_VS.Reset();
    CascadeFrustumDebug_PS.Reset();
    CascadeSplitReadbackBuffer.Reset();
    CascadeSplitReadbackFence.Reset();
}

bool FDebugRenderer::Initialize(FFrameResources& Resources)
{
    FMeshCreateInfo SphereMesh = FMeshFactory::CreateSphere(2, 0.35f);

    // VertexBuffer
    FRHIBufferDesc VBDesc;
    VBDesc.Stride = sizeof(FVertex);
    VBDesc.Size   = SphereMesh.Vertices.SizeInBytes();
    VBDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;
    VBDesc.bEnableResourceStateTracking = true;

    SphereVertexBuffer = FRHI::Get()->CreateBuffer(VBDesc, EResourceAccess::Common, SphereMesh.Vertices.Data());

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

    FRHIBufferDesc IBDesc;
    IBDesc.Stride = sizeof(uint16);
    IBDesc.Size   = SphereMeshSmallIndicies.SizeInBytes();
    IBDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;
    IBDesc.bEnableResourceStateTracking = true;

    SphereIndexBuffer = FRHI::Get()->CreateBuffer(IBDesc, EResourceAccess::Common, SphereMeshSmallIndicies.Data());
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

    VBDesc.Stride = sizeof(FVector3);
    VBDesc.Size   = AABBVertices.SizeInBytes();
    VBDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

    AABBVertexBuffer = FRHI::Get()->CreateBuffer(VBDesc, EResourceAccess::Common, AABBVertices.Data());

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

    IBDesc.Stride = sizeof(uint16);
    IBDesc.Size   = AABBWireframeIndices.SizeInBytes();
    IBDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Wireframe = FRHI::Get()->CreateBuffer(IBDesc, EResourceAccess::Common, AABBWireframeIndices.Data());

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

	IBDesc.Stride = sizeof(uint16);
	IBDesc.Size   = AABBSolidIndices.SizeInBytes();
	IBDesc.Flags  = EBufferFlags::IndexBuffer | EBufferFlags::Default;

    AABBIndexBuffer_Solid = FRHI::Get()->CreateBuffer(IBDesc, EResourceAccess::Common, AABBSolidIndices.Data());

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

        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIInputLayoutRef InputLayoutState = FRHI::Get()->CreateInputLayout(InputElements);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilDesc;
        DepthStencilDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilDesc.bDepthEnable      = false;
        DepthStencilDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState_NoDepth = FRHI::Get()->CreateDepthStencilState(DepthStencilDesc);
        if (!DepthStencilState_NoDepth)
        {
            DEBUG_BREAK();
            return false;
        }

        DepthStencilDesc.bDepthEnable = true;

        FRHIDepthStencilStateRef DepthStencilState_Depth = FRHI::Get()->CreateDepthStencilState(DepthStencilDesc);
        if (!DepthStencilState_Depth)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState_NoDepth.Get();
        PSODesc.InputLayout                                    = InputLayoutState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = AABB_VS.Get();
        PSODesc.PixelShader                                    = AABB_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::LineList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        AABB_NoDepth_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
        if (!AABB_NoDepth_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            AABB_NoDepth_PSO->SetDebugName("AABB Wireframe Debug PSO (No Depth)");
        }

        PSODesc.DepthStencilState = DepthStencilState_Depth.Get();

        AABB_Depth_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
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

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.InputLayout                                    = Resources.MeshInputLayout.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = LightDebug_VS.Get();
        PSODesc.PixelShader                                    = LightDebug_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        LightDebug_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
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

        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, 0, EVertexInputClass::Vertex, 0 },
        };

        FRHIInputLayoutRef InputLayoutState = FRHI::Get()->CreateInputLayout(InputElements);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!RasterizerState)
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

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.InputLayout                                    = InputLayoutState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = AABBSolid_VS.Get();
        PSODesc.PixelShader                                    = AABBSolid_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        AABBSolid_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
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

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = true;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.InputLayout                                    = Resources.MeshInputLayout.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = ProbeDebug_VS.Get();
        PSODesc.PixelShader                                    = ProbeDebug_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        ProbeDebug_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
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

    // Cascade Split Frustum Debug
    {
        TArray<FShaderDefine> CascadeFrustumDefines =
        {
            { "LINE_DEBUG", "(1)" }
        };

        FShaderCompileInfo CompileInfo("Line_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, CascadeFrustumDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        CascadeFrustumDebug_VS = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!CascadeFrustumDebug_VS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FShaderCompileInfo("Line_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, CascadeFrustumDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        CascadeFrustumDebug_PS = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!CascadeFrustumDebug_PS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float,      sizeof(FCascadeLineVertex), 0, static_cast<uint32>(offsetof(FCascadeLineVertex, Position)), 0, EVertexInputClass::Vertex, 0 },
            { "COLOR",    0, EFormat::R32G32B32A32_Float,   sizeof(FCascadeLineVertex), 0, static_cast<uint32>(offsetof(FCascadeLineVertex, Color)),    1, EVertexInputClass::Vertex, 0 },
        };

        FRHIInputLayoutRef InputLayoutState = FRHI::Get()->CreateInputLayout(InputElements);
        if (!InputLayoutState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateDesc DepthStencilStateDesc;
        DepthStencilStateDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateDesc.bDepthEnable      = false;
        DepthStencilStateDesc.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        RasterizerStateDesc.CullMode = ECullMode::None;

        FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = 1;

        FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.BlendState                                     = BlendState.Get();
        PSODesc.DepthStencilState                              = DepthStencilState.Get();
        PSODesc.InputLayout                                    = InputLayoutState.Get();
        PSODesc.RasterizerState                                = RasterizerState.Get();
        PSODesc.VertexShader                                   = CascadeFrustumDebug_VS.Get();
        PSODesc.PixelShader                                    = CascadeFrustumDebug_PS.Get();
        PSODesc.PrimitiveTopology                              = EPrimitiveTopology::LineList;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        CascadeFrustumDebug_PSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
        if (!CascadeFrustumDebug_PSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            CascadeFrustumDebug_PSO->SetDebugName("Cascade Split Frustum Debug PSO");
        }
    }

    return true;
}

void FDebugRenderer::RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin AABB DebugPass");

    TRACE_SCOPE("AABB DebugPass");

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

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

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End AABB DebugPass");
}

void FDebugRenderer::RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin PointLight DebugPass");

    TRACE_SCOPE("PointLight DebugPass");

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

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

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End PointLight DebugPass");
}

void FDebugRenderer::RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin LightProbe DebugPass");

    TRACE_SCOPE("LightProbe DebugPass");

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

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

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End LightProbe DebugPass");
}

void FDebugRenderer::UpdateCascadeSplitReadback()
{
    if (!bCascadeSplitReadbackInFlight || !CascadeSplitReadbackFence || !CascadeSplitReadbackBuffer)
    {
        return;
    }

    if (!CascadeSplitReadbackFence->IsSignaled())
    {
        return;
    }

    const uint64 BufferSize = CascadeSplitReadbackBuffer->GetDesc().Size;
    if (BufferSize < CascadeSplitReadbackSize || CascadeSplitStrideBytes == 0)
    {
        bCascadeSplitReadbackInFlight = false;
        return;
    }

    void* MappedData = CascadeSplitReadbackBuffer->Map(0, BufferSize);
    if (!MappedData)
    {
        bCascadeSplitReadbackInFlight = false;
        return;
    }

    const uint8* SplitBase = reinterpret_cast<const uint8*>(MappedData);
    const uint64 RequiredBytesPerSplit = sizeof(FVector4) * NumCascadeFrustumPlanes;
    bool bSawAnySplitDistance = false;

    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        const uint64 SplitOffset = uint64(CascadeIndex) * CascadeSplitStrideBytes;
        if ((SplitOffset + RequiredBytesPerSplit) > BufferSize)
        {
            break;
        }

        const uint8* SplitData = SplitBase + SplitOffset;
        std::memcpy(CachedCascadeFrustumPlanes[CascadeIndex], SplitData, RequiredBytesPerSplit);

        if (CascadeSplitStrideBytes >= sizeof(FCascadeSplitHLSL) && (SplitOffset + sizeof(FCascadeSplitHLSL)) <= BufferSize)
        {
            const FCascadeSplitHLSL* Split = reinterpret_cast<const FCascadeSplitHLSL*>(SplitData);
            CachedCascadeSplitStart[CascadeIndex] = Split->PreviousSplit;
            CachedCascadeSplitEnd[CascadeIndex]   = Split->Split;
            bSawAnySplitDistance = true;
        }
    }

    CascadeSplitReadbackBuffer->Unmap(0, BufferSize);

    bHasCascadeSplitDistances     = bSawAnySplitDistance;
    bCascadeSplitReadbackInFlight = false;
}

void FDebugRenderer::QueueCascadeSplitReadback(FRHICommandList& CommandList, FFrameResources& Resources)
{
    if (!Resources.CascadeSplitsBuffer)
    {
        return;
    }

    if (bCascadeSplitReadbackInFlight)
    {
        return;
    }

    const FRHIBufferDesc& CascadeSplitsDesc = Resources.CascadeSplitsBuffer->GetDesc();
    if (CascadeSplitsDesc.Size == 0 || CascadeSplitsDesc.Stride == 0)
    {
        return;
    }

    if (!CascadeSplitReadbackBuffer || CascadeSplitReadbackSize != CascadeSplitsDesc.Size)
    {
        FRHIBufferDesc ReadbackDesc;
        ReadbackDesc.Stride = sizeof(uint32);
        ReadbackDesc.Size   = CascadeSplitsDesc.Size;
        ReadbackDesc.Flags  = EBufferFlags::ReadBack;
        ReadbackDesc.bEnableResourceStateTracking = true;

        CascadeSplitReadbackBuffer = FRHI::Get()->CreateBuffer(ReadbackDesc, EResourceAccess::CopyDest, nullptr);
        if (!CascadeSplitReadbackBuffer)
        {
            return;
        }

        CascadeSplitReadbackBuffer->SetDebugName("CascadeSplit Debug Readback");
        CascadeSplitReadbackSize = CascadeSplitsDesc.Size;
    }

    if (!CascadeSplitReadbackFence)
    {
        CascadeSplitReadbackFence = FRHI::Get()->CreateFence();
        if (!CascadeSplitReadbackFence)
        {
            return;
        }

        CascadeSplitReadbackFence->SetDebugName("CascadeSplit Debug Readback Fence");
    }

    CascadeSplitStrideBytes = CascadeSplitsDesc.Stride;

    CommandList.RequireBufferState(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::CopySource);
    CommandList.RequireBufferState(CascadeSplitReadbackBuffer.Get(), EResourceAccess::CopyDest);

    FRHIBufferCopyDesc CopyDesc;
    CopyDesc.SrcOffset = 0;
    CopyDesc.DstOffset = 0;
    CopyDesc.Size      = CascadeSplitsDesc.Size;
    CommandList.CopyBuffer(CascadeSplitReadbackBuffer.Get(), Resources.CascadeSplitsBuffer.Get(), CopyDesc);

    CommandList.RequireBufferState(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource);
    CommandList.WriteFence(CascadeSplitReadbackFence.Get());

    bCascadeSplitReadbackInFlight = true;
}

bool FDebugRenderer::BuildCascadeFrustumVertices(const FFrameResources& Resources, const FScene* Scene, TArray<FCascadeLineVertex>& OutVertices) const
{
    if (!Scene || !Scene->Camera)
    {
        return false;
    }

    static const FVector4 CascadeColors[NUM_SHADOW_CASCADES] =
    {
        FVector4(1.0f, 0.2f, 0.2f, 1.0f),
        FVector4(0.2f, 1.0f, 0.2f, 1.0f),
        FVector4(0.2f, 0.5f, 1.0f, 1.0f),
        FVector4(1.0f, 0.9f, 0.2f, 1.0f),
    };

    const FVector4 NearPlaneColor(1.0f, 1.0f, 1.0f, 1.0f);
    const FVector4 FarPlaneColor(1.0f, 1.0f, 1.0f, 1.0f);
    const FVector4 SidePlaneColor(0.6f, 0.6f, 0.6f, 1.0f);

    static constexpr uint32 PlaneEdgeIndices[4][2] =
    {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
    };

    float PlaneDepths[NUM_SHADOW_CASCADES + 1] = {};
    const FCamera& Camera = *Scene->Camera;
    const float CameraNear = Math::Max(Camera.GetNearPlane(), 0.001f);
    const float CameraFar = Math::Max(Camera.GetFarPlane(), CameraNear + 0.001f);

    bool bHaveReadbackSplits = bHasCascadeSplitDistances;
    if (bHaveReadbackSplits)
    {
        PlaneDepths[0] = CachedCascadeSplitStart[0];
        for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
        {
            PlaneDepths[CascadeIndex + 1] = CachedCascadeSplitEnd[CascadeIndex];
        }
    }
    else
    {
        float NearDepth = CameraNear;
        float FarDepth = CameraFar;

        if (Scene->DirectionalLight)
        {
            const float CSMNearDistance = Math::Max(Resources.CascadeGenerationData.CSMNearDistance, 0.0f);
            if (CSMNearDistance > NearDepth)
            {
                NearDepth = Math::Min(CSMNearDistance, FarDepth);
            }

            const float MaxShadowDistance = Resources.CascadeGenerationData.MaxShadowDistance;
            if (MaxShadowDistance > NearDepth)
            {
                FarDepth = Math::Min(FarDepth, MaxShadowDistance);
            }
        }

        FarDepth = Math::Max(FarDepth, NearDepth + 0.001f);

        if (Resources.CascadeGenerationData.CascadeSplitMode != 0)
        {
            float ManualEnds[NUM_SHADOW_CASCADES] =
            {
                Math::Max(Resources.CascadeGenerationData.ManualCascadeSplitDistance0, 0.0f),
                Math::Max(Resources.CascadeGenerationData.ManualCascadeSplitDistance1, 0.0f),
                Math::Max(Resources.CascadeGenerationData.ManualCascadeSplitDistance2, 0.0f),
                FarDepth,
            };

            float PrevDepth = NearDepth;
            for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
            {
                float Distance = Math::Clamp(ManualEnds[CascadeIndex], NearDepth, FarDepth);
                Distance = Math::Max(Distance, PrevDepth + 0.001f);
                Distance = Math::Min(Distance, FarDepth);
                PlaneDepths[CascadeIndex + 1] = Distance;
                PrevDepth = Distance;
            }
        }
        else
        {
            const float Lambda = Math::Clamp(Resources.CascadeGenerationData.CascadeSplitLambda, 0.0f, 1.0f);
            const float ClipRange = FarDepth - NearDepth;
            for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
            {
                const float SplitRatio = float(CascadeIndex + 1) / float(NUM_SHADOW_CASCADES);
                const float LogScale = NearDepth * Math::Pow(FarDepth / NearDepth, SplitRatio);
                const float UniformScale = NearDepth + (ClipRange * SplitRatio);
                const float Distance = (Lambda * (LogScale - UniformScale)) + UniformScale;
                PlaneDepths[CascadeIndex + 1] = Distance;
            }
        }
        PlaneDepths[0] = NearDepth;
    }

    for (uint32 Index = 0; Index <= NUM_SHADOW_CASCADES; ++Index)
    {
        PlaneDepths[Index] = Math::Clamp(PlaneDepths[Index], CameraNear, CameraFar);
        if (Index > 0)
        {
            PlaneDepths[Index] = Math::Max(PlaneDepths[Index], PlaneDepths[Index - 1] + 0.001f);
        }
    }

    OutVertices.Clear();
    OutVertices.Reserve((NUM_SHADOW_CASCADES + 1) * 8 + (NUM_SHADOW_CASCADES * 8) + 8);

    auto AppendLine = [&](const FVector3& A, const FVector3& B, const FVector4& Color)
    {
        FCascadeLineVertex V0;
        V0.Position = A;
        V0.Color    = Color;
        OutVertices.Emplace(V0);

        FCascadeLineVertex V1;
        V1.Position = B;
        V1.Color    = Color;
        OutVertices.Emplace(V1);
    };

    FVector3 PlaneCorners[NUM_SHADOW_CASCADES + 1][4];
    for (uint32 PlaneIndex = 0; PlaneIndex <= NUM_SHADOW_CASCADES; ++PlaneIndex)
    {
        BuildCameraFrustumPlaneCorners(Camera, PlaneDepths[PlaneIndex], PlaneCorners[PlaneIndex]);
    }

    for (uint32 PlaneIndex = 0; PlaneIndex <= NUM_SHADOW_CASCADES; ++PlaneIndex)
    {
        FVector4 PlaneColor = NearPlaneColor;
        if (PlaneIndex == NUM_SHADOW_CASCADES)
        {
            PlaneColor = FarPlaneColor;
        }
        else if (PlaneIndex > 0)
        {
            PlaneColor = CascadeColors[(PlaneIndex - 1) & 3u];
        }

        for (uint32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
        {
            const uint32 A = PlaneEdgeIndices[EdgeIndex][0];
            const uint32 B = PlaneEdgeIndices[EdgeIndex][1];
            AppendLine(PlaneCorners[PlaneIndex][A], PlaneCorners[PlaneIndex][B], PlaneColor);
        }
    }

    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        const FVector4 CascadeColor = CascadeColors[CascadeIndex & 3u];
        for (uint32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
        {
            const FVector4 SideColor = (CornerIndex & 1u) ? SidePlaneColor : CascadeColor;
            AppendLine(PlaneCorners[CascadeIndex][CornerIndex], PlaneCorners[CascadeIndex + 1][CornerIndex], SideColor);
        }
    }

    const FVector3 CameraPosition = Camera.GetPosition();
    for (uint32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        AppendLine(CameraPosition, PlaneCorners[NUM_SHADOW_CASCADES][CornerIndex], SidePlaneColor);
    }

    return !OutVertices.IsEmpty();
}

void FDebugRenderer::RenderCascadeSplitFrustums(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene)
{
    if (!CascadeFrustumDebug_PSO || !CascadeFrustumDebug_VS || !CascadeFrustumDebug_PS)
    {
        return;
    }

    TArray<FCascadeLineVertex> LineVertices;
    if (!BuildCascadeFrustumVertices(Resources, Scene, LineVertices))
    {
        return;
    }

    FRHIBufferDesc VBDesc;
    VBDesc.Stride = sizeof(FCascadeLineVertex);
    VBDesc.Size   = LineVertices.SizeInBytes();
    VBDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;
    VBDesc.bEnableResourceStateTracking = true;

    FRHIBufferRef LineVertexBuffer = FRHI::Get()->CreateBuffer(VBDesc, EResourceAccess::Common, LineVertices.Data());
    if (!LineVertexBuffer)
    {
        return;
    }

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.RenderTargets[0] = FRHIRenderTargetView(Resources.FinalTarget.Get(), EAttachmentLoadAction::Load);
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.DepthStencilView = FRHIDepthStencilView(Resources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);
    CommandList.SetGraphicsPipelineState(CascadeFrustumDebug_PSO.Get());
    CommandList.SetConstantBuffer(CascadeFrustumDebug_VS.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetVertexBuffers(MakeArrayView(&LineVertexBuffer, 1), 0);
    CommandList.DrawInstanced(LineVertices.Size(), 1, 0, 0);
    CommandList.EndRenderPass();
}
