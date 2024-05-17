#include "DeferredRendering.h"
#include "Scene.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "Renderer/Debug/GPUProfiler.h"

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
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FPipelineStateInstance* CachedPrePassPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedPrePassPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");

        if (MaterialFlags & MaterialFlag_EnableAlpha)
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");

        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        constexpr EMaterialFlags PSFlags = MaterialFlag_EnableHeight | MaterialFlag_PackedDiffuseAlpha | MaterialFlag_EnableAlpha;
        const bool bWantPixelShader = (MaterialFlags & PSFlags) != MaterialFlag_None;
        if (bWantPixelShader)
        {
            CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }

            NewPipelineInstance.PixelShader = RHICreatePixelShader(ShaderCode);
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

        NewPipelineInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        if (MaterialFlags & MaterialFlag_EnableHeight)
        {
            NewPipelineInstance.InputLayout = FrameResources.MeshInputLayout;
        }
        else if (MaterialFlags & (MaterialFlag_PackedDiffuseAlpha | MaterialFlag_EnableAlpha))
        {
            FRHIVertexInputLayoutInitializer InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexMasked), 0, 0,  EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexMasked), 0, 12, EVertexInputClass::Vertex, 0 }
            };

            NewPipelineInstance.InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
            if (!NewPipelineInstance.InputLayout)
            {
                DEBUG_BREAK();
                return;
            }
        }
        else
        {
            FRHIVertexInputLayoutInitializer InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
            };

            NewPipelineInstance.InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
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
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        NewPipelineInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
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
    return CreateResources(FrameResources, FrameResources.CurrentWidth, FrameResources.CurrentHeight);
}

bool FDepthPrePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;
    const FClearValue DepthClearValue(FrameResources.DepthBufferFormat, 1.0f, 0);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FrameResources.DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    FrameResources.GBuffer[GBufferIndex_Depth] = RHICreateTexture(TextureInfo, EResourceAccess::PixelShaderResource);
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

    const float RenderWidth  = float(FrameResources.CurrentWidth);
    const float RenderHeight = float(FrameResources.CurrentHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    struct FTransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;
    TransformPerObject.TransformInv = FMatrix4::Identity();

    for (const FMeshBatch& Batch : Scene->VisibleMeshBatches)
    {
        FMaterial* Material = Batch.Material;
        CHECK(Material != nullptr);

        if (!Material->ShouldRenderInPrePass())
        {
            continue;
        }

        FPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
        if (!Instance)
        {
            DEBUG_BREAK();
        }

        FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(Instance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (Material->HasAlphaMask())
        {
            CommandList.SetConstantBuffer(Instance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
            CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

            if (Material->IsPackedMaterial())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            }
            else
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlphaMask->GetShaderResourceView(), 0);
            }
        }
        else if (Material->HasHeightMap())
        {
            CommandList.SetConstantBuffer(Instance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
            CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
        }

        for (FProxySceneComponent* Component : Batch.Primitives)
        {
            if (Component->IsOccluded())
            {
                continue;
            }

            if (Material->HasAlphaMask() || Material->IsDoubleSided())
            {
                CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
            }
            else if (Material->HasHeightMap())
            {
                CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->VertexBuffer, 1), 0);
            }
            else
            {
                CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
            }

            CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

            TransformPerObject.Transform = Component->CurrentActor->GetTransform().GetMatrix();
            CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &TransformPerObject, 32);

            CommandList.DrawIndexedInstanced(Component->NumIndices, 1, 0, 0, 0);
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
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FPipelineStateInstance* CachedBasePassPSO = MaterialPSOs.Find(MaterialFlags);
    if (!CachedBasePassPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");

        if (MaterialFlags & MaterialFlag_EnableNormalMapping)
            ShaderDefines.Emplace("ENABLE_NORMAL_MAPPING", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_NORMAL_MAPPING", "(0)");

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");

        if (MaterialFlags & MaterialFlag_EnableAlpha)
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");

        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FPipelineStateInstance NewPipelineInstance;
        NewPipelineInstance.VertexShader = RHICreateVertexShader(ShaderCode);
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

        NewPipelineInstance.PixelShader = RHICreatePixelShader(ShaderCode);
        if (!NewPipelineInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateInitializer DepthStencilInitializer;
        DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable      = true;
        DepthStencilInitializer.bDepthWriteEnable = false;

        NewPipelineInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
        if (!NewPipelineInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!NewPipelineInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = 5;

        NewPipelineInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
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
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.AlbedoFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[2] = FrameResources.MaterialFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[4] = FrameResources.VelocityFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 5;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

        NewPipelineInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
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
    return CreateResources(FrameResources, FrameResources.CurrentWidth, FrameResources.CurrentHeight);
}

bool FDeferredBasePass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FrameResources.AlbedoFormat, Width, Height, 1, 1, Usage);

    // Albedo
    FrameResources.GBuffer[GBufferIndex_Albedo] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Albedo])
    {
        FrameResources.GBuffer[GBufferIndex_Albedo]->SetDebugName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureInfo.Format = FrameResources.NormalFormat;

    FrameResources.GBuffer[GBufferIndex_Normal] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Normal])
    {
        FrameResources.GBuffer[GBufferIndex_Normal]->SetDebugName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureInfo.Format = FrameResources.MaterialFormat;

    FrameResources.GBuffer[GBufferIndex_Material] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Material])
    {
        FrameResources.GBuffer[GBufferIndex_Material]->SetDebugName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // View Normal
    TextureInfo.Format = FrameResources.ViewNormalFormat;

    FrameResources.GBuffer[GBufferIndex_ViewNormal] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_ViewNormal])
    {
        FrameResources.GBuffer[GBufferIndex_ViewNormal]->SetDebugName("GBuffer ViewNormal");
    }
    else
    {
        return false;
    }

    // Velocity
    TextureInfo.Format = FrameResources.VelocityFormat;

    FrameResources.GBuffer[GBufferIndex_Velocity] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
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

    const float RenderWidth  = float(FrameResources.CurrentWidth);
    const float RenderHeight = float(FrameResources.CurrentHeight);

    const EAttachmentLoadAction LoadAction = CVarBasePassClearAllTargets.GetValue() ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load;

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets = 5;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Albedo].Get(), LoadAction);
    RenderPass.RenderTargets[1] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Normal].Get(), EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[2] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Material].Get(), LoadAction);
    RenderPass.RenderTargets[3] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_ViewNormal].Get(), LoadAction);
    RenderPass.RenderTargets[4] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Velocity].Get(), LoadAction);
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    struct FTransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;

    for (const FMeshBatch& Batch : Scene->VisibleMeshBatches)
    {
        FMaterial* Material = Batch.Material;
        if (Material->ShouldRenderInForwardPass())
        {
            continue;
        }

        FPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
        if (!Instance)
        {
            DEBUG_BREAK();
        }

        FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
        CHECK(PipelineState  != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(Instance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        if (Material->IsPackedMaterial())
        {
            // Setup resources after the PipelineState since binding a pipeline invalidates all resources
            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            if (Material->HasNormalMap())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->NormalMap->GetShaderResourceView(), 1);
            }

            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->SpecularMap->GetShaderResourceView(), 2);

            if (Material->HasHeightMap())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 3);
            }
        }
        else
        {
            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlbedoMap->GetShaderResourceView(), 0);
            if (Material->HasNormalMap())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->NormalMap->GetShaderResourceView(), 1);
            }

            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->RoughnessMap->GetShaderResourceView(), 2);
            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->MetallicMap->GetShaderResourceView(), 3);
            CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AOMap->GetShaderResourceView(), 4);

            if (Material->HasAlphaMask())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->AlphaMask->GetShaderResourceView(), 5);
            }
            if (Material->HasHeightMap())
            {
                CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 6);
            }
        }

        FRHIBuffer* PSConstantBuffers[] =
        {
            FrameResources.CameraBuffer.Get(),
            Material->GetMaterialBuffer(),
        };

        CommandList.SetConstantBuffers(Instance->PixelShader.Get(), MakeArrayView(PSConstantBuffers), 0);
        CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

        for (const FProxySceneComponent* Component : Batch.Primitives)
        {
            if (Component->IsOccluded())
            {
                continue;
            }

            CommandList.SetVertexBuffers(MakeArrayView(&Component->VertexBuffer, 1), 0);
            CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

            TransformPerObject.Transform    = Component->CurrentActor->GetTransform().GetMatrix();
            TransformPerObject.TransformInv = Component->CurrentActor->GetTransform().GetMatrixInverse();
            CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &TransformPerObject, 32);

            CommandList.DrawIndexedInstanced(Component->NumIndices, 1, 0, 0, 0);
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

    FrameResources.GBufferSampler = RHICreateSamplerState(SamplerInfo);
    if (!FrameResources.GBufferSampler)
    {
        return false;
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentWidth, FrameResources.CurrentHeight))
    {
        return false;
    }

    TArray<uint8> ShaderCode;

    // BRDF LUT Generation
    constexpr uint32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;
    if (!RHIQueryUAVFormatSupport(LUTFormat))
    {
        LOG_ERROR("[FSceneRenderer]: R16G16_Float is not supported for UAVs");
        return false;
    }

    FRHITextureInfo LUTInfo = FRHITextureInfo::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccess);
    FRHITextureRef StagingTexture = RHICreateTexture(LUTInfo, EResourceAccess::Common);
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

    FrameResources.IntegrationLUT = RHICreateTexture(LUTInfo, EResourceAccess::Common);
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

    FrameResources.IntegrationLUTSampler = RHICreateSamplerState(SamplerInfo);
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

    FRHIComputeShaderRef BRDFShader = RHICreateComputeShader(ShaderCode);
    if (!BRDFShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer PSOInitializer(BRDFShader.Get());
    FRHIComputePipelineStateRef BRDFPipelineState = RHICreateComputePipelineState(PSOInitializer);
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
    CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(BRDFPipelineState.Get());

    FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(BRDFShader.Get(), StagingUAV, 0);

    constexpr uint32 ThreadCount = 16;
    constexpr uint32 DispatchWidth  = FMath::DivideByMultiple(LUTSize, ThreadCount);
    constexpr uint32 DispatchHeight = FMath::DivideByMultiple(LUTSize, ThreadCount);
    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CommandList.UnorderedAccessTextureBarrier(StagingTexture.Get());

    CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
    CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

    CommandList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);
    GRHICommandExecutor.ExecuteCommandList(CommandList);

    // Tiled lightning
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    TiledLightShader = RHICreateComputeShader(ShaderCode);
    if (!TiledLightShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer DeferredLightPassInitializer(TiledLightShader.Get());
    TiledLightPassPSO = RHICreateComputePipelineState(DeferredLightPassInitializer);
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

    TiledLightShader_TileDebug = RHICreateComputeShader(ShaderCode);
    if (!TiledLightShader_TileDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassInitializer = FRHIComputePipelineStateInitializer(TiledLightShader_TileDebug.Get());
    TiledLightPassPSO_TileDebug = RHICreateComputePipelineState(DeferredLightPassInitializer);
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

    TiledLightShader_CascadeDebug = RHICreateComputeShader(ShaderCode);
    if (!TiledLightShader_CascadeDebug)
    {
        DEBUG_BREAK();
        return false;
    }

    DeferredLightPassInitializer = FRHIComputePipelineStateInitializer(TiledLightShader_CascadeDebug.Get());
    TiledLightPassPSO_CascadeDebug = RHICreateComputePipelineState(DeferredLightPassInitializer);
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
    FRHITextureInfo FinalTargetInfo = FRHITextureInfo::CreateTexture2D(FrameResources.FinalTargetFormat, Width, Height, 1, 1, Usage);
    FrameResources.FinalTarget = RHICreateTexture(FinalTargetInfo, EResourceAccess::PixelShaderResource);
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

void FTiledLightPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources)
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

    const FProxyLightProbe& Skylight = FrameResources.Skylight;
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(LightPassShader, nullptr, 4); // DXR-Reflection
    CommandList.SetShaderResourceView(LightPassShader, Skylight.IrradianceMap->GetShaderResourceView(), 5);
    CommandList.SetShaderResourceView(LightPassShader, Skylight.SpecularIrradianceMap->GetShaderResourceView(), 6);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 7);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.DirectionalShadowMask->GetShaderResourceView(), 8);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.PointLightShadowMaps->GetShaderResourceView(), 9);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 10);

    if (bDrawCascades)
    {
        CommandList.SetShaderResourceView(LightPassShader, FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 11);
    }

    CommandList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(LightPassShader, FrameResources.DirectionalLightDataBuffer.Get(), 5);

    CommandList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CommandList.SetSamplerState(LightPassShader, FrameResources.IrradianceSampler.Get(), 1);
    CommandList.SetSamplerState(LightPassShader, FrameResources.GBufferSampler.Get(), 2);
    CommandList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(LightPassShader, FinalTargetUAV, 0);

    struct FLightPassSettings
    {
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 ScreenWidth;
        int32 ScreenHeight;
    } Settings;

    const int32 RenderWidth  = FrameResources.CurrentWidth;
    const int32 RenderHeight = FrameResources.CurrentHeight;

    Settings.NumShadowCastingPointLights = FrameResources.ShadowCastingPointLightsData.Size();
    Settings.NumPointLights              = FrameResources.PointLightsData.Size();
    Settings.NumSkyLightMips             = Skylight.SpecularIrradianceMap->GetNumMipLevels();
    Settings.ScreenWidth                 = static_cast<int32>(RenderWidth);
    Settings.ScreenHeight                = static_cast<int32>(RenderHeight);
    CommandList.Set32BitShaderConstants(LightPassShader, &Settings, 5);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = FMath::DivideByMultiple<uint32>(Settings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = FMath::DivideByMultiple<uint32>(Settings.ScreenHeight, NumThreads);
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

    ReduceDepthInitalShader = RHICreateComputeShader(ShaderCode);
    if (!ReduceDepthInitalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer PipelineStateInfo(ReduceDepthInitalShader.Get());
    ReduceDepthInitalPSO = RHICreateComputePipelineState(PipelineStateInfo);
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

    ReduceDepthShader = RHICreateComputeShader(ShaderCode);
    if (!ReduceDepthShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer PSOInitializer(ReduceDepthShader.Get());
    ReduceDepthPSO = RHICreateComputePipelineState(PSOInitializer);
    if (!ReduceDepthPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthPSO->SetDebugName("DepthReduction PipelineState");
    }

    if (!CreateResources(FrameResources, FrameResources.CurrentWidth, FrameResources.CurrentHeight))
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
        FrameResources.ReducedDepthBuffer[Index] = RHICreateTexture(TextureInfo, EResourceAccess::NonPixelShaderResource);
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
    CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    CommandList.Set32BitShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, FMath::BytesToNum32BitConstants(sizeof(ReductionConstants)));

    uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
    uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

    // Perform the other reductions
    CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

    ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
    ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

    ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
    ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Reduction");
}

FOcclusionPass::FOcclusionPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , VertexShader(nullptr)
    , PipelineState(nullptr)
    , CubeVertexBuffer(nullptr)
    , CubeIndexBuffer(nullptr)
    , CubeIndexCount(0)
{
}

FOcclusionPass::~FOcclusionPass()
{
    VertexShader.Reset();
    PipelineState.Reset();
    CubeVertexBuffer.Reset();
    CubeIndexBuffer.Reset();
}

bool FOcclusionPass::Initialize(FFrameResources& FrameResources)
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/OcclusionPass.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    VertexShader = RHICreateVertexShader(ShaderCode);
    if (!VertexShader)
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
    DepthStencilInitializer.bDepthEnable      = true;
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
    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.BlendState                         = BlendState.Get();
    PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
    PSOInitializer.VertexInputLayout                  = InputLayoutState.Get();
    PSOInitializer.RasterizerState                    = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader           = VertexShader.Get();
    PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
    PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

    PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetDebugName("Occlusion Culling PipelineState");
    }

    TStaticArray<FVector3, 8> Vertices =
    {
        FVector3(-0.5f, -0.5f, -0.5f), // 0
        FVector3( 0.5f, -0.5f, -0.5f), // 1
        FVector3( 0.5f,  0.5f, -0.5f), // 2
        FVector3(-0.5f,  0.5f, -0.5f), // 3
        FVector3(-0.5f, -0.5f,  0.5f), // 4
        FVector3( 0.5f, -0.5f,  0.5f), // 5
        FVector3( 0.5f,  0.5f,  0.5f), // 6
        FVector3(-0.5f,  0.5f,  0.5f)  // 7
    };

    FRHIBufferInfo VBInfo(Vertices.SizeInBytes(), sizeof(FVector3), EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Default);
    CubeVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::Common, Vertices.Data());
    if (!CubeVertexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CubeVertexBuffer->SetDebugName("Occlusion Cube VertexBuffer");
    }

    // Create IndexBuffer
    TStaticArray<uint16, 36> Indices =
    {
        // Front face
        4, 5, 6, 4, 6, 7,
        // Back face
        0, 3, 2, 0, 2, 1,
        // Left face
        0, 4, 7, 0, 7, 3,
        // Right face
        1, 2, 6, 1, 6, 5,
        // Top face
        3, 7, 6, 3, 6, 2,
        // Bottom face
        0, 1, 5, 0, 5, 4
    };

    FRHIBufferInfo IBInfo(Indices.SizeInBytes(), sizeof(uint16), EBufferUsageFlags::IndexBuffer | EBufferUsageFlags::Default);
    CubeIndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::Common, Indices.Data());
    if (!CubeIndexBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CubeIndexBuffer->SetDebugName("Occlusion Cube IndexBuffer");

        CubeIndexCount  = Indices.Size();
        CubeIndexFormat = EIndexFormat::uint16;
    }

    return true;
}

void FOcclusionPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Occlusion Pass");

    TRACE_SCOPE("Occlusion Pass");

    GPU_TRACE_SCOPE(CommandList, "Occlusion Pass");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load, EAttachmentStoreAction::DontCare);

    CommandList.BeginRenderPass(RenderPass);

    const float RenderWidth  = float(FrameResources.CurrentWidth);
    const float RenderHeight = float(FrameResources.CurrentHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    struct FTransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;
    TransformPerObject.TransformInv = FMatrix4::Identity();

    CommandList.SetGraphicsPipelineState(PipelineState.Get());
    CommandList.SetVertexBuffers(MakeArrayView(&CubeVertexBuffer, 1), 0);
    CommandList.SetIndexBuffer(CubeIndexBuffer.Get(), CubeIndexFormat);
    CommandList.SetConstantBuffer(VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    for (FProxySceneComponent* Component : Scene->VisiblePrimitives)
    {
        // Create Query
        Component->CurrentOcclusionQueryIndex = (Component->CurrentOcclusionQueryIndex + 1) % NUM_OCCLUSION_QUERIES;
        Component->CurrentOcclusionQuery = Component->OcclusionQueries[Component->CurrentOcclusionQueryIndex];

        if (!Component->CurrentOcclusionQuery)
        {
            FRHIQuery* NewOcclusionQuery = RHICreateQuery(EQueryType::Occlusion);
            if (!NewOcclusionQuery)
            {
                continue;
            }

            Component->OcclusionQueries[Component->CurrentOcclusionQueryIndex] = NewOcclusionQuery;
            Component->CurrentOcclusionQuery = NewOcclusionQuery;
        }

        // Create Translation Matrix for BoundingBox
        FAABB& Box = Component->Mesh->BoundingBox;

        FVector3 Scale             = FVector3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
        FVector3 Position          = Box.GetCenter();
        FMatrix4 TranslationMatrix = FMatrix4::Translation(Position.x, Position.y, Position.z);
        FMatrix4 ScaleMatrix       = FMatrix4::Scale(Scale.x, Scale.y, Scale.z).Transpose();
        FMatrix4 TransformMatrix   = Component->CurrentActor->GetTransform().GetMatrix();

        TransformMatrix = TransformMatrix.Transpose();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;
        TransformMatrix = TransformMatrix.Transpose();

        TransformPerObject.Transform = TransformMatrix;
        CommandList.Set32BitShaderConstants(VertexShader.Get(), &TransformPerObject, 32);

        CommandList.BeginQuery(Component->CurrentOcclusionQuery);
        CommandList.DrawIndexedInstanced(CubeIndexCount, 1, 0, 0, 0);
        CommandList.EndQuery(Component->CurrentOcclusionQuery);
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Occlusion Pass");
}
