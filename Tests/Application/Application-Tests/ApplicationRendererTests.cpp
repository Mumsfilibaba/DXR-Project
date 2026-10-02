#include "ApplicationRendererTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Application.h>
#include <Application/IApplicationRenderer.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Window.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/PopupWindow.h>
#include <Application/Style/UIStyle.h>
#include <RHI/RHITexture.h>

class FStubApplicationRenderer final : public IApplicationRenderer
{
public:
    FStubApplicationRenderer()
        : Commands()
        , BeginCount(0)
        , EndCount(0)
        , DestroyedCount(0)
        , EnsuredCount(0)
        , LastCommandCount(0)
        , LastBeginWindow(nullptr)
        , LastDestroyedWindow(nullptr)
        , LastEnsuredWindow(nullptr)
        , bRefuseWindows(false)
        , bWindowWasStillAlive(false)
    {
    }

    // IApplicationRenderer Interface Overrides
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindow>& InWindow) override final
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

    virtual void EndWindow(const TSharedPtr<FWindow>&) override final
    {
        ++EndCount;
        LastCommandCount = Commands.Size();
    }

    virtual void OnWindowDestroyed(const TSharedPtr<FWindow>& InWindow) override final
    {
        ++DestroyedCount;

        LastDestroyedWindow  = InWindow.Get();
        bWindowWasStillAlive = InWindow && InWindow->GetPlatformWindow()->IsValid();
    }

    virtual FRHITextureRef RenderElementToTexture(const TSharedPtr<FVisualElement>&, const IntVector2&, float) override final
    {
        return nullptr;
    }

    virtual void RetireTexture(const FRHITextureRef&) override final
    {
    }

    virtual void SetPrimaryWindow(const TSharedPtr<FWindow>&) override final
    {
    }

    virtual void EnsureWindowSurface(const TSharedPtr<FWindow>& InWindow) override final
    {
        ++EnsuredCount;
        LastEnsuredWindow = InWindow.Get();
    }

    virtual FRHISwapChainRef GetWindowSwapChain(const TSharedPtr<FWindow>&) const override final
    {
        return nullptr;
    }

    FDrawCommandList Commands;
    int32            BeginCount;
    int32            EndCount;
    int32            DestroyedCount;
    int32            EnsuredCount;
    int32            LastCommandCount;
    FWindow*         LastBeginWindow;
    FWindow*         LastDestroyedWindow;
    FWindow*         LastEnsuredWindow;
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

    TSharedPtr<FWindow> Window = CreateStubWindow(Application, IntVector2(800, 600));
    TEST_EXPECT(Window->GetPlatformWindow() != nullptr);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    Window->SetContent(FBorder::Create(BorderDesc));

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
    FBorder::FDesc OverlayDesc;
    OverlayDesc.BackgroundColor = FFloatColor(0.5f, 0.0f, 0.0f, 1.0f);

    Window->SetOverlay(FBorder::Create(OverlayDesc));
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

bool ApplicationRendererExternalSurface_Test()
{
    TEST_BEGIN();

    TSharedPtr<FApplication>             Application = CreateStubApplication();
    TSharedPtr<FStubApplicationRenderer> Renderer    = MakeSharedPtr<FStubApplicationRenderer>();

    Application->SetRenderer(Renderer);

    TEST_SECTION("A window's surface is the renderer's unless the window says otherwise");
    TSharedPtr<FWindow> Owned = CreateStubWindow(Application, IntVector2(400, 300));
    TEST_EXPECT(!Owned->HasExternalSurface());

    TEST_SECTION("An ImGui viewport says otherwise, which is what keeps a second swap chain off its native window");
    FWindow::FDesc ExternalDesc;
    ExternalDesc.Title               = "Viewport";
    ExternalDesc.Size                = IntVector2(400, 300);
    ExternalDesc.bHasExternalSurface = true;

    TSharedPtr<FWindow> External = FWindow::Create(ExternalDesc);
    Application->CreateWindow(External);

    TEST_EXPECT(External->HasExternalSurface());

    TEST_SECTION("It is a window like any other otherwise, so it is still registered and still drawn");
    TEST_EXPECT_EQ(Application->GetWindows().Size(), 2);

    Application->Tick(0.0f);
    Application->DrawWindows();

    TEST_EXPECT_EQ(Renderer->BeginCount, 2);
    TEST_EXPECT_EQ(Renderer->EndCount, 2);

    Application->DestroyWindow(External);
    Application->DestroyWindow(Owned);

    TEST_END();
}

bool ApplicationRendererWindowLifetime_Test()
{
    TEST_BEGIN();

    TSharedPtr<FApplication>             Application = CreateStubApplication();
    TSharedPtr<FStubApplicationRenderer> Renderer    = MakeSharedPtr<FStubApplicationRenderer>();

    Application->SetRenderer(Renderer);

    TSharedPtr<FWindow> First  = CreateStubWindow(Application, IntVector2(400, 300));
    TSharedPtr<FWindow> Second = CreateStubWindow(Application, IntVector2(400, 300));

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

bool ApplicationRendererPopupSurface_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FUIStyle::ResetDefault();

    const float         MenuRadius     = FUIStyle::GetDefault().Menu.CornerRadius;
    TSharedPtr<FWindow> Parent         = Application.CreateWindow(IntVector2(800, 600));
    const float         ExpectedRadius = (Popups::ResolveCornerRounding() == EPopupCornerRounding::Content) ? MenuRadius : 0.0f;

    TEST_SECTION("With no renderer at all a popup still opens, and its content rounds by what the platform and RHI allow");
    TSharedPtr<FMenu>   NoRendererContent = FMenu::Create();
    TSharedPtr<FWindow> NoRendererPopup   = Popups::Open(Parent, FRectangle(IntVector2(0, 0), 200, 100), NoRendererContent);

    TEST_EXPECT(NoRendererPopup != nullptr);
    TEST_EXPECT_EQ(NoRendererContent->GetStyle().CornerRadius, ExpectedRadius);

    Popups::Close(NoRendererPopup);

    TEST_SECTION("A renderer holding no surface for it is the same answer, since only a surface can say it fell back");
    TSharedPtr<FStubApplicationRenderer> Renderer = MakeSharedPtr<FStubApplicationRenderer>();
    Application.GetApplication().SetRenderer(Renderer);

    TSharedPtr<FMenu>   NoSurfaceContent = FMenu::Create();
    TSharedPtr<FWindow> NoSurfacePopup   = Popups::Open(Parent, FRectangle(IntVector2(0, 0), 200, 100), NoSurfaceContent);

    TEST_EXPECT(NoSurfacePopup != nullptr);
    TEST_EXPECT_EQ(NoSurfaceContent->GetStyle().CornerRadius, ExpectedRadius);

    TEST_SECTION("It is asked for the surface up front all the same, so the answer is there before anything draws");
    TEST_EXPECT_EQ(Renderer->EnsuredCount, 1);
    TEST_EXPECT(Renderer->LastEnsuredWindow == NoSurfacePopup.Get());

    Popups::Close(NoSurfacePopup);
    Application.GetApplication().SetRenderer(nullptr);

    TEST_END();
}
