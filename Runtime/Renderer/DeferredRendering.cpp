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

    FGraphicsPipelineStateInstance* CachedPrePassPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedPrePassPSO)
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

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
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

        NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInitializer);
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
            FRHIVertexLayoutInitializerList VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexTexCoord), 1, 0, 1, EVertexInputClass::Vertex, 0 }
            };

            NewPipelineInstance.InputLayout = FRHI::Get()->CreateVertexLayout(VertexElementList);
            if (!NewPipelineInstance.InputLayout)
            {
                DEBUG_BREAK();
                return;
            }
        }
        else
        {
            FRHIVertexLayoutInitializerList VertexElementList =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexPosition), 0, 0, 0, EVertexInputClass::Vertex, 0 }
            };

            NewPipelineInstance.InputLayout = FRHI::Get()->CreateVertexLayout(VertexElementList);
            if (!NewPipelineInstance.InputLayout)
            {
                DEBUG_BREAK();
                return;
            }
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.VertexInputLayout                  = NewPipelineInstance.InputLayout.Get();
        PSOInitializer.BlendState                         = NewPipelineInstance.BlendState.Get();
        PSOInitializer.DepthStencilState                  = NewPipelineInstance.DepthStencilState.Get();
        PSOInitializer.RasterizerState                    = NewPipelineInstance.RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader           = NewPipelineInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = NewPipelineInstance.PixelShader.Get();
        PSOInitializer.PipelineFormats.DepthStencilFormat = FGlobalTextureFormats::DepthBufferFormat;

        NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("PrePass PipelineState %d", MaterialFlags);
            NewPipelineInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineInstance));
    }
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

    const ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;
    const FClearValue DepthClearValue(FGlobalTextureFormats::DepthBufferFormat, 1.0f, 0);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    FrameResources.GBuffer[GBufferIndex_Depth] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::PixelShaderResource);
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

void FDepthPrePass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Depth Pre-Pass");

    TRACE_SCOPE("Depth Pre-Pass");

    GPU_TRACE_SCOPE(CommandList, "Depth Pre-Pass");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get());

    CommandList.BeginRenderPass(RenderPass);

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

        if (Material->HasAlphaMask())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
            CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

            if (Material->IsPackedMaterial())
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            }
            else
            {
                CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->AlphaMask->GetShaderResourceView(), 0);
            }
        }
        else if (Material->HasHeightMap())
        {
            CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
            CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
            CommandList.SetShaderResourceView(PipelineInstance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
        }

        for (const FMeshBatch::FMeshReference& MeshReference : Batch.MeshReferences)
        {
            FSceneStaticMesh* StaticMesh = MeshReference.StaticMesh;
            if (Material->HasAlphaMask() || Material->IsDoubleSided())
            {
                FRHIBuffer* VertexBuffers[] =
                {
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                };
                
                CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 2), 0);
            }
            else if (Material->HasHeightMap())
            {
                FRHIBuffer* VertexBuffers[] =
                {
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Positions),
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::Normals),
                    StaticMesh->GetMesh()->GetVertexBuffer(EVertexStream::TexCoords),
                };
                
                CommandList.SetVertexBuffers(MakeArrayView(VertexBuffers, 3), 0);
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
            CommandList.Set32BitShaderConstants(PipelineInstance->VertexShader.Get(), &StaticMesh->GetTransformShaderData(), NumConstants);

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Pre-Pass");
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

        FRHIDepthStencilStateInitializer DepthStencilInitializer;
        DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable      = true;
        DepthStencilInitializer.bDepthWriteEnable = false;

        NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilInitializer);
        if (!NewPipelineInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
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

        NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = GBuffer_NumRenderTargets;

        NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInitializer);
        if (!NewPipelineInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        // NOTE: Always use the default InputLayout
        NewPipelineInstance.InputLayout = FrameResources.MeshInputLayout;

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.VertexInputLayout                      = NewPipelineInstance.InputLayout.Get();
        PSOInitializer.BlendState                             = NewPipelineInstance.BlendState.Get();
        PSOInitializer.DepthStencilState                      = NewPipelineInstance.DepthStencilState.Get();
        PSOInitializer.RasterizerState                        = NewPipelineInstance.RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = NewPipelineInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader                = NewPipelineInstance.PixelShader.Get();
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FGlobalTextureFormats::AlbedoFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[1] = FGlobalTextureFormats::NormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[2] = FGlobalTextureFormats::MaterialFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[3] = FGlobalTextureFormats::VelocityFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = GBuffer_NumRenderTargets;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

        NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
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

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::AlbedoFormat, Width, Height, 1, 1, Usage);

    // Albedo
    FrameResources.GBuffer[GBufferIndex_Albedo] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Albedo])
    {
        FrameResources.GBuffer[GBufferIndex_Albedo]->SetDebugName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureInfo.Format = FGlobalTextureFormats::NormalFormat;

    FrameResources.GBuffer[GBufferIndex_Normal] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Normal])
    {
        FrameResources.GBuffer[GBufferIndex_Normal]->SetDebugName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureInfo.Format = FGlobalTextureFormats::MaterialFormat;

    FrameResources.GBuffer[GBufferIndex_Material] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Material])
    {
        FrameResources.GBuffer[GBufferIndex_Material]->SetDebugName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // Velocity
    TextureInfo.Format = FGlobalTextureFormats::VelocityFormat;

    FrameResources.GBuffer[GBufferIndex_Velocity] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
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

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets                     = GBuffer_NumRenderTargets;
    RenderPass.RenderTargets[GBufferIndex_Albedo]   = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Albedo].Get(), LoadAction);
    RenderPass.RenderTargets[GBufferIndex_Normal]   = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Normal].Get(), EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[GBufferIndex_Material] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Material].Get(), LoadAction);
    RenderPass.RenderTargets[GBufferIndex_Velocity] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Velocity].Get(), LoadAction);
    RenderPass.DepthStencilView                     = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

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
            CommandList.Set32BitShaderConstants(PipelineInstance->VertexShader.Get(), &StaticMesh->GetTransformShaderData(), NumConstants);

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
    , TiledLightPassPSO_CascadeDebug(nullptr)
    , TiledLightShader_CascadeDebug(nullptr)
{
}

FTiledLightPass::~FTiledLightPass()
{
    TiledLightPassPSO.Reset();
    TiledLightShader.Reset();
    TiledLightPassPSO_TileDebug.Reset();
    TiledLightShader_TileDebug.Reset();
    TiledLightPassPSO_CascadeDebug.Reset();
    TiledLightShader_CascadeDebug.Reset();
}

bool FTiledLightPass::Initialize(FFrameResources& FrameResources)
{
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Clamp;
    SamplerInfo.AddressV = ESamplerMode::Clamp;
    SamplerInfo.AddressW = ESamplerMode::Clamp;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.GBufferSampler = FRHI::Get()->CreateSamplerState(SamplerInfo);
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

    FRHITextureInfo LUTInfo = FRHITextureInfo::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccess);
    FRHITextureRef StagingTexture = FRHI::Get()->CreateTexture(LUTInfo, EResourceAccess::Common);

    if (!StagingTexture)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        StagingTexture->SetDebugName("Staging IntegrationLUT");
    }

    LUTInfo.UsageFlags = ETextureUsageFlags::ShaderResource;

    FrameResources.IntegrationLUT = FRHI::Get()->CreateTexture(LUTInfo, EResourceAccess::Common);
    if (!FrameResources.IntegrationLUT)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetDebugName("IntegrationLUT");
    }

    SamplerInfo.AddressU = ESamplerMode::Clamp;
    SamplerInfo.AddressV = ESamplerMode::Clamp;
    SamplerInfo.AddressW = ESamplerMode::Clamp;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = FRHI::Get()->CreateSamplerState(SamplerInfo);
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

    FRHIComputePipelineStateInitializer PSOInitializer(BRDFShader.Get());
    FRHIComputePipelineStateRef BRDFPipelineState = FRHI::Get()->CreateComputePipelineState(PSOInitializer);
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
    constexpr uint32 DispatchWidth  = FMath::DivideByMultiple(LUTSize, ThreadCount);
    constexpr uint32 DispatchHeight = FMath::DivideByMultiple(LUTSize, ThreadCount);
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

    FRHIComputePipelineStateInitializer DeferredLightPassInitializer(TiledLightShader.Get());
    TiledLightPassPSO = FRHI::Get()->CreateComputePipelineState(DeferredLightPassInitializer);
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

    DeferredLightPassInitializer = FRHIComputePipelineStateInitializer(TiledLightShader_TileDebug.Get());
    TiledLightPassPSO_TileDebug = FRHI::Get()->CreateComputePipelineState(DeferredLightPassInitializer);
    if (!TiledLightPassPSO_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_TileDebug->SetDebugName("DeferredLightPass PipelineState Tile-Debug");
    }

    // Tiled lightning Cascade debugging
    Defines =
    {
        { "DRAW_CASCADE_DEBUG", "(1)" }
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    TiledLightShader_CascadeDebug = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!TiledLightShader_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassInitializer = FRHIComputePipelineStateInitializer(TiledLightShader_CascadeDebug.Get());
    TiledLightPassPSO_CascadeDebug = FRHI::Get()->CreateComputePipelineState(DeferredLightPassInitializer);
    if (!TiledLightPassPSO_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        TiledLightPassPSO_CascadeDebug->SetDebugName("DeferredLightPass PipelineState Cascade-Debug");
    }

    return true;
}

bool FTiledLightPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo FinalTargetInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::FinalTargetFormat, Width, Height, 1, 1, Usage);
    FrameResources.FinalTarget = FRHI::Get()->CreateTexture(FinalTargetInfo, EResourceAccess::PixelShaderResource);
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

    bool bDrawCascades = false;
    if (IConsoleVariable* CVarDrawCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawCascades"))
    {
        bDrawCascades = CVarDrawCascades->GetBool();
    }

    FRHIComputeShader* LightPassShader;
    if (CVarDrawTileDebug.GetValue())
    {
        LightPassShader = TiledLightShader_TileDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_TileDebug.Get());
    }
    else if (bDrawCascades)
    {
        LightPassShader = TiledLightShader_CascadeDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_CascadeDebug.Get());
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

    if (bDrawCascades)
    {
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 13);
    }

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
    CommandList.Set32BitShaderConstants(LightPassShader, &LightPassSettings, NumConstants);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = FMath::DivideByMultiple<uint32>(LightPassSettings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = FMath::DivideByMultiple<uint32>(LightPassSettings.ScreenHeight, NumThreads);
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

    FRHIComputePipelineStateInitializer PipelineStateInfo(ReduceDepthInitalShader.Get());
    ReduceDepthInitalPSO = FRHI::Get()->CreateComputePipelineState(PipelineStateInfo);

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

    FRHIComputePipelineStateInitializer PSOInitializer(ReduceDepthShader.Get());
    ReduceDepthPSO = FRHI::Get()->CreateComputePipelineState(PSOInitializer);

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
    const uint32 ReducedWidth  = FMath::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = FMath::DivideByMultiple(Height, Alignment);

    const ETextureUsageFlags Usage = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(EFormat::R32G32_Float, ReducedWidth, ReducedHeight, 1, 1, Usage);
    for (int32 Index = 0; Index < FrameResources.NumReducedDepthBuffers; Index++)
    {
        FrameResources.ReducedDepthBuffer[Index] = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
        if (FrameResources.ReducedDepthBuffer[Index])
        {
            FrameResources.ReducedDepthBuffer[Index]->SetDebugName("Reduced DepthStencil[" + TTypeToString<int32>::ToString(Index) + "]");
        }
        else
        {
            return false;
        }
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

    // Perform the first reduction
    CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));

    CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    constexpr uint32 NumConstants = sizeof(FReductionConstants) / sizeof(uint32);
    CommandList.Set32BitShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, NumConstants);

    uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
    uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
    CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite));

    // Perform the other reductions
    CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

    ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
    ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess));
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
    ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource));
    
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Reduction");
}
