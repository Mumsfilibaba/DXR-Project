#include "ApplicationRendererTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Application.h>
#include <Application/IApplicationRenderer.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/BorderElement.h>
#include <Application/Elements/WindowElement.h>

class FStubApplicationRenderer final : public IApplicationRenderer
{
public:
    FStubApplicationRenderer()
        : Commands()
        , BeginCount(0)
        , EndCount(0)
        , DestroyedCount(0)
        , LastCommandCount(0)
        , LastBeginWindow(nullptr)
        , LastDestroyedWindow(nullptr)
        , bRefuseWindows(false)
        , bWindowWasStillAlive(false)
    {
    }

    // IApplicationRenderer Interface Overrides
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindowElement>& InWindow) override final
    {
        ++BeginCount;
        LastBeginWindow = InWindow.Get();

        if (bRefuseWindows)
        {
            return nullptr;
        }

        Commands.Reset();
        return &Commands;
    }

    virtual void EndWindow(const TSharedPtr<FWindowElement>&) override final
    {
        ++EndCount;
        LastCommandCount = Commands.Size();
    }

    virtual void OnWindowDestroyed(const TSharedPtr<FWindowElement>& InWindow) override final
    {
        ++DestroyedCount;

        LastDestroyedWindow  = InWindow.Get();
        bWindowWasStillAlive = InWindow && InWindow->GetPlatformWindow()->IsValid();
    }

    FDrawCommandList Commands;
    int32            BeginCount;
    int32            EndCount;
    int32            DestroyedCount;
    int32            LastCommandCount;
    FWindowElement*  LastBeginWindow;
    FWindowElement*  LastDestroyedWindow;
    bool             bRefuseWindows;
    bool             bWindowWasStillAlive;
};

bool ApplicationRendererWindowPass_Test()
{
    TEST_BEGIN();

    TSharedPtr<FApplication>             Application = CreateStubApplication();
    TSharedPtr<FStubApplicationRenderer> Renderer    = MakeSharedPtr<FStubApplicationRenderer>();

    Application->SetRenderer(Renderer);
    TEST_EXPECT(Application->GetRenderer() == Renderer);

    TSharedPtr<FWindowElement> Window = CreateStubWindow(Application, IntVector2(800, 600));
    TEST_EXPECT(Window->GetPlatformWindow() != nullptr);

    FBorderElement::FInitializer BorderInitializer;
    BorderInitializer.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    Window->SetContent(FBorderElement::Create(BorderInitializer));

    Application->Tick(0.0f);

    TEST_SECTION("A visible window opens and closes exactly one list");
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 1);
    TEST_EXPECT_EQ(Renderer->EndCount, 1);
    TEST_EXPECT(Renderer->LastBeginWindow == Window.Get());

    TEST_SECTION("The window paints its content into the list the renderer handed out");
    TEST_EXPECT(Renderer->LastCommandCount > 0);
    TEST_EXPECT_EQ(Renderer->Commands.CountCommandsOfType(EDrawCommandType::Box), 1);

    TEST_SECTION("The overlay is painted above the content");
    FBorderElement::FInitializer OverlayInitializer;
    OverlayInitializer.BackgroundColor = FFloatColor(0.5f, 0.0f, 0.0f, 1.0f);

    Window->SetOverlay(FBorderElement::Create(OverlayInitializer));
    Application->Tick(0.0f);
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->Commands.CountCommandsOfType(EDrawCommandType::Box), 2);
    TEST_EXPECT(Renderer->Commands[1].LayerId > Renderer->Commands[0].LayerId);

    TEST_SECTION("A hidden window is skipped before the list is opened");
    Window->SetVisibility(EVisibility::Hidden);
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 2);
    TEST_EXPECT_EQ(Renderer->EndCount, 2);

    Window->SetVisibility(EVisibility::Visible);

    TEST_SECTION("A renderer that refuses a window is never asked to close its list");
    Renderer->bRefuseWindows = true;
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 3);
    TEST_EXPECT_EQ(Renderer->EndCount, 2);

    Renderer->bRefuseWindows = false;

    TEST_SECTION("Without a renderer the pass does nothing at all");
    Application->SetRenderer(nullptr);
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 3);
    TEST_EXPECT(!Application->GetRenderer());

    Application->DestroyWindow(Window);

    TEST_END();
}

bool ApplicationRendererWindowLifetime_Test()
{
    TEST_BEGIN();

    TSharedPtr<FApplication>             Application = CreateStubApplication();
    TSharedPtr<FStubApplicationRenderer> Renderer    = MakeSharedPtr<FStubApplicationRenderer>();

    Application->SetRenderer(Renderer);

    TSharedPtr<FWindowElement> First  = CreateStubWindow(Application, IntVector2(400, 300));
    TSharedPtr<FWindowElement> Second = CreateStubWindow(Application, IntVector2(400, 300));

    Application->Tick(0.0f);

    TEST_SECTION("Every visible window gets its own list");
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 2);
    TEST_EXPECT_EQ(Renderer->EndCount, 2);

    TEST_SECTION("A destroyed window is reported before the platform window goes away");
    Application->DestroyWindow(Second);

    TEST_EXPECT_EQ(Renderer->DestroyedCount, 1);
    TEST_EXPECT(Renderer->LastDestroyedWindow == Second.Get());
    TEST_EXPECT(Renderer->bWindowWasStillAlive);

    TEST_SECTION("The destroyed window no longer takes part in the pass");
    Second.Reset();
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 3);
    TEST_EXPECT_EQ(Renderer->EndCount, 3);

    Application->DestroyWindow(First);
    TEST_EXPECT_EQ(Renderer->DestroyedCount, 2);

    TEST_END();
}
