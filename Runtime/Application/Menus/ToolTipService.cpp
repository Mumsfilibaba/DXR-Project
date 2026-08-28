#include "Application/Menus/ToolTipService.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Math.h"

TUniquePtr<FToolTipService> FToolTipService::ToolTipService = nullptr;

TSharedPtr<FToolTip> FToolTip::Create(const String& InText, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FToolTip> NewToolTip = MakeSharedPtr<FToolTip>();
    NewToolTip->Text = InText;
    NewToolTip->Font = InFont;
    return NewToolTip;
}

FToolTip::FToolTip()
    : FCompoundElement()
    , Text()
    , Font(nullptr)
{
    SetPadding(FMargin(6, 3, 6, 3));
}

FToolTip::~FToolTip() = default;

IntVector2 FToolTip::ComputeDesiredSize() const
{
    const FMargin& Inset = GetPadding();

    IntVector2 DesiredSize(Inset.GetTotalHorizontal(), Inset.GetTotalVertical());
    if (Font)
    {
        DesiredSize.X += Font->MeasureWidth(StringView(Text.Data(), Text.Length()));
        DesiredSize.Y += Font->GetLineHeight();
    }

    return DesiredSize;
}

int32 FToolTip::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style  = FUIStyle::GetDefault();
    const FRectangle   Bounds = AllottedGeometry.Bounds;
    const FCornerRadii Radii(Style.Metrics.CornerRadius);

    OutCommandList.AddBox(LayerId, Bounds, Style.Colors.PanelBackground, Radii);
    OutCommandList.AddBoxOutline(LayerId, Bounds, Style.Colors.Border, 1.0f, Radii);

    if (Font && !Text.IsEmpty())
    {
        OutCommandList.AddText(LayerId + 1, Bounds.Deflate(GetPadding()), Text, Font.Get(), Style.Colors.Text);
    }

    return LayerId + 1;
}

void FToolTip::SetText(const String& InText)
{
    Text = InText;
}

TSharedPtr<FToolTipHost> FToolTipHost::Create(
    const TSharedPtr<FVisualElement>& InContent,
    const String&                     InText,
    const TSharedPtr<IFontFace>&      InFont,
    EToolTipPlacement                 InPlacement)
{
    TSharedPtr<FToolTipHost> NewHost = MakeSharedPtr<FToolTipHost>();
    NewHost->Text      = InText;
    NewHost->Font      = InFont;
    NewHost->Placement = InPlacement;
    NewHost->SetContent(InContent);

    return NewHost;
}

FToolTipHost::FToolTipHost()
    : FCompoundElement()
    , Text()
    , Font(nullptr)
    , Placement(EToolTipPlacement::FollowCursor)
{
}

FToolTipHost::~FToolTipHost() = default;

FEventResponse FToolTipHost::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    FToolTipService& ToolTips = FToolTipService::Get();
    ToolTips.NotifyCursorMoved(CursorEvent.GetScreenPosition());
    ToolTips.RequestTextToolTip(AsSharedPtr(), Text, Font, Placement);

    return FEventResponse::Unhandled();
}

FEventResponse FToolTipHost::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    FToolTipService::Get().CancelToolTip(AsSharedPtr());
    return FEventResponse::Unhandled();
}

void FToolTipHost::SetToolTipText(const String& InText)
{
    Text = InText;
}

FToolTipService& FToolTipService::Get()
{
    if (!ToolTipService)
    {
        ToolTipService = MakeUniquePtr<FToolTipService>();
    }

    return *ToolTipService;
}

void FToolTipService::Shutdown()
{
    if (ToolTipService)
    {
        ToolTipService->DismissToolTip();
        ToolTipService.Reset();
    }
}

FToolTipService::FToolTipService()
    : Owner(nullptr)
    , Content(nullptr)
    , ToolTipWindow(nullptr)
    , Placement(EToolTipPlacement::FollowCursor)
    , CursorPosition()
    , RequestedDelay(DefaultDelay)
    , RemainingSeconds(0.0f)
    , bIsShowing(false)
{
}

FToolTipService::~FToolTipService()
{
    DismissToolTip();
}

void FToolTipService::RequestToolTip(
    const TSharedPtr<FVisualElement>& InOwner,
    const TSharedPtr<FVisualElement>& InContent,
    EToolTipPlacement                 InPlacement,
    float                             DelaySeconds)
{
    if (!InOwner || !InContent)
    {
        return;
    }

    if (Owner != InOwner)
    {
        DismissToolTip();
    }

    Owner            = InOwner;
    Content          = InContent;
    Placement        = InPlacement;
    RequestedDelay   = Math::Max(DelaySeconds, 0.0f);
    RemainingSeconds = RequestedDelay;
}

void FToolTipService::RequestTextToolTip(
    const TSharedPtr<FVisualElement>& InOwner,
    const String&                     Text,
    const TSharedPtr<IFontFace>&      Font,
    EToolTipPlacement                 InPlacement,
    float                             DelaySeconds)
{
    RequestToolTip(InOwner, FToolTip::Create(Text, Font), InPlacement, DelaySeconds);
}

void FToolTipService::CancelToolTip(const TSharedPtr<FVisualElement>& InOwner)
{
    if (Owner == InOwner)
    {
        DismissToolTip();
    }
}

void FToolTipService::NotifyCursorMoved(const IntVector2& ScreenPosition)
{
    if (CursorPosition == ScreenPosition)
    {
        return;
    }

    CursorPosition = ScreenPosition;

    if (bIsShowing)
    {
        if (Placement == EToolTipPlacement::FollowCursor && ToolTipWindow)
        {
            const IntVector2 Size = ToolTipWindow->GetSize();
            ToolTipWindow->SetPosition(ResolveBounds(Size).Position);
        }

        return;
    }

    if (Owner)
    {
        RemainingSeconds = RequestedDelay;
    }
}

void FToolTipService::DismissToolTip()
{
    Popups::Close(ToolTipWindow);

    Owner            = nullptr;
    Content          = nullptr;
    ToolTipWindow    = nullptr;
    RemainingSeconds = 0.0f;
    bIsShowing       = false;
}

bool FToolTipService::IsShowing() const
{
    return bIsShowing;
}

bool FToolTipService::IsPending() const
{
    return Owner != nullptr && !bIsShowing;
}

void FToolTipService::Tick(float DeltaSeconds)
{
    if (!Owner || bIsShowing)
    {
        return;
    }

    RemainingSeconds -= DeltaSeconds;
    if (RemainingSeconds <= 0.0f)
    {
        ShowToolTip();
    }
}

void FToolTipService::ShowToolTip()
{
    if (!Content || !FApplication::IsInitialized())
    {
        return;
    }

    TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(Owner);
    if (!OwningWindow)
    {
        return;
    }

    Content->PrepareDesiredSize();

    const IntVector2 ToolTipSize = Content->GetCachedDesiredSize();
    if (ToolTipSize.X <= 0 || ToolTipSize.Y <= 0)
    {
        return;
    }

    ToolTipWindow = Popups::Open(OwningWindow, ResolveBounds(ToolTipSize), Content, false);
    bIsShowing    = ToolTipWindow != nullptr;
}

FRectangle FToolTipService::ResolveBounds(const IntVector2& ToolTipSize) const
{
    FRectangle Bounds(IntVector2(), ToolTipSize.X, ToolTipSize.Y);

    if (Placement == EToolTipPlacement::BelowAnchor)
    {
        const FRectangle AnchorBounds = FMenuStack::GetScreenBounds(Owner);
        Bounds.Position = IntVector2(AnchorBounds.Position.X, AnchorBounds.GetBottom() + 2);
    }
    else
    {
        Bounds.Position = CursorPosition + IntVector2(CursorOffset, CursorOffset);
    }

    return Popups::ClampToWorkArea(Bounds);
}
