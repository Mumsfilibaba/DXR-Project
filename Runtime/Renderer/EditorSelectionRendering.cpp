#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Material.h"
#include "Renderer/EditorSelectionRendering.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "Core/Misc/ConsoleManager.h"

#if EDITOR_BUILD

static TAutoConsoleVariable<bool> CVarEditorSelectionUseUnjitteredCamera(
    "Renderer.Editor.Selection.UseUnjitteredCamera",
    "Use unjittered camera matrices for editor selection buffers (depth/ObjectID). Disable to better match TAA-jittered shading at the cost of more outline jitter.",
    true,
    EConsoleVariableFlags::Default);

FEditorNoJitterDepthPass::FEditorNoJitterDepthPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FEditorNoJitterDepthPass::~FEditorNoJitterDepthPass()
{
    MaterialPSOs.Clear();
}

void FEditorNoJitterDepthPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const int32 MaterialFlags = static_cast<int32>(Material->GetMaterialFlags());

    FGraphicsPipelineStateInstance* CachedPSO = MaterialPSOs.Find(MaterialFlags);
    if (CachedPSO)
    {
        return;
    }

    TArray<uint8>         ShaderCode;
    TArray<FShaderDefine> ShaderDefines;

    ShaderDefines.Emplace("USE_UNJITTERED_CAMERA", CVarEditorSelectionUseUnjitteredCamera.GetValue() ? "(1)" : "(0)");

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

    FRHIDepthStencilStateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc         = EComparisonFunc::Less;
    DepthStencilStateInfo.bDepthEnable      = true;
    DepthStencilStateInfo.bDepthWriteEnable = true;

    NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInfo);
    if (!NewPipelineInstance.DepthStencilState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIRasterizerStateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = Material->IsDoubleSided() ? ECullMode::None : ECullMode::Back;

    NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
    if (!NewPipelineInstance.RasterizerState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIBlendStateInfo BlendStateInfo;
    NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
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
        TArray<FRHIInputElementInfo> InputElements =
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
        TArray<FRHIInputElementInfo> InputElements =
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

    FRHIGraphicsPipelineStateInfo PSOInfo;
    PSOInfo.InputLayout                                = NewPipelineInstance.InputLayout.Get();
    PSOInfo.BlendState                                 = NewPipelineInstance.BlendState.Get();
    PSOInfo.DepthStencilState                          = NewPipelineInstance.DepthStencilState.Get();
    PSOInfo.RasterizerState                            = NewPipelineInstance.RasterizerState.Get();
    PSOInfo.VertexShader                               = NewPipelineInstance.VertexShader.Get();
    PSOInfo.PixelShader                                = NewPipelineInstance.PixelShader.Get();
    PSOInfo.RasterizerOutputFormats.DepthStencilFormat = FGlobalTextureFormats::DepthBufferFormat;

    NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
    if (!NewPipelineInstance.PipelineState)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Editor NoJitter Depth PSO %d", MaterialFlags);
        NewPipelineInstance.PipelineState->SetDebugName(DebugName);
    }

    MaterialPSOs.Add(MaterialFlags, Move(NewPipelineInstance));
}

bool FEditorNoJitterDepthPass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FEditorNoJitterDepthPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue        DepthClearValue(FGlobalTextureFormats::DepthBufferFormat, 1.0f, 0);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::DepthBufferFormat, Width, Height, 1, 1, Usage, DepthClearValue);
    TextureInfo.bEnableResourceStateTracking = true;

    FrameResources.EditorNoJitterDepth = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::PixelShaderResource);
    if (!FrameResources.EditorNoJitterDepth)
    {
        return false;
    }

    FrameResources.EditorNoJitterDepth->SetDebugName("Editor NoJitter Depth");
    return true;
}

void FEditorNoJitterDepthPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (!FrameResources.EditorNoJitterDepth)
    {
        return;
    }

    CommandList.TransitionTexture(FrameResources.EditorNoJitterDepth.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite));

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Editor NoJitter Depth");
    TRACE_SCOPE("Editor NoJitter Depth");
    GPU_TRACE_SCOPE(CommandList, "Editor NoJitter Depth");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.EditorNoJitterDepth.Get(), EAttachmentLoadAction::Clear);

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
        CHECK(PipelineState != nullptr);
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

    CommandList.TransitionTexture(FrameResources.EditorNoJitterDepth.Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource));

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Editor NoJitter Depth");
}

FEditorSelectionIDPass::FEditorSelectionIDPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
{
}

FEditorSelectionIDPass::~FEditorSelectionIDPass()
{
    MaterialPSOs.Clear();
}

void FEditorSelectionIDPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const int32 MaterialFlags = static_cast<int32>(Material->GetMaterialFlags());

    FGraphicsPipelineStateInstance* CachedPSO = MaterialPSOs.Find(MaterialFlags);
    if (CachedPSO)
    {
        return;
    }

    TArray<uint8>         ShaderCode;
    TArray<FShaderDefine> ShaderDefines;

    ShaderDefines.Emplace("USE_UNJITTERED_CAMERA", CVarEditorSelectionUseUnjitteredCamera.GetValue() ? "(1)" : "(0)");

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
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/EditorSelectionID.hlsl", CompileInfo, ShaderCode))
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
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/EditorSelectionID.hlsl", CompileInfo, ShaderCode))
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

    FRHIDepthStencilStateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc         = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.bDepthEnable      = true;
    DepthStencilStateInfo.bDepthWriteEnable = false;

    NewPipelineInstance.DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateInfo);
    if (!NewPipelineInstance.DepthStencilState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIRasterizerStateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = Material->IsDoubleSided() ? ECullMode::None : ECullMode::Back;

    NewPipelineInstance.RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateInfo);
    if (!NewPipelineInstance.RasterizerState)
    {
        DEBUG_BREAK();
        return;
    }

    FRHIBlendStateInfo BlendStateInfo;
    BlendStateInfo.NumRenderTargets = 1;
    NewPipelineInstance.BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
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
        TArray<FRHIInputElementInfo> InputElements =
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
        TArray<FRHIInputElementInfo> InputElements =
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

    FRHIGraphicsPipelineStateInfo PSOInfo;
    PSOInfo.InputLayout                                    = NewPipelineInstance.InputLayout.Get();
    PSOInfo.BlendState                                     = NewPipelineInstance.BlendState.Get();
    PSOInfo.DepthStencilState                              = NewPipelineInstance.DepthStencilState.Get();
    PSOInfo.RasterizerState                                = NewPipelineInstance.RasterizerState.Get();
    PSOInfo.VertexShader                                   = NewPipelineInstance.VertexShader.Get();
    PSOInfo.PixelShader                                    = NewPipelineInstance.PixelShader.Get();
    PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::ObjectIDFormat;
    PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = FGlobalTextureFormats::DepthBufferFormat;

    NewPipelineInstance.PipelineState = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
    if (!NewPipelineInstance.PipelineState)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Editor SelectionID PSO %d", MaterialFlags);
        NewPipelineInstance.PipelineState->SetDebugName(DebugName);
    }

    MaterialPSOs.Add(MaterialFlags, Move(NewPipelineInstance));
}

bool FEditorSelectionIDPass::Initialize(FFrameResources& FrameResources)
{
    return CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight);
}

bool FEditorSelectionIDPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    if (Width <= 0 && Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue        ClearValue(FGlobalTextureFormats::ObjectIDFormat, 0.0f, 0.0f, 0.0f, 0.0f);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(FGlobalTextureFormats::ObjectIDFormat, Width, Height, 1, 1, Usage, ClearValue);
    TextureInfo.bEnableResourceStateTracking = true;
    
    FrameResources.EditorObjectID_NoJitter = FRHI::Get()->CreateTexture(TextureInfo, EResourceAccess::PixelShaderResource);
    if (!FrameResources.EditorObjectID_NoJitter)
    {
        return false;
    }

    FrameResources.EditorObjectID_NoJitter->SetDebugName("Editor ObjectID NoJitter");
    return true;
}

void FEditorSelectionIDPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    if (!FrameResources.EditorNoJitterDepth || !FrameResources.EditorObjectID_NoJitter)
    {
        return;
    }

    CommandList.TransitionTexture(FrameResources.EditorNoJitterDepth.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite)); 
    CommandList.TransitionTexture(FrameResources.EditorObjectID_NoJitter.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Editor SelectionID");
    TRACE_SCOPE("Editor SelectionID");
    GPU_TRACE_SCOPE(CommandList, "Editor SelectionID");

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets = 1;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.EditorObjectID_NoJitter.Get(), EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.EditorNoJitterDepth.Get(), EAttachmentLoadAction::Load);

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
        CHECK(PipelineState != nullptr);
        CommandList.SetGraphicsPipelineState(PipelineState);

        CommandList.SetConstantBuffer(PipelineInstance->VertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        CommandList.SetConstantBuffer(PipelineInstance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
        CommandList.SetSamplerState(PipelineInstance->PixelShader.Get(), Material->GetMaterialSampler(), 0);

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

            if (FRHIPixelShader* PixelShader = PipelineInstance->PixelShader.Get())
            {
                CommandList.SetShaderConstants(PixelShader, &StaticMesh->GetTransformShaderData(), NumConstants);
            }

            CommandList.DrawIndexedInstanced(MeshReference.IndexCount, 1, MeshReference.StartIndex, 0, 0);
        }
    }

    CommandList.EndRenderPass();

    CommandList.TransitionTexture(FrameResources.EditorObjectID_NoJitter.Get(), FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource)); 
    CommandList.TransitionTexture(FrameResources.EditorNoJitterDepth.Get(), FRHITextureTransition::Make(EResourceAccess::DepthWrite, EResourceAccess::PixelShaderResource)); 

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Editor SelectionID");
}

#endif
