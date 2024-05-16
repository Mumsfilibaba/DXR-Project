#include "DebugRendering.h"
#include "Scene.h"
#include "Core/Math/AABB.h"
#include "Core/Math/Matrix4.h"
#include "Core/Misc/FrameProfiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"

bool FDebugRenderer::Initialize(FFrameResources& Resources)
{
    TArray<uint8> ShaderCode;
    
    {
        TArray<FShaderDefine> AABBDefines =
        {
            { "AABB_DEBUG", "(1)" }
        };
        
        {
            FShaderCompileInfo CompileInfo("AABB_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, AABBDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
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
            FShaderCompileInfo CompileInfo("AABB_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, AABBDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
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
        DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable      = false;
        DepthStencilInitializer.bDepthWriteEnable = false;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
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
        PSOInitializer.VertexInputLayout                      = InputLayoutState.Get();
        PSOInitializer.RasterizerState                        = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = AABBVertexShader.Get();
        PSOInitializer.ShaderState.PixelShader                = AABBPixelShader.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::LineList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = Resources.FinalTargetFormat;
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
            AABBDebugPipelineState->SetDebugName("Debug PipelineState");
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

        {
            FRHIBufferInfo VBInfo(Vertices.SizeInBytes(), sizeof(FVector3), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
            AABBVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::Common, Vertices.Data());
            if (!AABBVertexBuffer)
            {
                DEBUG_BREAK();
                return false;
            }
            else
            {
                AABBVertexBuffer->SetDebugName("AABB VertexBuffer");
            }
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

        {
            AABBIndexCount = Indices.Size();

            FRHIBufferInfo IBInfo(Indices.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
            AABBIndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::Common, Indices.Data());
            if (!AABBIndexBuffer)
            {
                DEBUG_BREAK();
                return false;
            }
        }
    }

    {
        TArray<FShaderDefine> PointLightDefines =
        {
            { "POINTLIGHT_DEBUG", "(1)" }
        };
        
        {
            FShaderCompileInfo CompileInfo("Light_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, PointLightDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        LightDebugVS = RHICreateVertexShader(ShaderCode);
        if (!LightDebugVS)
        {
            DEBUG_BREAK();
            return false;
        }

        {
            FShaderCompileInfo CompileInfo("Light_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, PointLightDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/Debug.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        LightDebugPS = RHICreatePixelShader(ShaderCode);
        if (!LightDebugPS)
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
        PSOInitializer.ShaderState.VertexShader               = LightDebugVS.Get();
        PSOInitializer.ShaderState.PixelShader                = LightDebugPS.Get();
        PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = Resources.FinalTargetFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = Resources.DepthBufferFormat;

        LightDebugPSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!LightDebugPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightDebugPSO->SetDebugName("Light Debug PipelineState");
        }

        FMeshData SphereMesh = FMeshFactory::CreateSphere(1, 0.25f);

        // VertexBuffer
        {
            FRHIBufferInfo VBInfo(SphereMesh.Vertices.SizeInBytes(), sizeof(FVertex), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
            DbgSphereVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::Common, SphereMesh.Vertices.Data());
            if (!DbgSphereVertexBuffer)
            {
                DEBUG_BREAK();
                return false;
            }
            else
            {
                DbgSphereVertexBuffer->SetDebugName("Light Debug VertexBuffer");
            }
        }

        // Create IndexBuffer
        {
            TArray<uint16> SmallIndicies = FMeshFactory::ConvertSmallIndices(SphereMesh.Indices);
            DbgSphereIndexCount = SmallIndicies.Size();

            FRHIBufferInfo IBInfo(SmallIndicies.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
            DbgSphereIndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::Common, SmallIndicies.Data());
            if (!DbgSphereIndexBuffer)
            {
                DEBUG_BREAK();
                return false;
            }
            else
            {
                DbgSphereIndexBuffer->SetDebugName("Light Debug IndexBuffer");
            }
        }
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

    LightDebugPSO.Reset();
    LightDebugVS.Reset();
    LightDebugPS.Reset();
    DbgSphereVertexBuffer.Reset();
    DbgSphereIndexBuffer.Reset();
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

    CommandList.SetGraphicsPipelineState(AABBDebugPipelineState.Get());

    CommandList.SetConstantBuffer(AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0);

    CommandList.SetVertexBuffers(MakeArrayView(&AABBVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(AABBIndexBuffer.Get(), EIndexFormat::uint16);

    for (const FProxySceneComponent* Component : Scene->VisiblePrimitives)
    {
        FAABB& Box = Component->Mesh->BoundingBox;

        FVector3 Scale    = FVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        FVector3 Position = Box.GetCenter();

        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.x, Position.y, Position.z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.x, Scale.y, Scale.z).Transpose();
        FMatrix4 TransformMatrix   = Component->CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;
        TransformMatrix = TransformMatrix.Transpose();

        CommandList.Set32BitShaderConstants(AABBVertexShader.Get(), TransformMatrix.Data(), 16);

        CommandList.DrawIndexedInstanced(AABBIndexCount, 1, 0, 0, 0);
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

    CommandList.SetGraphicsPipelineState(LightDebugPSO.Get());

    CommandList.SetConstantBuffer(LightDebugVS.Get(), Resources.CameraBuffer.Get(), 0);

    CommandList.SetVertexBuffers(MakeArrayView(&DbgSphereVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(DbgSphereIndexBuffer.Get(), EIndexFormat::uint16);

    struct FPointlightDebugData
    {
        FVector4 Color;
        FVector3 WorldPosition;
        float    Padding;
    } PointLightData;

    for (FLight* Light : Scene->Lights)
    {
        if (FPointLight* CurrentPointLight = Cast<FPointLight>(Light))
        {
            FVector3 Color = CurrentPointLight->GetColor();
            PointLightData.Color         = FVector4(Color.x, Color.y, Color.z, 1.0f);
            PointLightData.WorldPosition = CurrentPointLight->GetPosition();

            CommandList.Set32BitShaderConstants(LightDebugVS.Get(), &PointLightData, 8);

            CommandList.DrawIndexedInstanced(DbgSphereIndexCount, 1, 0, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End PointLight DebugPass");
}
