#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/DeferredRendering.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

static TAutoConsoleVariable<bool> CVarDrawTileDebug(
    "Renderer.Debug.DrawTiledLightning", 
    "Draws the tiled lightning overlay, that displays how many lights are used in a certain tile", 
    false);

static TAutoConsoleVariable<bool> CVarBasePassClearAllTargets(
    "Renderer.BasePass.ClearAllTargets",
    "Set to true to clear all the GBuffer RenderTargets inside of the BasePass, otherwise only a few targets are cleared to save bandwidth",
    true);

FDepthPrePass::FDepthPrePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FDepthPrePass::~FDepthPrePass()
{
    MaterialPSOs.Clear();
}

void FDepthPrePass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const int32 MaterialFlags = static_cast<int32>(Material->GetMaterialFlags());
    if (MaterialPSOs.Find(MaterialFlags))
    {
        return;
    }

    TArray<uint8>         ShaderCode;
    TArray<FShaderDefine> ShaderDefines;

    ShaderDefines.Emplace("USE_UNJITTERED_CAMERA", "(0)");

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

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return;
    }

    FGraphicsPipelineStateInstance NewPipelineInstance;
    NewPipelineInstance.VertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);
    if (!NewPipelineInstance.VertexShader)
    {
        DEBUG_BREAK();
        return;
    }

    const bool bWantPixelShader = Material->HasHeightMap() || Material->HasPackedDiffuseAlpha() || Material->HasAlphaMask();
    if (bWantPixelShader)
    {
        CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.PixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!NewPipelineInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Less;
    DepthStencilStateDesc.bDepthEnable      = true;
    DepthStencilStateDesc.bDepthWriteEnable = true;

    NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!NewPipelineInstance.DepthStencilState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    if (Material->IsDoubleSided())
    {
        RasterizerStateDesc.CullMode = ECullMode::None;
    }
    else
    {
        RasterizerStateDesc.CullMode = ECullMode::Back;
    }

    NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
    if (!NewPipelineInstance.RasterizerState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIBlendStateDesc BlendStateDesc;
    NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
    if (!NewPipelineInstance.BlendState)
    {
        DEBUG_BREAK();
        return;
    }

    if (Material->HasHeightMap())
    {
        NewPipelineInstance.InputLayout = FrameResources.MeshInputLayout;
    }
    else if (Material->HasAlphaMask() || Material->HasPackedDiffuseAlpha())
    {
        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
            { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
        };

        NewPipelineInstance.InputLayout = FRHI::Get()->CreateInputLayout(InputElements);
        if (!NewPipelineInstance.InputLayout)
        {
            DEBUG_BREAK();
            return;
        }
    }
    else
    {
        TArray<FRHIInputElementDesc> InputElements =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
        };

        NewPipelineInstance.InputLayout = FRHI::Get()->CreateInputLayout(InputElements);
        if (!NewPipelineInstance.InputLayout)
        {
            DEBUG_BREAK();
            return;
        }
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                = NewPipelineInstance.InputLayout.Get();
    PSODesc.BlendState                                 = NewPipelineInstance.BlendState.Get();
    PSODesc.DepthStencilState                          = NewPipelineInstance.DepthStencilState.Get();
    PSODesc.RasterizerState                            = NewPipelineInstance.RasterizerState.Get();
    PSODesc.VertexShader                               = NewPipelineInstance.VertexShader.Get();
    PSODesc.PixelShader                                = NewPipelineInstance.PixelShader.Get();
    PSODesc.RasterizerOutputFormats.DepthStencilFormat = GlobalTextureFormats::DepthBufferFormat;

    NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!NewPipelineInstance.PipelineState)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("PrePass PSO %d", MaterialFlags);
        NewPipelineInstance.PipelineState->SetDebugName(DebugName);
    }

    MaterialPSOs.Add(MaterialFlags, Move(NewPipelineInstance));
}

bool FDepthPrePass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FDepthPrePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue DepthClearValue(GlobalTextureFormats::DepthBufferFormat, 1.0f, 0);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(GlobalTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    TextureDesc.bEnableResourceStateTracking = true;

    FrameResources.GBuffer[GBufferIndex_Depth] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Depth])
    {
        FrameResources.GBuffer[GBufferIndex_Depth]->SetDebugName("GBuffer DepthStencil");
    }
    else
    {
        return false;
    }

    return true;
}

void FDepthPrePass::ExecuteInternal(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene, FRHITexture* DepthTarget, const char* PassName)
{
    if (!DepthTarget)
    {
        return;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, PassName);

    TRACE_SCOPE(PassName);

    GPU_TRACE_SCOPE(CommandList, PassName);

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.DepthStencilView = FRHIDepthStencilView(DepthTarget);

    CommandList.BeginRenderPass(RenderPassDesc);

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    for (const FMeshBatch& Batch : Scene->CameraView.GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        CHECK(Material != nullptr);

        if (!Material->ShouldRenderInPrePass())
        {
            continue;
        }

        FGraphicsPipelineStateInstance* PipelineInstance = MaterialPSOs.Find(static_cast<int32>(Material->GetMaterialFlags()));
        if (!PipelineInstance)
        {
            DEBUG_BREAK();
        }

        FRHIGraphicsPipelineState* PipelineState = PipelineInstance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (Material->HasAlphaMask() || Material->HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
            CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
        }

        if (Material->HasAlphaMask())
        {
            if (Material->IsPackedMaterial())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            }
            else
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlphaMask->GetShaderResourceView(), 0);
            }
        }

        if (Material->HasHeightMap())
        {
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
            if (Material->HasHeightMap())
            {
                FRHIBuffer* VertexBuffers[] =
                {
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Normals),
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                };
                
                CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 3), 0);
            }
            else if (Material->HasAlphaMask() || Material->HasPackedDiffuseAlpha())
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

            constexpr uint32 NumConstants = sizeof(FTransformBufferHLSL) / sizeof(uint32);
            CommandList.SetShaderConstants(PipelineInstance->VertexShader.Get(), &StaticMesh->GetTransformShaderData(), NumConstants);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Pre-Pass");
}

void FDepthPrePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    ExecuteInternal(CommandList, FrameResources, Scene, FrameResources.GBuffer[GBufferIndex_Depth].Get(), "Depth Pre-Pass");
}

FDeferredBasePass::FDeferredBasePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FDeferredBasePass::~FDeferredBasePass()
{
    MaterialPSOs.Clear();
}

void FDeferredBasePass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const int32 MaterialFlags = static_cast<int32>(Material->GetMaterialFlags());

    FGraphicsPipelineStateInstance* CachedBasePassPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedBasePassPSO)
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

        if (Material->HasNormalMap())
        {
            ShaderDefines.Emplace("ENABLE_NORMAL_MAPPING", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_NORMAL_MAPPING", "(0)");
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

        if (Material->IsDoubleSided())
        {
            ShaderDefines.Emplace("ENABLE_DOUBLE_SIDED", "(1)");
        }
        else
        {
            ShaderDefines.Emplace("ENABLE_DOUBLE_SIDED", "(0)");
        }

        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FGraphicsPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);
        if (!NewPipelineInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        NewPipelineInstance.PixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
        if (!NewPipelineInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateDesc DepthStencilDesc;
        DepthStencilDesc.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilDesc.bDepthEnable      = true;
        DepthStencilDesc.bDepthWriteEnable = false;

        NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilDesc);
        if (!NewPipelineInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateDesc RasterizerStateDesc;
        if (Material->IsDoubleSided())
        {
            RasterizerStateDesc.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateDesc.CullMode = ECullMode::Back;
        }

        NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
        if (!NewPipelineInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateDesc BlendStateDesc;
        BlendStateDesc.NumRenderTargets = GBuffer_NumRenderTargets;

        NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
        if (!NewPipelineInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        // NOTE: Always use the default InputLayout
        NewPipelineInstance.InputLayout = FrameResources.MeshInputLayout;

        FRHIGraphicsPipelineStateDesc PSODesc;
        PSODesc.InputLayout                                    = NewPipelineInstance.InputLayout.Get();
        PSODesc.BlendState                                     = NewPipelineInstance.BlendState.Get();
        PSODesc.DepthStencilState                              = NewPipelineInstance.DepthStencilState.Get();
        PSODesc.RasterizerState                                = NewPipelineInstance.RasterizerState.Get();
        PSODesc.VertexShader                                   = NewPipelineInstance.VertexShader.Get();
        PSODesc.PixelShader                                    = NewPipelineInstance.PixelShader.Get();
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::AlbedoFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[1] = GlobalTextureFormats::NormalFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[2] = GlobalTextureFormats::MaterialFormat;
        PSODesc.RasterizerOutputFormats.RenderTargetFormats[3] = GlobalTextureFormats::VelocityFormat;
        PSODesc.RasterizerOutputFormats.NumRenderTargets       = GBuffer_NumRenderTargets;
        PSODesc.RasterizerOutputFormats.DepthStencilFormat     = GlobalTextureFormats::DepthBufferFormat;

        NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
        if (!NewPipelineInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("BasePass PipelineState %d", MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineInstance));
    }
}

bool FDeferredBasePass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FDeferredBasePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(GlobalTextureFormats::AlbedoFormat, Width, Height, 1, 1, Usage);
    TextureDesc.bEnableResourceStateTracking = true;

    // Albedo
    FrameResources.GBuffer[GBufferIndex_Albedo] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Albedo])
    {
        FrameResources.GBuffer[GBufferIndex_Albedo]->SetDebugName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureDesc.Format = GlobalTextureFormats::NormalFormat;

    FrameResources.GBuffer[GBufferIndex_Normal] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Normal])
    {
        FrameResources.GBuffer[GBufferIndex_Normal]->SetDebugName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureDesc.Format = GlobalTextureFormats::MaterialFormat;

    FrameResources.GBuffer[GBufferIndex_Material] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Material])
    {
        FrameResources.GBuffer[GBufferIndex_Material]->SetDebugName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // Velocity
    TextureDesc.Format = GlobalTextureFormats::VelocityFormat;

    FrameResources.GBuffer[GBufferIndex_Velocity] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Velocity])
    {
        FrameResources.GBuffer[GBufferIndex_Velocity]->SetDebugName("GBuffer Velocity");
    }
    else
    {
        return false;
    }

    return true;
}

void FDeferredBasePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Deferred BasePass");

    TRACE_SCOPE("Deferred BasePass");

    GPU_TRACE_SCOPE(CommandList, "Deferred BasePass");

    const float RenderWidth  = float(FrameResources.CurrentRenderWidth);
    const float RenderHeight = float(FrameResources.CurrentRenderHeight);

    const EAttachmentLoadAction LoadAction = CVarBasePassClearAllTargets.GetValue() ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets                     = GBuffer_NumRenderTargets;
    RenderPassDesc.RenderTargets[GBufferIndex_Albedo]   = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Albedo].Get(), LoadAction);
    RenderPassDesc.RenderTargets[GBufferIndex_Normal]   = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Normal].Get(), EAttachmentLoadAction::Clear);
    RenderPassDesc.RenderTargets[GBufferIndex_Material] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Material].Get(), LoadAction);
    RenderPassDesc.RenderTargets[GBufferIndex_Velocity] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Velocity].Get(), LoadAction);
    RenderPassDesc.DepthStencilView                     = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPassDesc);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    for (const FMeshBatch& Batch : Scene->CameraView.GetMeshBatches())
    {
        FMaterial* Material = Batch.Material;
        if (Material->ShouldRenderInForwardPass())
        {
            continue;
        }

        FGraphicsPipelineStateInstance* PipelineInstance = MaterialPSOs.Find(static_cast<int32>(Material->GetMaterialFlags()));
        if (!PipelineInstance)
        {
            DEBUG_BREAK();
        }

        FRHIGraphicsPipelineState* PipelineState = PipelineInstance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (Material->IsPackedMaterial())
        {
            // Setup resources after the PipelineState since binding a pipeline invalidates all resources
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            if (Material->HasNormalMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->NormalMap->GetShaderResourceView(), 1);
            }

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->SpecularMap->GetShaderResourceView(), 2);

            if (Material->HasHeightMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 3);
            }
        }
        else
        {
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            if (Material->HasNormalMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->NormalMap->GetShaderResourceView(), 1);
            }

            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->RoughnessMap->GetShaderResourceView(), 2);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->MetallicMap->GetShaderResourceView(), 3);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AOMap->GetShaderResourceView(), 4);

            if (Material->HasAlphaMask())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlphaMask->GetShaderResourceView(), 5);
            }
            if (Material->HasHeightMap())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 6);
            }
        }

        FRHIBuffer* PSConstantBuffers[] =
        {
            FrameResources.CameraBuffer.Get(),
            Material->GetMaterialBuffer(),
        };

        CommandList.SetConstantBuffers(PipelineInstance->PixelShader.Get(), MakeArrayView(PSConstantBuffers), 0);
        CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;

            FRHIBuffer* VertexBuffers[] =
            {
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Normals),
                StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
            };
            
            CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 3), 0);
            CommandList.SetIndexBuffer(StaticMesh->GetIndexBuffer(), StaticMesh->GetIndexFormat());

            constexpr uint32 NumConstants = sizeof(FTransformBufferHLSL) / sizeof(uint32);
            CommandList.SetShaderConstants(PipelineInstance->VertexShader.Get(), &StaticMesh->GetTransformShaderData(), NumConstants);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Deferred BasePass");
}

FTiledLightPass::FTiledLightPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TiledLightPassPSO(nullptr)
    , TiledLightShader(nullptr)
    , TiledLightPassPSO_TileDebug(nullptr)
    , TiledLightShader_TileDebug(nullptr)
{
}

FTiledLightPass::~FTiledLightPass()
{
    TiledLightPassPSO.Reset();
    TiledLightShader.Reset();
    TiledLightPassPSO_TileDebug.Reset();
    TiledLightShader_TileDebug.Reset();
}

bool FTiledLightPass::Initialize(FFrameResources& FrameResources)
{
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.GBufferSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!FrameResources.GBufferSampler)
    {
        return false;
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

    TArray<uint8> ShaderCode;

    // BRDF LUT Generation
    constexpr uint32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;
    if (!FRHI::Get()->QueryUAVFormatSupport(LUTFormat))
    {
        LOG_ERROR("[FSceneRenderer]: R16G16_Float is not supported for UAVs");
        return false;
    }

    FRHITextureDesc LUTDesc = FRHITextureDesc::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccessTexture);
    LUTDesc.bEnableResourceStateTracking = true;
    
    FRHITextureRef StagingTexture = FRHI::Get()->CreateTexture(LUTDesc, EResourceAccess::Common);
    if (!StagingTexture)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        StagingTexture->SetDebugName("Staging IntegrationLUT");
    }

    LUTDesc.UsageFlags = ETextureUsageFlags::ShaderResourceTexture;

    FrameResources.IntegrationLUT = FRHI::Get()->CreateTexture(LUTDesc, EResourceAccess::Common);
    if (!FrameResources.IntegrationLUT)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetDebugName("IntegrationLUT");
    }

    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!FrameResources.IntegrationLUTSampler)
    {
        DEBUG_BREAK();
        return false;
    }

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/BRDFIntegationGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputeShaderRef BRDFShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!BRDFShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = BRDFShader.Get();

    FRHIComputePipelineStateRef BRDFPipelineState = FRHI::Get()->CreateComputePipelineState(PSODesc);
    if (!BRDFPipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BRDFPipelineState->SetDebugName("BRDFIntegationGen PipelineState");
    }

    FRHICommandList CommandList;
    CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::UnorderedAccess));

    CommandList.SetComputePipelineState(BRDFPipelineState.Get());

    FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(BRDFShader.Get(), StagingUAV, 0);

    constexpr uint32 ThreadCount = 16;
    constexpr uint32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    constexpr uint32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);
    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CommandList.UnorderedAccessTextureBarrier(StagingTexture.Get());

    CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

    CommandList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    // Tiled lightning
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    TiledLightShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!TiledLightShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc DeferredLightPassPSODesc;
    DeferredLightPassPSODesc.Shader = TiledLightShader.Get();

    TiledLightPassPSO = FRHI::Get()->CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    // Tiled lightning Tile debugging
    TArray<FShaderDefine> Defines =
    {
        { "DRAW_TILE_DEBUG", "(1)" }
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    TiledLightShader_TileDebug = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!TiledLightShader_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassPSODesc.Shader = TiledLightShader_TileDebug.Get();

    TiledLightPassPSO_TileDebug = FRHI::Get()->CreateComputePipelineState(DeferredLightPassPSODesc);
    if (!TiledLightPassPSO_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_TileDebug->SetDebugName("DeferredLightPass PipelineState Tile-Debug");
    }

    return true;
}

bool FTiledLightPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc FinalTargetDesc = FRHITextureDesc::CreateTexture2D(GlobalTextureFormats::FinalTargetFormat, Width, Height, 1, 1, Usage);
    FinalTargetDesc.bEnableResourceStateTracking = true;

    FrameResources.FinalTarget = FRHI::Get()->CreateTexture(FinalTargetDesc, EResourceAccess::PixelShaderResource);
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetDebugName("Final Target");
    }
    else
    {
        return false;
    }

    return true;
}

void FTiledLightPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    GPU_TRACE_SCOPE(CommandList, "Light Pass");

    FRHIComputeShader* LightPassShader;
    if (CVarDrawTileDebug.GetValue())
    {
        LightPassShader = TiledLightShader_TileDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_TileDebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(LightPassShader, nullptr, 4); // DXR-Reflection

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 5);

    if (Scene)
    {
        // Global SkyLight as a fallback
        if (FSceneSkyLight* SkyLight = Scene->SkyLight)
        {
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->DiffuseCubeMap->GetShaderResourceView(), 6);
            CommandList.SetShaderResourceView(LightPassShader, SkyLight->SpecularCubeMap->GetShaderResourceView(), 7);
        }

        // Local Light-Probe
        if (!Scene->LightProbes.IsEmpty())
        {
            // TODO: Support more than the first probe
            if (FSceneLightProbe* LightProbe = Scene->LightProbes.FirstElement())
            {
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->DiffuseCubeMap->GetShaderResourceView(), 8);
                CommandList.SetShaderResourceView(LightPassShader, LightProbe->SpecularCubeMap->GetShaderResourceView(), 9);
            }
        }
    }

    CommandList.SetShaderResourceView(LightPassShader, FrameResources.DirectionalShadowMask->GetShaderResourceView(), 10);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 11);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 12);

    CommandList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.LightProbeBuffer.Get(), 6);

    CommandList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CommandList.SetSamplerState(LightPassShader, FrameResources.LightProbeSampler.Get(), 1);
    CommandList.SetSamplerState(LightPassShader, FrameResources.GBufferSampler.Get(), 2);
    CommandList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(LightPassShader, FinalTargetUAV, 0);

    struct FLightPassSettingsHLSL
    {
        // 0-16
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 NumLightProbes;

        // 16-32
        int32 ScreenWidth;
        int32 ScreenHeight;
        int32 bEnablePointLightShadows;
        int32 Padding0;
    } LightPassSettings;

    const int32 RenderWidth  = FrameResources.CurrentRenderWidth;
    const int32 RenderHeight = FrameResources.CurrentRenderHeight;

    LightPassSettings.NumSkyLightMips             = 0;
    LightPassSettings.NumShadowCastingPointLights = FrameResources.ShadowCastingPointLightsData.Size();
    LightPassSettings.NumPointLights              = FrameResources.PointLightsData.Size();
    LightPassSettings.NumLightProbes              = FrameResources.LightProbeInfos.Size();
    LightPassSettings.ScreenWidth                 = static_cast<int32>(RenderWidth);
    LightPassSettings.ScreenHeight                = static_cast<int32>(RenderHeight);
    LightPassSettings.Padding0                    = 0;

    if (Scene)
    {
        if (FSceneSkyLight* SkyLight = Scene->SkyLight)
        {
            LightPassSettings.NumSkyLightMips = SkyLight->SpecularCubeMap->GetNumMipLevels();
        }
    }

    // Enable point-light shadows based on CVar
    if (IConsoleVariable* CVarEnablePointLightShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.PointLightShadows"))
    {
        LightPassSettings.bEnablePointLightShadows = CVarEnablePointLightShadows->GetInt();
    }
    else
    {
        LightPassSettings.bEnablePointLightShadows = 1;
    }

    // ... also check the shadows CVar if the shadows should be enabled or not
    if (IConsoleVariable* CVarEnableShadows = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.Shadows"))
    {
        const int32 ShadowSetting = CVarEnableShadows->GetInt();
        LightPassSettings.bEnablePointLightShadows = LightPassSettings.bEnablePointLightShadows & ShadowSetting;
    }

    constexpr uint32 NumConstants = sizeof(FLightPassSettingsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(LightPassShader, &LightPassSettings, NumConstants);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = Math::DivideByMultiple<uint32>(LightPassSettings.ScreenHeight, NumThreads);
    CommandList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End LightPass");
}

FDepthReducePass::FDepthReducePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , ReduceDepthInitalPSO(nullptr)
    , ReduceDepthInitalShader(nullptr)
    , ReduceDepthPSO(nullptr)
    , ReduceDepthShader(nullptr)
{
}

FDepthReducePass::~FDepthReducePass()
{
    ReduceDepthInitalPSO.Reset();
    ReduceDepthInitalShader.Reset();
    ReduceDepthPSO.Reset();
    ReduceDepthShader.Reset();
}

bool FDepthReducePass::Initialize(FFrameResources& FrameResources)
{
    TArray<uint8> ShaderCode;

    // Depth-Reduction
    FShaderCompileInfo CompileInfo("ReductionMainInital", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    ReduceDepthInitalShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!ReduceDepthInitalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = ReduceDepthInitalShader.Get();

    ReduceDepthInitalPSO = FRHI::Get()->CreateComputePipelineState(PSODesc);
    if (!ReduceDepthInitalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthInitalPSO->SetDebugName("Initial DepthReduction PipelineState");
    }

    // Depth-Reduction
    CompileInfo = FShaderCompileInfo("ReductionMain", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    ReduceDepthShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!ReduceDepthShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = ReduceDepthShader.Get();

    ReduceDepthPSO = FRHI::Get()->CreateComputePipelineState(PSODesc);
    if (!ReduceDepthPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthPSO->SetDebugName("DepthReduction PipelineState");
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

    return true;
}

bool FDepthReducePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    constexpr uint32 Alignment = 16;
    const uint32 ReducedWidth  = Math::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = Math::DivideByMultiple(Height, Alignment);

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32_Float, ReducedWidth, ReducedHeight, 1, 1, Usage);
    TextureDesc.bEnableResourceStateTracking = true;
    
    for (int32 Index = 0; Index < FrameResources.NumReducedDepthBuffers; Index++)
    {
        FrameResources.ReducedDepthBuffer[Index] = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
        if (FrameResources.ReducedDepthBuffer[Index])
        {
            FrameResources.ReducedDepthBuffer[Index]->SetDebugName("Reduced DepthStencil[" + TTypeToString<int32>::ToString(Index) + "]");
        }
        else
        {
            return false;
        }
    }

    FRHITextureDesc HistoryDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32_Float, 1, 1, 1, 1, Usage);
    HistoryDesc.bEnableResourceStateTracking = true;

    FrameResources.CSMMinMaxDepthHistory = FRHI::Get()->CreateTexture(HistoryDesc, EResourceAccess::UnorderedAccess);
    if (FrameResources.CSMMinMaxDepthHistory)
    {
        FrameResources.CSMMinMaxDepthHistory->SetDebugName("CSM MinMax Depth History");
        
        // Reset the history on resize
        FrameResources.bCSMMinMaxHistoryInitialized  = false;
        FrameResources.bCSMCascadeHistoryInitialized = false;
    }
    else
    {
        return false;
    }

    return true;
}

void FDepthReducePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Depth Reduction");

    TRACE_SCOPE("Depth Reduction");

    GPU_TRACE_SCOPE(CommandList, "Depth Reduction");

    struct FReductionConstants
    {
        FMatrix4 CamProjection;
        float    NearPlane;
        float    FarPlane;
    } ReductionConstants;

    FCamera* Camera = Scene->Camera;
    ReductionConstants.CamProjection = Camera->GetProjectionMatrix();
    ReductionConstants.NearPlane     = Camera->GetNearPlane();
    ReductionConstants.FarPlane      = Camera->GetFarPlane();

    const EResourceAccess DepthBefore = EResourceAccess::DepthWrite;

    // Perform the first reduction
    FRHITexture* DepthSource = FrameResources.GBuffer[GBufferIndex_Depth].Get();
    CommandList.TransitionTexture(DepthSource, FRHITextureTransition::Make(DepthBefore, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

    CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), DepthSource->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    constexpr uint32 NumConstants = sizeof(FReductionConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, NumConstants);

    uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
    uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(DepthSource, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, DepthBefore));

    // Perform the other reductions
    CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

    ThreadsX = Math::DivideByMultiple(ThreadsX, 16);
    ThreadsY = Math::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    ThreadsX = Math::DivideByMultiple(ThreadsX, 16);
    ThreadsY = Math::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
    
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Reduction");
}
