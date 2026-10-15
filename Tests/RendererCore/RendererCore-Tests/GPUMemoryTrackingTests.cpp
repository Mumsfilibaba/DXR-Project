#include <Core/Memory/MemoryProfiler.h>
#include <RHI/RHI.h>

#include "TestCommon/TestMacros.h"

#include "GPUMemoryTrackingTests.h"

static uint64 GetTextureAllocatedSize(const FRHITextureDesc& TextureDesc)
{
    FRHITextureRef Texture = RHI::CreateTexture(TextureDesc);
    return Texture ? Texture->GetAllocatedSize() : 0;
}

bool GPUMemoryTracking_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The NullRHI reports texture sizes across mips, layers and compressed blocks");
    {
        const ETextureUsageFlags Usage = ETextureUsageFlags::ShaderResourceTexture;
        TEST_EXPECT_EQ(GetTextureAllocatedSize(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 64, 32, 1, 1, Usage)), 64ull * 32 * 4);
        TEST_EXPECT_EQ(GetTextureAllocatedSize(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 64, 64, 7, 1, Usage)), 4ull * (4096 + 1024 + 256 + 64 + 16 + 4 + 1));
        TEST_EXPECT_EQ(GetTextureAllocatedSize(FRHITextureDesc::CreateTexture2D(EFormat::BC1_UNorm, 128, 128, 1, 1, Usage)), 32ull * 32 * 8);
        TEST_EXPECT_EQ(GetTextureAllocatedSize(FRHITextureDesc::CreateTextureCube(EFormat::R16G16B16A16_Float, 16, 1, 1, Usage)), 16ull * 16 * 8 * 6);

        FRHIBufferRef Buffer = RHI::CreateBuffer(FRHIBufferDesc::CreateStructuredBuffer(16, 64));
        TEST_CHECK(Buffer != nullptr);
        TEST_EXPECT_EQ(Buffer->GetAllocatedSize(), 1024ull);
    }

#if ENABLE_MEMORY_PROFILER
    FMemoryProfiler& Profiler = FMemoryProfiler::Get();

    const FMemoryNameId CallerTag  = Profiler.InternDynamic("GPUMemoryTest.Caller");
    const FMemoryNameId BackendTag = Profiler.InternDynamic("NullRHI");

    FRHICommandListExecutor::Get().FlushDeletedResources();

    TEST_SECTION("A texture is billed to the caller's tag and scope at the size it reports, while the backend's own allocations go to the backend");
    {
        const int64 BackendAllocationsBefore = Profiler.GetTagStats(BackendTag).TotalAllocations;

        FRHITextureRef Texture;
        FMemoryScopeId Scope = MEMORY_SCOPE_ROOT;
        {
            MEMORY_MODULE_SCOPE_ID(CallerTag);
            MEMORY_SCOPE("GPUMemoryTest.Scope");
            Scope   = Profiler.GetCurrentScope();
            Texture = RHI::CreateTexture(FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 64, 32, 1, 1, ETextureUsageFlags::ShaderResourceTexture));
        }

        TEST_CHECK(Texture != nullptr);

        const FRHIResource* Resource = Texture.Get();
        const int64         Size     = static_cast<int64>(Texture->GetAllocatedSize());

        FMemoryGPUResourceInfo Info;
        TEST_CHECK(Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT(Info.Kind == EMemoryGPUResourceKind::Texture);
        TEST_EXPECT(Info.State == EMemoryGPUResourceState::Live);
        TEST_EXPECT_EQ(static_cast<int64>(Info.Size), Size);
        TEST_EXPECT_EQ(Info.Tag, CallerTag);
        TEST_EXPECT_EQ(Info.Scope, Scope);

        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).CurrentBytes, Size);
        TEST_EXPECT_EQ(Profiler.GetGPUScopeStats(Scope).CurrentBytes, Size);
        TEST_EXPECT(Profiler.GetTagStats(BackendTag).TotalAllocations > BackendAllocationsBefore);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(BackendTag).TotalAllocations, 0);

        Texture->SetDebugName("GPUMemoryTest.Texture");
        TEST_CHECK(Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT(Info.Name == "GPUMemoryTest.Texture");

        TEST_SECTION("A released texture stays counted as pending deletion until the deletion flush destroys it");
        const int64 PendingBefore = Profiler.GetGPUPendingDeletionBytes();
        Texture.Reset();

        TEST_CHECK(Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT(Info.State == EMemoryGPUResourceState::PendingDeletion);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).CurrentBytes, Size);
        TEST_EXPECT_EQ(Profiler.GetGPUPendingDeletionBytes() - PendingBefore, Size);

        FRHICommandListExecutor::Get().FlushDeletedResources();

        TEST_EXPECT(!Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).CurrentBytes, 0);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).PeakBytes, Size);
        TEST_EXPECT_EQ(Profiler.GetGPUScopeStats(Scope).CurrentBytes, 0);
        TEST_EXPECT_EQ(Profiler.GetGPUPendingDeletionBytes(), PendingBefore);
    }

    TEST_SECTION("A buffer is billed the same way, and resizing it moves its tag's bytes");
    {
        FRHIBufferRef Buffer;
        {
            MEMORY_MODULE_SCOPE_ID(CallerTag);
            Buffer = RHI::CreateBuffer(FRHIBufferDesc::CreateStructuredBuffer(16, 64));
        }

        TEST_CHECK(Buffer != nullptr);

        const FRHIResource* Resource = Buffer.Get();

        FMemoryGPUResourceInfo Info;
        TEST_CHECK(Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT(Info.Kind == EMemoryGPUResourceKind::Buffer);
        TEST_EXPECT_EQ(Info.Size, 1024ull);
        TEST_EXPECT_EQ(Info.Tag, CallerTag);
        TEST_EXPECT_EQ(Info.Scope, MEMORY_SCOPE_ROOT);

        Profiler.OnGPUResourceResized(Resource, 4096);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).CurrentBytes, 4096);
        TEST_EXPECT(Profiler.GetGPUTagStats(CallerTag).PeakBytes >= 4096);

        Buffer.Reset();
        FRHICommandListExecutor::Get().FlushDeletedResources();

        TEST_EXPECT(!Profiler.GetGPUResource(Resource, Info));
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).CurrentBytes, 0);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).NumAllocations, 0);
        TEST_EXPECT_EQ(Profiler.GetGPUTagStats(CallerTag).TotalAllocations, 2);
    }

    Profiler.LogTags();
#endif

    TEST_END();
}
