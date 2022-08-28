#include "DebugRenderer.h"
#include "MeshDrawCommand.h"

#include "Core/Math/AABB.h"
#include "Core/Math/Matrix4.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include "Engine/Scene/Actor.h"
#include "Engine/Resources/Mesh.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDebugRenderer

bool FDebugRenderer::Init(FFrameResources& Resources)
{
    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    AABBVertexShader = RHICreateVertexShader(ShaderCode);
    if (!AABBVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    AABBPixelShader = RHICreatePixelShader(ShaderCode);
    if (!AABBPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexInputLayoutInitializer InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 },
    };

    FRHIVertexInputLayoutRef InputLayoutState = RHICreateVertexInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc      = EComparisonFunc::LessEqual;
    DepthStencilInitializer.bDepthEnable   = false;
    DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::Zero;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInfo;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInfo);
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
    PSOInitializer.ShaderState.VertexShader               = AABBVertexShader.Get();
    PSOInitializer.ShaderState.PixelShader                = AABBPixelShader.Get();
    PSOInitializer.PrimitiveTopologyType                  = EPrimitiveTopologyType::Line;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

    AABBDebugPipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!AABBDebugPipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName("Debug PipelineState");
    }

    TStaticArray<FVector3, 8> Vertices =
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

    FRHIBufferDataInitializer VertexData(Vertices.GetData(), Vertices.SizeInBytes());

    FRHIVertexBufferInitializer VBInitializer(EBufferUsageFlags::Default, Vertices.GetSize(), sizeof(FVector3), EResourceAccess::Common, &VertexData);
    AABBVertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!AABBVertexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetName("AABB VertexBuffer");
    }

    // Create IndexBuffer
    TStaticArray<uint16, 24> Indices =
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

    FRHIBufferDataInitializer IndexData(Indices.GetData(), Indices.SizeInBytes());

    FRHIIndexBufferInitializer IBInitializer(EBufferUsageFlags::Default, EIndexFormat::uint16, Indices.GetSize(), EResourceAccess::Common, &IndexData);
    AABBIndexBuffer = RHICreateIndexBuffer(IBInitializer);
    if (!AABBIndexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName("AABB IndexBuffer");
    }

    return true;
}

void FDebugRenderer::Release()
{
    AABBVertexBuffer.Reset();
    AABBIndexBuffer.Reset();
    AABBDebugPipelineState.Reset();
    AABBVertexShader.Reset();
    AABBPixelShader.Reset();
}

void FDebugRenderer::RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin DebugPass");

    TRACE_SCOPE("DebugPass");

    CommandList.SetGraphicsPipelineState(AABBDebugPipelineState.Get());

    CommandList.SetPrimitiveTopology(EPrimitiveTopology::LineList);

    CommandList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(AABBIndexBuffer.Get());

    for (const FMeshDrawCommand& Command : Resources.GlobalMeshDrawCommands)
    {
        FAABB& Box = Command.Mesh->BoundingBox;

        FVector3 Scale    = FVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        FVector3 Position = Box.GetCenter();

        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.x, Position.y, Position.z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.x, Scale.y, Scale.z).Transpose();
        FMatrix4 TransformMatrix   = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;
        TransformMatrix = TransformMatrix.Transpose();

        CommandList.Set32BitShaderConstants(AABBVertexShader.Get(), TransformMatrix.GetData(), 16);

        CommandList.DrawIndexedInstanced(24, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End DebugPass");
}