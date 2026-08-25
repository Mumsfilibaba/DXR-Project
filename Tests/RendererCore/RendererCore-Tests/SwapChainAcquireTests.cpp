#include <RHI/RHI.h>
#include <RHI/RHIValidation.h>

#include "TestCommon/TestMacros.h"

#include "SwapChainAcquireTests.h"

static constexpr uint16 TestExtent = 64;

static FRHISwapChainRef CreateSwapChain(ESwapChainUsageFlags Usage = ESwapChainUsageFlags::RenderTarget)
{
    // NullRHI never looks at the handle, but the validation layer refuses a null one
    static int32 WindowHandlePlaceholder = 0;

    FRHISwapChainDesc SwapChainDesc(&WindowHandlePlaceholder, EFormat::B8G8R8A8_Unorm, TestExtent, TestExtent);
    SwapChainDesc.Usage = Usage;
    return RHI::CreateSwapChain(SwapChainDesc);
}

static void TransitionBackBuffer(FRHICommandList& CommandList, FRHISwapChain* SwapChain, ERHIResourceState BeforeState, ERHIResourceState AfterState)
{
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(SwapChain->GetBackBuffer(), BeforeState, AfterState));
}

bool SwapChainAcquire_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Acquire, overwrite and present runs clean, frame after frame");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;

        FRHITexture* FirstBackBuffer = SwapChain->GetBackBuffer();

        for (int32 Frame = 0; Frame < 3; ++Frame)
        {
            CommandList.AcquireNextBackBuffer(SwapChain.Get());
            TEST_EXPECT_EQ(SwapChain->GetBackBuffer(), FirstBackBuffer);

            TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Undefined, ERHIResourceState::RenderTarget);
            CommandList.ClearRenderTargetView(SwapChain->GetRenderTargetView(), Vector4());
            TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::Present);
            CommandList.PresentSwapChain(SwapChain.Get(), false);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
    }

    TEST_SECTION("The back-buffer views are the texture's own views");
    {
        FRHISwapChainRef SwapChain = CreateSwapChain(ESwapChainUsageFlags::RenderTarget | ESwapChainUsageFlags::UnorderedAccess);
        TEST_CHECK(SwapChain != nullptr);

        FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
        TEST_CHECK(BackBuffer != nullptr);

        TEST_CHECK(SwapChain->GetRenderTargetView() != nullptr);
        TEST_EXPECT_EQ(SwapChain->GetRenderTargetView(), BackBuffer->GetRenderTargetView());

        TEST_CHECK(SwapChain->GetUnorderedAccessView() != nullptr);
        TEST_EXPECT_EQ(SwapChain->GetUnorderedAccessView(), BackBuffer->GetUnorderedAccessView());
    }

    TEST_SECTION("A back buffer asked for ShaderResource carries a shader-resource view");
    {
        FRHISwapChainRef SwapChain = CreateSwapChain(ESwapChainUsageFlags::RenderTarget | ESwapChainUsageFlags::ShaderResource);
        TEST_CHECK(SwapChain != nullptr);

        FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
        TEST_CHECK(BackBuffer != nullptr);

        TEST_CHECK(SwapChain->GetShaderResourceView() != nullptr);
        TEST_EXPECT_EQ(SwapChain->GetShaderResourceView(), BackBuffer->GetShaderResourceView());
    }

    TEST_SECTION("A back buffer not asked for ShaderResource carries no shader-resource view");
    {
        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
        TEST_CHECK(BackBuffer != nullptr);

        TEST_EXPECT_EQ(SwapChain->GetShaderResourceView(), nullptr);
        TEST_EXPECT_EQ(BackBuffer->GetShaderResourceView(), nullptr);
    }

    TEST_SECTION("ShaderResource on its own is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain(ESwapChainUsageFlags::ShaderResource);

        TEST_EXPECT_EQ(SwapChain.Get(), nullptr);
        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("A second acquire without a present in between is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.AcquireNextBackBuffer(SwapChain.Get());
        CommandList.AcquireNextBackBuffer(SwapChain.Get());

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("Presenting without acquiring is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.PresentSwapChain(SwapChain.Get(), false);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("Presenting a back-buffer nothing wrote is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.AcquireNextBackBuffer(SwapChain.Get());

        // Reaching Present through barriers alone says nothing about the pixels
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Undefined, ERHIResourceState::RenderTarget);
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::Present);
        CommandList.PresentSwapChain(SwapChain.Get(), false);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("A Present before-state after an acquire is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.AcquireNextBackBuffer(SwapChain.Get());
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Present, ERHIResourceState::RenderTarget);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("Undefined is refused as an after-state");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.AcquireNextBackBuffer(SwapChain.Get());
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Undefined, ERHIResourceState::RenderTarget);
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::Undefined);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("Writing a back-buffer that was never acquired is refused");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Undefined, ERHIResourceState::RenderTarget);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
    }

    TEST_SECTION("A resize invalidates the acquire it interrupts");
    {
        RHIValidation::ResetErrorCount();

        FRHISwapChainRef SwapChain = CreateSwapChain();
        TEST_CHECK(SwapChain != nullptr);

        FRHICommandList CommandList;
        CommandList.AcquireNextBackBuffer(SwapChain.Get());
        CommandList.ResizeSwapChain(SwapChain.Get(), TestExtent * 2, TestExtent * 2);
        CommandList.PresentSwapChain(SwapChain.Get(), false);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT(RHIValidation::GetErrorCount() > 0);
        TEST_EXPECT_EQ(SwapChain->GetDesc().Width, uint16(TestExtent * 2));
    }

    TEST_SECTION("A resize keeps the back-buffer texture and its views");
    {
        FRHISwapChainRef SwapChain = CreateSwapChain(ESwapChainUsageFlags::RenderTarget | ESwapChainUsageFlags::UnorderedAccess | ESwapChainUsageFlags::ShaderResource);
        TEST_CHECK(SwapChain != nullptr);

        FRHITexture*             BackBuffer          = SwapChain->GetBackBuffer();
        FRHIRenderTargetView*    RenderTargetView    = SwapChain->GetRenderTargetView();
        FRHIUnorderedAccessView* UnorderedAccessView = SwapChain->GetUnorderedAccessView();
        FRHIShaderResourceView*  ShaderResourceView  = SwapChain->GetShaderResourceView();

        FRHICommandList CommandList;
        CommandList.ResizeSwapChain(SwapChain.Get(), TestExtent * 2, TestExtent * 2);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(SwapChain->GetBackBuffer(), BackBuffer);
        TEST_EXPECT_EQ(SwapChain->GetRenderTargetView(), RenderTargetView);
        TEST_EXPECT_EQ(SwapChain->GetUnorderedAccessView(), UnorderedAccessView);
        TEST_EXPECT_EQ(SwapChain->GetShaderResourceView(), ShaderResourceView);
    }

    TEST_END();
}
