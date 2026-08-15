#include <Core/Containers/Array.h>
#include <RHI/RHI.h>
#include <RHI/RHIValidation.h>
#include <RendererCore/RenderGraph/RenderGraphBuilder.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>

#include "TestCommon/TestMacros.h"

#include "RenderGraphTests.h"

static constexpr uint32 ViewTestExtent = 64;

static void ResetPool()
{
    FRenderGraphResourcePool::Get().Flush();
}

bool RenderGraphViewSlice_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Per-slice SRV and UAV use graph view handles");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("ViewSlice");

    FRenderGraphTexture* TextureArray = GraphBuilder.CreateTexture( FRenderGraphTextureDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, ViewTestExtent, ViewTestExtent, 2, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture), "TextureArray");

    const FRHIShaderResourceViewDesc SliceSRVDesc = FRHIShaderResourceViewDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, 0, 1, 1, 1);
    const FRHIUnorderedAccessViewDesc SliceUAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, 0, 1, 1);

    FRenderGraphShaderResourceView*  SliceSRV = GraphBuilder.CreateSRV(TextureArray, SliceSRVDesc, "SliceSRV");
    FRenderGraphUnorderedAccessView* SliceUAV = GraphBuilder.CreateUAV(TextureArray, SliceUAVDesc, "SliceUAV");

    bool bWriteRan = false;
    bool bReadRan  = false;

    GraphBuilder.AddPass("WriteSlice", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [SliceUAV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Write(SliceUAV);
        },
        [&bWriteRan, SliceUAV](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
        {
            FRHIUnorderedAccessView* UAV = Resources.Get(SliceUAV);
            TEST_CHECK(UAV != nullptr);
            PassCommandList.ClearUnorderedAccessViewFloat(UAV, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            bWriteRan = true;
        });

    GraphBuilder.AddPass("ReadSlice", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [SliceSRV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Read(SliceSRV);
        },
        [&bReadRan, SliceSRV](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(SliceSRV) != nullptr);
            bReadRan = true;
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(bWriteRan);
    TEST_EXPECT(bReadRan);
    TEST_EXPECT(GraphBuilder.GetStatistics().NumSubresourceBarriers > 0);
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}

bool RenderGraphViewUndeclared_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Undeclared views are not resolved");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("UndeclaredView");

    FRenderGraphTexture* Texture = GraphBuilder.CreateTexture( FRenderGraphTextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, ViewTestExtent, ViewTestExtent, 1, 1, ETextureUsageFlags::ShaderResourceTexture), "Texture");

    FRenderGraphShaderResourceView* SliceSRV = GraphBuilder.CreateSRV(Texture, FRHIShaderResourceViewDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 0, 1), "SliceSRV");

    bool bUndeclaredViewNull = false;

    GraphBuilder.AddPass("Empty", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [](FRenderGraphPassBuilder&)
        {
        },
        [&bUndeclaredViewNull, SliceSRV](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            bUndeclaredViewNull = (Resources.Get(SliceSRV) == nullptr);
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(bUndeclaredViewNull);
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}

bool RenderGraphViewBufferRange_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Buffer SRV and UAV views declare element ranges");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("BufferView");

    FRenderGraphBuffer* Buffer = GraphBuilder.CreateBuffer( FRenderGraphBufferDesc::CreateStructuredBuffer(sizeof(uint32), 16, EBufferFlags::UnorderedAccessBuffer), "StructuredBuffer");

    FRenderGraphUnorderedAccessView* BufferUAV = GraphBuilder.CreateUAV(Buffer, FRHIUnorderedAccessViewDesc::CreateBuffer(0, 8), "BufferUAV");
    FRenderGraphShaderResourceView* BufferSRV = GraphBuilder.CreateSRV(Buffer, FRHIShaderResourceViewDesc::CreateBuffer(0, 8), "BufferSRV");

    GraphBuilder.AddPass("WriteBuffer", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [BufferUAV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Write(BufferUAV);
        },
        [BufferUAV](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(BufferUAV) != nullptr);
            const uint32 ClearValues[4] = { 1, 0, 0, 0 };
            PassCommandList.ClearUnorderedAccessViewUint(Resources.Get(BufferUAV), ClearValues);
        });

    GraphBuilder.AddPass("ReadBuffer", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [BufferSRV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Read(BufferSRV);
        },
        [BufferSRV](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(BufferSRV) != nullptr);
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(GraphBuilder.GetStatistics().NumViewsCreated >= 2);
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}

bool RenderGraphViewValidation_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Invalid view creation marks the graph as having errors");

    ResetPool();

    FRenderGraphBuilder GraphBuilder("InvalidView");

    FRenderGraphTexture* Texture = GraphBuilder.CreateTexture( FRenderGraphTextureDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, ViewTestExtent, ViewTestExtent, 2, 1, 1, ETextureUsageFlags::ShaderResourceTexture), "TextureArray");

    GraphBuilder.CreateSRV(Texture, FRHIShaderResourceViewDesc::CreateTexture2DArray( EFormat::R8G8B8A8_Unorm, 0, 1, 4, 1), "OutOfBoundsSRV");

    TEST_EXPECT(GraphBuilder.HasErrors());

    TEST_END();
}

bool RenderGraphViewTypelessFormat_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Typeless storage accepts typed SRV and UAV views");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("TypelessFormat");

    FRenderGraphTexture* Texture = GraphBuilder.CreateTexture( FRenderGraphTextureDesc::CreateTexture2D(EFormat::R32_Typeless, ViewTestExtent, ViewTestExtent, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture), "TypelessR32");

    FRenderGraphShaderResourceView* FloatSRV = GraphBuilder.CreateSRV(Texture, FRHIShaderResourceViewDesc::CreateTexture2D(EFormat::R32_Float, 0, 1), "R32FloatSRV");
    FRenderGraphUnorderedAccessView* UintUAV = GraphBuilder.CreateUAV(Texture, FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32_Uint, 0), "R32UintUAV");

    TEST_CHECK(FloatSRV != nullptr);
    TEST_CHECK(UintUAV != nullptr);
    TEST_CHECK(!GraphBuilder.HasErrors());

    bool bWriteRan = false;
    bool bReadRan  = false;

    GraphBuilder.AddPass("WriteTypedUAV", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [UintUAV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Write(UintUAV);
        },
        [&bWriteRan, UintUAV](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(UintUAV) != nullptr);
            const uint32 ClearValues[4] = { 42, 0, 0, 0 };
            PassCommandList.ClearUnorderedAccessViewUint(Resources.Get(UintUAV), ClearValues);
            bWriteRan = true;
        });

    GraphBuilder.AddPass("ReadTypedSRV", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [FloatSRV](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Read(FloatSRV);
        },
        [&bReadRan, FloatSRV](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(FloatSRV) != nullptr);
            bReadRan = true;
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(bWriteRan);
    TEST_EXPECT(bReadRan);
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}

bool RenderGraphViewHybridUsage_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Texture with render-target, unordered-access, and shader-resource views compiles and executes");

    ResetPool();
    RHIValidation::ResetErrorCount();

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("HybridUsage");

    FRenderGraphTexture* Texture = GraphBuilder.CreateTexture(FRenderGraphTextureDesc::CreateTexture2D(EFormat::R16G16B16A16_Float, ViewTestExtent, ViewTestExtent, 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture), "HybridTarget");

    FRenderGraphUnorderedAccessView* UnorderedAccessView = GraphBuilder.CreateUAV(Texture, FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R16G16B16A16_Float, 0), "HybridUnorderedAccessView");
    FRenderGraphRenderTargetView* RenderTargetView = GraphBuilder.CreateRTV(Texture, FRHIRenderTargetViewDesc::CreateTexture2D(EFormat::R16G16B16A16_Float, 0), "HybridRenderTargetView");
    FRenderGraphShaderResourceView* ShaderResourceView = GraphBuilder.CreateSRV(Texture, FRHIShaderResourceViewDesc::CreateTexture2D(EFormat::R16G16B16A16_Float, 0, 1), "HybridShaderResourceView");

    TEST_CHECK(UnorderedAccessView != nullptr);
    TEST_CHECK(RenderTargetView != nullptr);
    TEST_CHECK(ShaderResourceView != nullptr);
    TEST_CHECK(!GraphBuilder.HasErrors());

    GraphBuilder.AddPass("Write", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [UnorderedAccessView](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Write(UnorderedAccessView);
        },
        [UnorderedAccessView](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(UnorderedAccessView) != nullptr);
            PassCommandList.ClearUnorderedAccessViewFloat(Resources.Get(UnorderedAccessView), Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        });

    GraphBuilder.AddPass("Draw", ERenderGraphPassFlags::Raster | ERenderGraphPassFlags::NeverCull, [RenderTargetView](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.SetRenderTarget(0, RenderTargetView, EAttachmentLoadAction::Load);
        },
        [](FRHICommandList&, const FRenderGraphPassResources&)
        {
        });

    GraphBuilder.AddPass("Read", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [ShaderResourceView](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.Read(ShaderResourceView);
        },
        [ShaderResourceView](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(ShaderResourceView) != nullptr);
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(!GraphBuilder.HasErrors());
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}

bool RenderGraphSubresourceRejoin_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Reading a whole texture after per-slice writes transitions every slice");

    ResetPool();
    RHIValidation::ResetErrorCount();

    constexpr uint32 NumAtlasSlices = 4;

    FRHITextureRef AtlasTexture = RHI::CreateTexture( FRHITextureDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, ViewTestExtent, ViewTestExtent, NumAtlasSlices, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture), ERHIResourceState::NonPixelShaderResource);

    TEST_CHECK(AtlasTexture.IsValid());

    FRHICommandList     CommandList;
    FRenderGraphBuilder GraphBuilder("SubresourceRejoin");

    FRenderGraphTexture* Atlas = GraphBuilder.RegisterExternalTexture(AtlasTexture.Get(), "Atlas", ERHIResourceState::NonPixelShaderResource, ERHIResourceState::NonPixelShaderResource);

    TEST_CHECK(Atlas != nullptr);

    FRenderGraphUnorderedAccessView* SliceViews[NumAtlasSlices] = { };
    for (uint32 SliceIndex = 0; SliceIndex < NumAtlasSlices; ++SliceIndex)
    {
        SliceViews[SliceIndex] = GraphBuilder.CreateUAV(Atlas, FRHIUnorderedAccessViewDesc::CreateTexture2DArray(EFormat::R8G8B8A8_Unorm, 0, static_cast<uint16>(SliceIndex), 1), "AtlasSliceUnorderedAccessView");

        TEST_CHECK(SliceViews[SliceIndex] != nullptr);
    }

    bool bReadRan = false;

    GraphBuilder.AddPass("WriteSlices", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [&SliceViews](FRenderGraphPassBuilder& PassBuilder)
        {
            for (FRenderGraphUnorderedAccessView* SliceView : SliceViews)
            {
                PassBuilder.Write(SliceView);
            }
        },
        [&SliceViews](FRHICommandList& PassCommandList, const FRenderGraphPassResources& Resources)
        {
            for (FRenderGraphUnorderedAccessView* SliceView : SliceViews)
            {
                FRHIUnorderedAccessView* UnorderedAccessView = Resources.Get(SliceView);
                TEST_CHECK(UnorderedAccessView != nullptr);
                PassCommandList.ClearUnorderedAccessViewFloat(UnorderedAccessView, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            }
        });

    GraphBuilder.AddPass("ReadWhole", ERenderGraphPassFlags::Compute | ERenderGraphPassFlags::NeverCull, [Atlas](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Atlas, ERHIResourceState::NonPixelShaderResource);
        },
        [&bReadRan, Atlas](FRHICommandList&, const FRenderGraphPassResources& Resources)
        {
            TEST_CHECK(Resources.Get(Atlas) != nullptr);
            bReadRan = true;
        });

    GraphBuilder.Execute(CommandList);
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    TEST_EXPECT(bReadRan);

    TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumSubresourceBarriers, int32(NumAtlasSlices));
    TEST_EXPECT_EQ(GraphBuilder.GetStatistics().NumTransitionBarriers, int32(NumAtlasSlices) + 1);
    TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);

    TEST_END();
}
