#include "Application/Menus/MenuItem.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FMenuItem> FMenuItem::Create(const FDesc& Desc)
{
    TSharedPtr<FMenuItem> NewItem = MakeSharedPtr<FMenuItem>();
    NewItem->Initialize(Desc);
    return NewItem;
}

FMenuItem::FMenuItem()
    : FInteractiveElement()
    , Label()
    , ShortcutText()
    , Icon()
    , Font(nullptr)
    , CheckState(ECheckBoxState::Unchecked)
    , bIsCheckable(false)
    , bIsHighlighted(false)
    , SubMenu(nullptr)
    , OnActivatedDelegate()
{
}

FMenuItem::~FMenuItem() = default;

void FMenuItem::Initialize(const FDesc& Desc)
{
    Label               = Desc.Label;
    ShortcutText        = Desc.ShortcutText;
    Icon                = Desc.Icon;
    Font                = Desc.Font;
    CheckState          = Desc.CheckState;
    bIsCheckable        = Desc.bIsCheckable;
    SubMenu             = Desc.SubMenu;
    OnActivatedDelegate = Desc.OnActivated;

    SetPadding(FMargin(6, 3, 6, 3));
}

IntVector2 FMenuItem::ComputeDesiredSize() const
{
    const FUIStyle& Style = FUIStyle::GetDefault();
    const FMargin&  Inset = GetPadding();

    int32 Width  = Inset.GetTotalHorizontal() + GutterWidth;
    int32 Height = Inset.GetTotalVertical();

    if (Font)
    {
        Width += Font->MeasureWidth(StringView(Label.Data(), Label.Length()));
        Height += Font->GetLineHeight();

        if (!ShortcutText.IsEmpty())
        {
            Width += ShortcutGap + Font->MeasureWidth(StringView(ShortcutText.Data(), ShortcutText.Length()));
        }
    }

    if (SubMenu)
    {
        Width += ShortcutGap + ArrowWidth;
    }

    return IntVector2(Width, Math::Max(Height, Style.Metrics.RowHeight));
}

int32 FMenuItem::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style  = FUIStyle::GetDefault();
    const EInteractionState State  = GetInteractionState();
    const FRectangle        Bounds = AllottedGeometry.Bounds;

    if (bIsHighlighted || State == EInteractionState::Hovered || State == EInteractionState::Pressed)
    {
        OutCommandList.AddBox(LayerId, Bounds, Style.Colors.Accent, FCornerRadii(Style.Metrics.CornerRadius));
    }

    const FFloatColor TextColor = Style.GetTextColor(State);
    const FRectangle  Inner     = Bounds.Deflate(GetPadding());
    const int32       MidY      = Inner.Position.Y + (Inner.Height / 2);

    if (bIsCheckable && CheckState != ECheckBoxState::Unchecked)
    {
        const float Left = static_cast<float>(Inner.Position.X) + 3.0f;
        const float Mid  = static_cast<float>(MidY);

        if (CheckState == ECheckBoxState::Checked)
        {
            const Vector2 Tick[] =
            {
                Vector2(Left,        Mid + 1.0f),
                Vector2(Left + 4.0f, Mid + 5.0f),
                Vector2(Left + 11.0f, Mid - 5.0f),
            };

            OutCommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Tick, ARRAY_COUNT(Tick)), TextColor, 2.0f);
        }
        else
        {
            FRectangle Dash(IntVector2(Inner.Position.X + 4, MidY - 1), 10, 2);
            OutCommandList.AddBox(LayerId, Dash, TextColor);
        }
    }
    else if (Icon.IsValid())
    {
        FRectangle IconBounds(IntVector2(Inner.Position.X + 2, MidY - 8), 16, 16);
        OutCommandList.AddImage(LayerId, IconBounds, Icon, FFloatColor::White);
    }

    if (Font && !Label.IsEmpty())
    {
        FRectangle LabelBounds = Inner;
        LabelBounds.Position.X += GutterWidth;
        LabelBounds.Width = Math::Max(Inner.Width - GutterWidth, 0);

        OutCommandList.AddText(LayerId, LabelBounds, Label, Font.Get(), TextColor);
    }

    if (Font && !ShortcutText.IsEmpty())
    {
        const int32 ShortcutWidth = Font->MeasureWidth(StringView(ShortcutText.Data(), ShortcutText.Length()));

        FRectangle ShortcutBounds = Inner;
        ShortcutBounds.Position.X = Inner.GetRight() - ShortcutWidth - (SubMenu ? ArrowWidth : 0);
        ShortcutBounds.Width      = ShortcutWidth;

        OutCommandList.AddText(LayerId, ShortcutBounds, ShortcutText, Font.Get(), Style.Colors.TextDisabled);
    }

    if (SubMenu)
    {
        const float Right = static_cast<float>(Inner.GetRight()) - 4.0f;
        const float Mid   = static_cast<float>(MidY);

        const Vector2 Arrow[] =
        {
            Vector2(Right - 5.0f, Mid - 4.0f),
            Vector2(Right,        Mid),
            Vector2(Right - 5.0f, Mid + 4.0f),
        };

        OutCommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Arrow, ARRAY_COUNT(Arrow)), TextColor, 1.5f);
    }

    return LayerId;
}

FEventResponse FMenuItem::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseEntered(CursorEvent);
    if (!IsEnabled())
    {
        return Response;
    }

    FMenuStack& Stack = FMenuStack::Get();

    if (FApplication::IsInitialized())
    {
        const int32 OwnDepth = Stack.GetMenuDepth(FApplication::Get().FindWindow(AsSharedPtr()));
        if (OwnDepth > 0)
        {
            Stack.DismissToDepth(OwnDepth);
        }
    }

    if (SubMenu)
    {
        Stack.ScheduleSubMenu(AsSharedPtr(), FMenuStack::GetScreenBounds(AsSharedPtr()), SubMenu);
    }

    return Response;
}

FEventResponse FMenuItem::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    FMenuStack::Get().CancelScheduledSubMenu(AsSharedPtr());
    return FInteractiveElement::OnMouseLeft(CursorEvent);
}

void FMenuItem::SetCheckState(ECheckBoxState InCheckState)
{
    CheckState = InCheckState;
}

void FMenuItem::SetHighlighted(bool bInIsHighlighted)
{
    bIsHighlighted = bInIsHighlighted;
}

void FMenuItem::Activate()
{
    if (!IsEnabled())
    {
        return;
    }

    if (SubMenu)
    {
        OpenSubMenu();
        return;
    }

    FMenuStack::Get().DismissAll();
    OnActivatedDelegate.ExecuteIfBound();
}

void FMenuItem::SetOnActivated(const FOnMenuItemActivated& InOnActivated)
{
    OnActivatedDelegate = InOnActivated;
}

void FMenuItem::OpenSubMenu()
{
    if (!SubMenu || !FApplication::IsInitialized())
    {
        return;
    }

    if (TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(AsSharedPtr()))
    {
        FMenuStack::Get().PushMenu(OwningWindow, FMenuStack::GetScreenBounds(AsSharedPtr()), EMenuPlacement::RightOfTopAligned, SubMenu);
    }
}

void FMenuItem::OnClicked()
{
    Activate();
}

TSharedPtr<FMenuSeparator> FMenuSeparator::Create()
{
    return MakeSharedPtr<FMenuSeparator>();
}

FMenuSeparator::FMenuSeparator()
    : FVisualElement()
{
}

FMenuSeparator::~FMenuSeparator() = default;

IntVector2 FMenuSeparator::ComputeDesiredSize() const
{
    return IntVector2(0, FUIStyle::GetDefault().Metrics.SeparatorThickness + 6);
}

int32 FMenuSeparator::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FRectangle Line = AllottedGeometry.Bounds.Deflate(FMargin(6, 0, 6, 0));
    Line.Height     = Style.Metrics.SeparatorThickness;
    Line.Position.Y = AllottedGeometry.Bounds.Position.Y + ((AllottedGeometry.Bounds.Height - Line.Height) / 2);

    if (!Line.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, Line, Style.Colors.Border);
    }

    return LayerId;
}
