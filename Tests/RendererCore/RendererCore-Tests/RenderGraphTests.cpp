#include <Core/Containers/Array.h>
#include <RHI/RHI.h>
#include <RHI/RHIValidation.h>
#include <RendererCore/RenderGraph/RenderGraphBuilder.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>

#include "TestCommon/TestMacros.h"

#include "RenderGraphTests.h"

static constexpr uint32 TestExtent = 64;

static FRenderGraphTextureDesc MakeRenderTargetDesc()
{
    return FRenderGraphTextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1,
        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture);
}

static FRenderGraphTextureDesc MakeDepthStencilDesc()
{
    return FRenderGraphTextureDesc::CreateTexture2D(EFormat::D32_Float, TestExtent, TestExtent, 1, 1,
        ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResourceTexture, FClearValue(EFormat::D32_Float, 1.0f, 0));
}

static FRenderGraphTextureDesc MakeUnorderedAccessDesc()
{
    return FRenderGraphTextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1,
        ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture);
}

static void ResetPool()
{
    FRenderGraphResourcePool::Get().Flush();
}

static FRHITexture* RunGraphAndReturnTexture()
{
    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("PoolingProbe");

    FRenderGraphTexture* SceneColor = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "SceneColor");

    GraphBuilder.AddPass("Write", ERenderGraphPassFlags::Raster | ERenderGraphPassFlags::NeverCull,
        [SceneColor](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetRenderTarget(0, SceneColor, EAttachmentLoadAction::Clear);
        },
        [](FRHICommandList&, const FRenderGraphPassResources&)
        {
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    return SceneColor->GetRHITexture();
}

bool RenderGraphValidationSelfCheck_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A render target bound without a barrier is reported");

    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource;

    FRHITextureRef Texture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1, UsageFlags), ERHIResourceState::CopySource);
    TEST_CHECK(Texture != nullptr);

    RHIValidation::ResetErrorCount();

    FRHIBeginRenderPassDesc::FRenderTargetAttachments Attachments;
    Attachments[0] = FRHIRenderPassAttachment(Texture->GetRenderTargetView(), EAttachmentLoadAction::Load, EAttachmentStoreAction::Store);

    FRHICommandList CommandList;
    CommandList.BeginRenderPass(FRHIBeginRenderPassDesc(Attachments, 1));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(RHIValidation::GetErrorCount() > 0);

    TEST_END();
}

bool RenderGraph_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A write-then-read chain executes without validation errors");
    {
        RHIValidation::ResetErrorCount();

        FRHICommandList     CommandList;
        FRenderGraphBuilder GraphBuilder("WriteThenRead");

        FRenderGraphTexture* SceneColor = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "SceneColor");

        GraphBuilder.AddPass("Write", ERenderGraphPassFlags::Raster,
            [SceneColor](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetRenderTarget(0, SceneColor, EAttachmentLoadAction::Clear);
            },
            [](FRHICommandList&, const FRenderGraphPassResources&)
            {
            });

        GraphBuilder.AddPass("Read", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull,
            [SceneColor](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(SceneColor, ERHIResourceState::NonPixelShaderResource);
            },
            [](FRHICommandList&, const FRenderGraphPassResources&)
            {
            });

        GraphBuilder.Execute(CommandList);
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumCulledPasses, 0);
        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    TEST_SECTION("Two consecutive reads in the same state share one transition");
    {
        ResetPool();
        RHIValidation::ResetErrorCount();

        FRHICommandList     CommandList;
        FRenderGraphBuilder GraphBuilder("SharedTransition");

        FRenderGraphTexture* SceneColor = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "SceneColor");

        GraphBuilder.AddPass("Write", ERenderGraphPassFlags::Raster,
            [SceneColor](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetRenderTarget(0, SceneColor, EAttachmentLoadAction::Clear);
            },
            [](FRHICommandList&, const FRenderGraphPassResources&)
            {
            });

        for (int32 Index = 0; Index < 2; ++Index)
        {
            GraphBuilder.AddPass("Read", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull,
                [SceneColor](FRenderGraphPassBuilder& PassBuilder)
                {
                    PassBuilder.ReadTexture(SceneColor, ERHIResourceState::PixelShaderResource);
                },
                [](FRHICommandList&, const FRenderGraphPassResources&)
                {
                });
        }

        GraphBuilder.Execute(CommandList);
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumTransitionBarriers, 2);
        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    TEST_SECTION("A pass nobody reads from is culled, unless it says otherwise");
    {
        RHIValidation::ResetErrorCount();

        bool bCulledPassRan    = false;
        bool bNeverCullPassRan = false;

        FRHICommandList     CommandList;
        FRenderGraphBuilder GraphBuilder("Culling");

        FRenderGraphTexture* Unread   = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "Unread");
        FRenderGraphTexture* Retained = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "Retained");

        GraphBuilder.AddPass("Culled", ERenderGraphPassFlags::Raster,
            [Unread](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetRenderTarget(0, Unread, EAttachmentLoadAction::Clear);
            },
            [&bCulledPassRan](FRHICommandList&, const FRenderGraphPassResources&)
            {
                bCulledPassRan = true;
            });

        GraphBuilder.AddPass("Retained", ERenderGraphPassFlags::Raster | ERenderGraphPassFlags::NeverCull,
            [Retained](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetRenderTarget(0, Retained, EAttachmentLoadAction::Clear);
            },
            [&bNeverCullPassRan](FRHICommandList&, const FRenderGraphPassResources&)
            {
                bNeverCullPassRan = true;
            });

        GraphBuilder.Execute(CommandList);
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(!bCulledPassRan);
        TEST_EXPECT(bNeverCullPassRan);
        TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumCulledPasses, 1);

        // A culled pass's resource is never acquired, so only the retained one costs a texture
        TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumTexturesAllocated, 1);
        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    TEST_SECTION("Passes execute in declaration order");
    {
        TArray<int32> ExecutionOrder;

        FRHICommandList     CommandList;
        FRenderGraphBuilder GraphBuilder("Ordering");

        for (int32 Index = 0; Index < 3; ++Index)
        {
            GraphBuilder.AddPass("Ordered", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull,
                [](FRenderGraphPassBuilder&)
                {
                },
                [&ExecutionOrder, Index](FRHICommandList&, const FRenderGraphPassResources&)
                {
                    ExecutionOrder.Emplace(Index);
                });
        }

        GraphBuilder.Execute(CommandList);
        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_CHECK_EQ(ExecutionOrder.Size(), 3);
        TEST_EXPECT_EQ(ExecutionOrder[0], 0);
        TEST_EXPECT_EQ(ExecutionOrder[1], 1);
        TEST_EXPECT_EQ(ExecutionOrder[2], 2);
    }

    TEST_SECTION("An equivalent graph recycles the pooled texture");
    {
        ResetPool();
        RHIValidation::ResetErrorCount();

        FRHITexture* FirstTexture  = RunGraphAndReturnTexture();
        FRHITexture* SecondTexture = RunGraphAndReturnTexture();

        TEST_EXPECT(FirstTexture != nullptr);
        TEST_EXPECT_EQ(FirstTexture, SecondTexture);
        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    TEST_SECTION("An external resource is restored to its registered final state");
    {
        RHIValidation::ResetErrorCount();

        const ETextureUsageFlags SourceUsage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::CopySource;

        FRHITextureRef External = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1, SourceUsage),
            ERHIResourceState::CopySource);

        FRHITextureRef CopyDestination = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1, ETextureUsageFlags::CopyDest),
            ERHIResourceState::CopyDest);

        TEST_CHECK(External != nullptr);
        TEST_CHECK(CopyDestination != nullptr);

        FRHICommandList CommandList;

        {
            FRenderGraphBuilder GraphBuilder("ExternalFinalState");

            FRenderGraphTexture* Registered = GraphBuilder.RegisterExternalTexture(External.Get(), "External", ERHIResourceState::CopySource, ERHIResourceState::CopySource);

            GraphBuilder.AddPass("Write", ERenderGraphPassFlags::Raster,
                [Registered](FRenderGraphPassBuilder& PassBuilder)
                {
                    PassBuilder.SetRenderTarget(0, Registered, EAttachmentLoadAction::Clear);
                },
                [](FRHICommandList&, const FRenderGraphPassResources&)
                {
                });

            GraphBuilder.Execute(CommandList);

            TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumTransitionBarriers, 2);
        }

        CommandList.CopyTexture(CopyDestination.Get(), External.Get());

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    FRenderGraphResourcePool::Get().Flush();
    TEST_EXPECT_EQ(FRenderGraphResourcePool::Get().GetNumTextures(), 0);

    TEST_END();
}

bool RenderGraphFrame_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A frame-shaped graph drops its dead branch and barriers the rest");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHITextureRef BackBufferTexture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, TestExtent, TestExtent, 1, 1, ETextureUsageFlags::RenderTarget),
        ERHIResourceState::RenderTarget);

    TEST_CHECK(BackBufferTexture != nullptr);

    TArray<int32>          ExecutedPasses;
    FRenderGraphStatistics Statistics;
    FRHICommandList        CommandList;

    {
        FRenderGraphBuilder GraphBuilder("Frame");

        FRenderGraphTexture* Depth        = GraphBuilder.CreateTexture(MakeDepthStencilDesc(), "SceneDepth");
        FRenderGraphTexture* Albedo       = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "GBufferAlbedo");
        FRenderGraphTexture* Normal       = GraphBuilder.CreateTexture(MakeRenderTargetDesc(), "GBufferNormal");
        FRenderGraphTexture* ShadowMask   = GraphBuilder.CreateTexture(MakeUnorderedAccessDesc(), "ShadowMask");
        FRenderGraphTexture* SceneColor   = GraphBuilder.CreateTexture(MakeUnorderedAccessDesc(), "SceneColor");
        FRenderGraphTexture* Bloom        = GraphBuilder.CreateTexture(MakeUnorderedAccessDesc(), "Bloom");
        FRenderGraphTexture* DebugScratch = GraphBuilder.CreateTexture(MakeUnorderedAccessDesc(), "DebugScratch");
        FRenderGraphTexture* DebugHeatmap = GraphBuilder.CreateTexture(MakeUnorderedAccessDesc(), "DebugHeatmap");

        FRenderGraphBuffer* ObjectData = GraphBuilder.CreateBuffer(FRenderGraphBufferDesc::CreateStructuredBuffer(sizeof(uint32), 64, EBufferFlags::CopyDest), "ObjectData");

        FRenderGraphTexture* BackBuffer = GraphBuilder.RegisterExternalTexture(BackBufferTexture.Get(), "BackBuffer",
            ERHIResourceState::RenderTarget, ERHIResourceState::Present);

        GraphBuilder.AddPass("UploadObjectData", ERenderGraphPassFlags::Copy,
            [ObjectData](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.WriteBuffer(ObjectData, ERHIResourceState::CopyDest);
            },
            [&ExecutedPasses, ObjectData](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
            {
                const uint32 Payload[4] = { 0, 1, 2, 3 };
                PassCommandList.UpdateBuffer(Resources.Get(ObjectData), FBufferRegion(0, sizeof(Payload)), Payload);
                ExecutedPasses.Emplace(0);
            });

        GraphBuilder.AddPass("DepthPrepass", ERenderGraphPassFlags::Raster,
            [Depth](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetDepthStencil(Depth, EAttachmentLoadAction::Clear);
            },
            [&ExecutedPasses](FRHICommandList&, const FRenderGraphPassResources&)
            {
                ExecutedPasses.Emplace(1);
            });

        GraphBuilder.AddPass("GBuffer", ERenderGraphPassFlags::Raster,
            [Albedo, Normal, Depth, ObjectData](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.SetRenderTarget(0, Albedo, EAttachmentLoadAction::Clear);
                PassBuilder.SetRenderTarget(1, Normal, EAttachmentLoadAction::Clear);
                PassBuilder.SetDepthStencil(Depth, EAttachmentLoadAction::Load, EAttachmentStoreAction::Store, FDepthStencilValue(1.0f, 0), true);
                PassBuilder.ReadBuffer(ObjectData, ERHIResourceState::NonPixelShaderResource);
            },
            [&ExecutedPasses](FRHICommandList&, const FRenderGraphPassResources&)
            {
                ExecutedPasses.Emplace(2);
            });

        GraphBuilder.AddPass("DebugPrepare", ERenderGraphPassFlags::Compute,
            [DebugScratch](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.WriteTexture(DebugScratch, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses](FRHICommandList&, const FRenderGraphPassResources&)
            {
                ExecutedPasses.Emplace(3);
            });

        GraphBuilder.AddPass("ShadowMask", ERenderGraphPassFlags::Compute,
            [Depth, ShadowMask](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(Depth, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.WriteTexture(ShadowMask, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses, ShadowMask](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
            {
                PassCommandList.ClearUnorderedAccessViewFloat(Resources.Get(ShadowMask)->GetUnorderedAccessView(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
                ExecutedPasses.Emplace(4);
            });

        GraphBuilder.AddPass("DebugOverlay", ERenderGraphPassFlags::Compute,
            [DebugScratch, DebugHeatmap](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(DebugScratch, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.WriteTexture(DebugHeatmap, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses](FRHICommandList&, const FRenderGraphPassResources&)
            {
                ExecutedPasses.Emplace(5);
            });

        GraphBuilder.AddPass("Lighting", ERenderGraphPassFlags::Compute,
            [Albedo, Normal, ShadowMask, Depth, SceneColor](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(Albedo, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.ReadTexture(Normal, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.ReadTexture(ShadowMask, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.ReadTexture(Depth, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.WriteTexture(SceneColor, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses, SceneColor](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
            {
                PassCommandList.ClearUnorderedAccessViewFloat(Resources.Get(SceneColor)->GetUnorderedAccessView(), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
                ExecutedPasses.Emplace(6);
            });

        GraphBuilder.AddPass("BloomDownsample", ERenderGraphPassFlags::Compute,
            [SceneColor, Bloom](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(SceneColor, ERHIResourceState::NonPixelShaderResource);
                PassBuilder.WriteTexture(Bloom, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses, Bloom](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
            {
                PassCommandList.ClearUnorderedAccessViewFloat(Resources.Get(Bloom)->GetUnorderedAccessView(), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
                ExecutedPasses.Emplace(7);
            });

        GraphBuilder.AddPass("BloomBlur", ERenderGraphPassFlags::Compute,
            [Bloom](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.WriteTexture(Bloom, ERHIResourceState::UnorderedAccess);
            },
            [&ExecutedPasses, Bloom](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
            {
                PassCommandList.ClearUnorderedAccessViewFloat(Resources.Get(Bloom)->GetUnorderedAccessView(), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
                ExecutedPasses.Emplace(8);
            });

        GraphBuilder.AddPass("Tonemap", ERenderGraphPassFlags::Raster,
            [SceneColor, Bloom, BackBuffer](FRenderGraphPassBuilder& PassBuilder)
            {
                PassBuilder.ReadTexture(SceneColor, ERHIResourceState::PixelShaderResource);
                PassBuilder.ReadTexture(Bloom, ERHIResourceState::PixelShaderResource);
                PassBuilder.SetRenderTarget(0, BackBuffer, EAttachmentLoadAction::DontCare);
            },
            [&ExecutedPasses](FRHICommandList&, const FRenderGraphPassResources&)
            {
                ExecutedPasses.Emplace(9);
            });

        GraphBuilder.Execute(CommandList);
        Statistics = GraphBuilder.GetStatistics();
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBufferTexture.Get(), ERHIResourceState::Present, ERHIResourceState::RenderTarget));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT_EQ(Statistics.NumPasses, 10);
    TEST_EXPECT_EQ(Statistics.NumCulledPasses, 2);
    TEST_EXPECT_EQ(Statistics.NumTexturesAllocated, 6);
    TEST_EXPECT_EQ(Statistics.NumBuffersAllocated, 1);
    TEST_EXPECT_EQ(Statistics.NumUnorderedAccessBarriers, 1);
    TEST_EXPECT_EQ(Statistics.NumTransitionBarriers, 17);

    const int32 ExpectedOrder[] = { 0, 1, 2, 4, 6, 7, 8, 9 };
    TEST_CHECK_EQ(ExecutedPasses.Size(), int32(ARRAY_COUNT(ExpectedOrder)));

    for (int32 Index = 0; Index < ExecutedPasses.Size(); ++Index)
    {
        TEST_EXPECT_EQ(ExecutedPasses[Index], ExpectedOrder[Index]);
    }

    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}
