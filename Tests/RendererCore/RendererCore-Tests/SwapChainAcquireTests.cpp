#include <RHI/RHI.h>
#include <RHI/RHIValidation.h>

#include "TestCommon/TestMacros.h"

#include "SwapChainAcquireTests.h"

static constexpr uint16 TestExtent = 64;

static FRHISwapChainRef CreateSwapChain()
{
    // NullRHI never looks at the handle, but the validation layer refuses a null one
    static int32 WindowHandlePlaceholder = 0;
    return RHI::CreateSwapChain(FRHISwapChainDesc(&WindowHandlePlaceholder, EFormat::B8G8R8A8_Unorm, TestExtent, TestExtent));
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

        for (int32 Frame = 0; Frame < 3; ++Frame)
        {
            CommandList.AcquireNextBackBuffer(SwapChain.Get());
            TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::Undefined, ERHIResourceState::RenderTarget);
            CommandList.ClearRenderTargetView(SwapChain->GetBackBufferRenderTargetView(), Vector4());
            TransitionBackBuffer(CommandList, SwapChain.Get(), ERHIResourceState::RenderTarget, ERHIResourceState::Present);
            CommandList.PresentSwapChain(SwapChain.Get(), false);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

        TEST_EXPECT_EQ(RHIValidation::GetErrorCount(), 0);
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

    TEST_END();
}
