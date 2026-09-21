#include "Application/Menus/ToolTipService.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/PopupWindow.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/MenuHost.h"
#include "Application/Style/UIStyle.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Math.h"

TUniquePtr<FToolTipService> FToolTipService::ToolTipService = nullptr;

static TArray<String> SplitTextIntoLines(const String& Text)
{
    TArray<String> Lines;
    if (Text.IsEmpty())
    {
        return Lines;
    }

    const CHAR* const End       = Text.Data() + Text.Length();
    const CHAR*       LineStart = Text.Data();

    for (const CHAR* Current = LineStart; Current != End; ++Current)
    {
        if (*Current == '\n')
        {
            Lines.Emplace(LineStart, static_cast<int32>(Current - LineStart));
            LineStart = Current + 1;
        }
    }

    Lines.Emplace(LineStart, static_cast<int32>(End - LineStart));
    return Lines;
}

TSharedPtr<FToolTip> FToolTip::Create(const String& InText, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FToolTip> NewToolTip = MakeSharedPtr<FToolTip>();
    NewToolTip->Font = InFont;
    NewToolTip->SetText(InText);
    return NewToolTip;
}

FToolTip::FToolTip()
    : FCompoundElement()
    , Text()
    , Lines()
    , Font(nullptr)
    , CornerRadius(FUIStyle::GetDefault().Metrics.CornerRadius)
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
        int32 WidestLine = 0;
        for (const String& Line : Lines)
        {
            WidestLine = Math::Max(WidestLine, Font->MeasureWidth(StringView(Line.Data(), Line.Length())));
        }

        DesiredSize.X += WidestLine;
        DesiredSize.Y += Font->GetLineHeight() * Math::Max(Lines.Size(), 1);
    }

    return DesiredSize;
}

int32 FToolTip::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style  = FUIStyle::GetDefault();
    const FRectangle   Bounds = AllottedGeometry.Bounds;
    const FCornerRadii Radii(CornerRadius);

    OutCommandList.AddBox(LayerId, Bounds, Style.Colors.PanelBackground, Radii);
    OutCommandList.AddBoxOutline(LayerId, Bounds, Style.Colors.Border, 1.0f, Radii);

    if (Font)
    {
        const FRectangle TextBounds = Bounds.Deflate(GetPadding());
        const int32      LineHeight = Font->GetLineHeight();

        for (int32 LineIndex = 0; LineIndex < Lines.Size(); ++LineIndex)
        {
            const FRectangle LineBounds(IntVector2(TextBounds.Position.X, TextBounds.Position.Y + (LineIndex * LineHeight)),
                TextBounds.Width, LineHeight);

            OutCommandList.AddText(LayerId + 1, LineBounds, Lines[LineIndex], Font.Get(), Style.Colors.Text);
        }
    }

    return LayerId + 1;
}

void FToolTip::SetOuterCornerRadius(float InCornerRadius)
{
    CornerRadius = InCornerRadius;
}

void FToolTip::SetText(const String& InText)
{
    Text  = InText;
    Lines = SplitTextIntoLines(Text);
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
    , HostWindow(nullptr)
    , ToolTipWindow(nullptr)
    , Placement(EToolTipPlacement::FollowCursor)
    , AnchorBounds()
    , ClampArea()
    , ToolTipBounds()
    , CursorPosition()
    , RequestedText()
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
    float                             DelaySeconds,
    const FRectangle&                 InAnchorBounds)
{
    if (!InOwner || !InContent)
    {
        return;
    }

    if (Owner != InOwner || bIsShowing)
    {
        DismissToolTip();
    }

    Owner            = InOwner;
    Content          = InContent;
    Placement        = InPlacement;
    AnchorBounds     = InAnchorBounds;
    RequestedDelay   = Math::Max(DelaySeconds, 0.0f);
    RemainingSeconds = RequestedDelay;

    RequestedText.Clear();
}

void FToolTipService::RequestTextToolTip(
    const TSharedPtr<FVisualElement>& InOwner,
    const String&                     Text,
    const TSharedPtr<IFontFace>&      Font,
    EToolTipPlacement                 InPlacement,
    float                             DelaySeconds)
{
    if (Owner == InOwner && RequestedText == Text && Placement == InPlacement)
    {
        return;
    }

    RequestToolTip(InOwner, FToolTip::Create(Text, Font), InPlacement, DelaySeconds);
    RequestedText = Text;
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
        if (Placement == EToolTipPlacement::FollowCursor)
        {
            MoveToolTip();
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
    if (ToolTipWindow)
    {
        Popups::Close(ToolTipWindow);
    }
    else if (HostWindow)
    {
        if (TSharedPtr<FMenuHost> Host = HostWindow->GetMenuHost())
        {
            Host->RemoveChild(Content);
        }
    }

    Owner            = nullptr;
    Content          = nullptr;
    HostWindow       = nullptr;
    ToolTipWindow    = nullptr;
    AnchorBounds     = FRectangle();
    ClampArea        = FRectangle();
    ToolTipBounds    = FRectangle();
    RequestedText.Clear();
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

    const IntVector2 OwningSize = OwningWindow->GetSize();
    const FRectangle OwningArea(OwningWindow->GetPosition(), OwningSize.X, OwningSize.Y);

    if (ToolTipSize.X <= OwningArea.Width && ToolTipSize.Y <= OwningArea.Height)
    {
        HostWindow    = OwningWindow;
        ClampArea     = OwningArea;
        ToolTipBounds = ResolveBounds(ToolTipSize);

        const FRectangle ClientBounds(ToolTipBounds.Position - OwningArea.Position, ToolTipBounds.Width, ToolTipBounds.Height);
        OwningWindow->GetOrCreateMenuHost()->AddChild(Content, ClientBounds, false);

        bIsShowing = true;
        return;
    }

    const IntVector2 ClampAnchor = (Placement == EToolTipPlacement::FollowCursor) ? CursorPosition : ResolveAnchorBounds().Position;

    ClampArea     = Popups::FindWorkArea(ClampAnchor);
    ToolTipBounds = ResolveBounds(ToolTipSize);
    ToolTipWindow = Popups::Open(OwningWindow, ToolTipBounds, Content, false);
    bIsShowing    = ToolTipWindow != nullptr;
}

void FToolTipService::MoveToolTip()
{
    if (!Content)
    {
        return;
    }

    const IntVector2 ToolTipSize(ToolTipBounds.Width, ToolTipBounds.Height);
    ToolTipBounds = ResolveBounds(ToolTipSize);

    if (ToolTipWindow)
    {
        ToolTipWindow->SetPosition(ToolTipBounds.Position);
    }
    else if (HostWindow)
    {
        if (TSharedPtr<FMenuHost> Host = HostWindow->GetMenuHost())
        {
            const FRectangle ClientBounds(ToolTipBounds.Position - HostWindow->GetPosition(), ToolTipBounds.Width, ToolTipBounds.Height);
            Host->SetChildBounds(Content, ClientBounds);
        }
    }
}

FRectangle FToolTipService::ResolveBounds(const IntVector2& ToolTipSize) const
{
    FRectangle Bounds(IntVector2(), ToolTipSize.X, ToolTipSize.Y);

    const int32 Gap = HasAnchorBoundsOverride() ? 0 : AnchorGap;

    switch (Placement)
    {
        case EToolTipPlacement::BelowAnchor:
        {
            const FRectangle Anchor = ResolveAnchorBounds();
            Bounds.Position = IntVector2(Anchor.Position.X, Anchor.GetBottom() + Gap);
            break;
        }

        case EToolTipPlacement::RightOfAnchor:
        {
            const FRectangle Anchor   = ResolveAnchorBounds();
            const int32      FlippedX = Anchor.Position.X - Gap - ToolTipSize.X;

            Bounds.Position = IntVector2(Anchor.GetRight() + Gap, Anchor.Position.Y);

            if (Bounds.GetRight() > ClampArea.GetRight() && FlippedX >= ClampArea.Position.X)
            {
                Bounds.Position.X = FlippedX;
            }

            break;
        }

        default:
        {
            Bounds.Position = CursorPosition + IntVector2(CursorOffset, CursorOffset);
            break;
        }
    }

    Bounds.Position.X = Math::Clamp(Bounds.Position.X, ClampArea.Position.X, Math::Max(ClampArea.Position.X, ClampArea.GetRight() - ToolTipSize.X));
    Bounds.Position.Y = Math::Clamp(Bounds.Position.Y, ClampArea.Position.Y, Math::Max(ClampArea.Position.Y, ClampArea.GetBottom() - ToolTipSize.Y));

    return Bounds;
}

FRectangle FToolTipService::ResolveAnchorBounds() const
{
    return HasAnchorBoundsOverride() ? AnchorBounds : FMenuStack::GetScreenBounds(Owner);
}

bool FToolTipService::HasAnchorBoundsOverride() const
{
    return AnchorBounds.Width > 0 && AnchorBounds.Height > 0;
}
