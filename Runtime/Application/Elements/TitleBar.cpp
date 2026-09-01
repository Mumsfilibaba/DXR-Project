#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/TitleBar.h"
#include "Application/Elements/Window.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

// What a caption button asks for when the platform has no opinion, which is the Windows 11 shape
constexpr int32 CAPTION_BUTTON_WIDTH  = 46;
constexpr int32 CAPTION_BUTTON_HEIGHT = 32;

// Half the side of the glyph drawn inside a caption button
constexpr float CAPTION_GLYPH_EXTENT = 5.0f;

constexpr float CAPTION_GLYPH_THICKNESS = 1.0f;

// The gap between the window icon and the title, and the side the icon is drawn at
constexpr int32 TITLE_ICON_SIZE   = 16;
constexpr int32 TITLE_ICON_MARGIN = 8;

// Close is the one button that has to read as destructive before it is pressed
static const FFloatColor GCloseHoveredColor(0.77f, 0.16f, 0.16f, 1.0f);
static const FFloatColor GClosePressedColor(0.60f, 0.12f, 0.12f, 1.0f);

static FWindowRect ToWindowRect(const FRectangle& Rectangle)
{
    FWindowRect Result;
    Result.Left   = Rectangle.Position.X;
    Result.Top    = Rectangle.Position.Y;
    Result.Right  = Rectangle.GetRight();
    Result.Bottom = Rectangle.GetBottom();
    return Result;
}

TSharedPtr<FCaptionButton> FCaptionButton::Create(ECaptionButtonKind InKind)
{
    TSharedPtr<FCaptionButton> NewButton = MakeSharedPtr<FCaptionButton>();
    NewButton->Kind = InKind;
    return NewButton;
}

FCaptionButton::FCaptionButton()
    : FInteractiveElement()
    , Kind(ECaptionButtonKind::Close)
    , ButtonSize(CAPTION_BUTTON_WIDTH, CAPTION_BUTTON_HEIGHT)
{
}

FCaptionButton::~FCaptionButton() = default;

IntVector2 FCaptionButton::ComputeDesiredSize() const
{
    return ButtonSize;
}

int32 FCaptionButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style  = FUIStyle::GetDefault();
    const EInteractionState State  = GetInteractionState();
    const FRectangle&       Bounds = AllottedGeometry.Bounds;

    if (Bounds.IsEmpty())
    {
        return LayerId;
    }

    if (State != EInteractionState::Normal)
    {
        FFloatColor Fill = Style.GetControlColor(State);
        if (Kind == ECaptionButtonKind::Close)
        {
            Fill = State == EInteractionState::Pressed ? GClosePressedColor : GCloseHoveredColor;
        }

        OutCommandList.AddBox(LayerId, Bounds, Fill);
    }

    const FFloatColor GlyphColor = Style.GetTextColor(State);

    const float CenterX = static_cast<float>(Bounds.Position.X) + static_cast<float>(Bounds.Width) * 0.5f;
    const float CenterY = static_cast<float>(Bounds.Position.Y) + static_cast<float>(Bounds.Height) * 0.5f;
    const float Extent  = CAPTION_GLYPH_EXTENT;

    switch (Kind)
    {
        case ECaptionButtonKind::Minimize:
        {
            OutCommandList.AddLine(LayerId + 1, Vector2(CenterX - Extent, CenterY), Vector2(CenterX + Extent, CenterY), GlyphColor, CAPTION_GLYPH_THICKNESS);
            break;
        }

        case ECaptionButtonKind::Maximize:
        {
            const Vector2 Corners[] =
            {
                Vector2(CenterX - Extent, CenterY - Extent),
                Vector2(CenterX + Extent, CenterY - Extent),
                Vector2(CenterX + Extent, CenterY + Extent),
                Vector2(CenterX - Extent, CenterY + Extent),
            };

            OutCommandList.AddPolyline(LayerId + 1, MakeArrayView(Corners, 4), GlyphColor, CAPTION_GLYPH_THICKNESS, true);
            break;
        }

        case ECaptionButtonKind::Close:
        {
            OutCommandList.AddLine(LayerId + 1, Vector2(CenterX - Extent, CenterY - Extent), Vector2(CenterX + Extent, CenterY + Extent), GlyphColor, CAPTION_GLYPH_THICKNESS);
            OutCommandList.AddLine(LayerId + 1, Vector2(CenterX + Extent, CenterY - Extent), Vector2(CenterX - Extent, CenterY + Extent), GlyphColor, CAPTION_GLYPH_THICKNESS);
            break;
        }
    }

    return LayerId + 2;
}

void FCaptionButton::SetButtonSize(const IntVector2& InSize)
{
    ButtonSize = InSize;
}

void FCaptionButton::OnClicked()
{
    TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(AsSharedPtr());
    if (!Window)
    {
        return;
    }

    switch (Kind)
    {
        case ECaptionButtonKind::Minimize:
        {
            Window->Minimize();
            break;
        }

        case ECaptionButtonKind::Maximize:
        {
            if (Window->IsMaximized())
            {
                Window->Restore();
            }
            else
            {
                Window->Maximize();
            }

            break;
        }

        case ECaptionButtonKind::Close:
        {
            FApplication::Get().DestroyWindow(Window);
            break;
        }
    }
}

TSharedPtr<FTitleBar> FTitleBar::Create(const FDesc& Desc)
{
    TSharedPtr<FTitleBar> NewTitleBar = MakeSharedPtr<FTitleBar>();
    NewTitleBar->Initialize(Desc);
    return NewTitleBar;
}

FTitleBar::FTitleBar()
    : FCompoundElement()
    , Metrics()
    , Regions()
    , Title()
    , Icon()
    , bShowCaptionButtons(false)
    , OwningWindow()
    , Panel(nullptr)
    , LeadingSpacer(nullptr)
    , IconSpacer(nullptr)
    , TrailingSpacer(nullptr)
    , TitleLabel(nullptr)
    , CaptionButtonRow(nullptr)
    , CaptionButtons()
{
}

FTitleBar::~FTitleBar() = default;

void FTitleBar::Initialize(const FDesc& Desc)
{
    Title               = Desc.Title;
    Icon                = Desc.Icon;
    bShowCaptionButtons = Desc.bShowCaptionButtons;

    Panel = MakeSharedPtr<FHorizontalBox>();

    LeadingSpacer = FSpacer::CreateHorizontal(0);
    Panel->AddSlot(LeadingSpacer);

    IconSpacer = FSpacer::CreateHorizontal(Icon.IsValid() ? TITLE_ICON_SIZE + TITLE_ICON_MARGIN : 0);
    Panel->AddSlot(IconSpacer);

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = Title;
    LabelDesc.Font            = Desc.Font;
    LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    TitleLabel = FTextBlock::Create(LabelDesc);

    Panel->AddSlot(Desc.Content);
    Panel->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);

    if (bShowCaptionButtons)
    {
        CaptionButtonRow = MakeSharedPtr<FHorizontalBox>();

        const ECaptionButtonKind Kinds[] = { ECaptionButtonKind::Minimize, ECaptionButtonKind::Maximize, ECaptionButtonKind::Close };
        for (ECaptionButtonKind Kind : Kinds)
        {
            TSharedPtr<FCaptionButton> Button = FCaptionButton::Create(Kind);
            CaptionButtonRow->AddSlot(Button);
            CaptionButtons.Add(Button);
        }

        Panel->AddSlot(CaptionButtonRow).SetVerticalAlignment(EVerticalAlignment::Top);
    }

    TrailingSpacer = FSpacer::CreateHorizontal(0);
    Panel->AddSlot(TrailingSpacer);

    TSharedPtr<FOverlay> Layers = FOverlay::Create();
    Layers->AddSlot(TitleLabel).SetHorizontalAlignment(EHorizontalAlignment::Center).SetVerticalAlignment(EVerticalAlignment::Center);
    Layers->AddSlot(Panel);

    SetContent(Layers);
}

IntVector2 FTitleBar::PrepareDesiredSize()
{
    RefreshMetrics();
    return FCompoundElement::PrepareDesiredSize();
}

IntVector2 FTitleBar::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.Y = Math::Max(DesiredSize.Y, Math::CeilToInt(Metrics.Height));
    DesiredSize.Y = Math::Max(DesiredSize.Y, FUIStyle::GetDefault().Metrics.RowHeight);
    return DesiredSize;
}

void FTitleBar::OnArrange(const FRectangle& AllottedBounds)
{
    FCompoundElement::OnArrange(AllottedBounds);
    PublishRegions();
}

int32 FTitleBar::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.PanelBackground);

    if (Icon.IsValid() && IconSpacer)
    {
        const FRectangle& SpacerBounds = IconSpacer->GetContentRectangle();
        const IntVector2  IconPosition = IntVector2(SpacerBounds.Position.X, SpacerBounds.Position.Y + (SpacerBounds.Height - TITLE_ICON_SIZE) / 2);
        const FRectangle  IconBounds   = FRectangle(IconPosition, TITLE_ICON_SIZE, TITLE_ICON_SIZE);

        OutCommandList.AddImage(LayerId + 1, IconBounds, Icon, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f));
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 2);
}

void FTitleBar::SetTitle(const String& InTitle)
{
    Title = InTitle;

    if (TitleLabel)
    {
        TitleLabel->SetText(InTitle);
    }
}

void FTitleBar::RefreshMetrics()
{
    TSharedPtr<FWindow> Window = OwningWindow.IsValid() ? TSharedPtr<FWindow>(OwningWindow) : nullptr;
    if (!Window)
    {
        Window       = FApplication::Get().FindWindow(AsSharedPtr());
        OwningWindow = Window;
    }

    Metrics = Window ? Window->GetTitleBarMetrics() : FWindowTitleBarMetrics();

    if (LeadingSpacer)
    {
        LeadingSpacer->SetSize(IntVector2(Math::CeilToInt(Metrics.LeadingInset), 0));
    }

    if (TrailingSpacer)
    {
        TrailingSpacer->SetSize(IntVector2(Math::CeilToInt(Metrics.TrailingInset), 0));
    }

    if (bShowCaptionButtons)
    {
        const int32 ButtonWidth  = Math::CeilToInt(Metrics.CaptionButtonWidth);
        const int32 ButtonHeight = ButtonWidth > 0 ? Math::Max(1, Math::CeilToInt(Metrics.Height)) : 0;

        for (const TSharedPtr<FCaptionButton>& Button : CaptionButtons)
        {
            Button->SetButtonSize(IntVector2(ButtonWidth, ButtonHeight));
        }
    }
}

void FTitleBar::PublishRegions()
{
    Regions = FWindowTitleBarRegions();
    Regions.CaptionRect = ToWindowRect(GetContentRectangle());

    GatherInteractiveRects(GetContent(), Regions.InteractiveRects);

    if (bShowCaptionButtons)
    {
        for (const TSharedPtr<FCaptionButton>& Button : CaptionButtons)
        {
            if (Button->GetKind() == ECaptionButtonKind::Maximize)
            {
                Regions.MaximizeButtonRect = ToWindowRect(Button->GetContentRectangle());
                break;
            }
        }
    }

    if (TSharedPtr<FWindow> Window = OwningWindow.IsValid() ? TSharedPtr<FWindow>(OwningWindow) : nullptr)
    {
        Window->SetTitleBarRegions(Regions);
    }
}

void FTitleBar::GatherInteractiveRects(const TSharedPtr<FVisualElement>& Element, TArray<FWindowRect>& OutRects) const
{
    if (!Element || !Element->IsVisible())
    {
        return;
    }

    if (Element->IsInteractive())
    {
        const FRectangle& Bounds = Element->GetContentRectangle();
        if (!Bounds.IsEmpty())
        {
            OutRects.Add(ToWindowRect(Bounds));
        }

        return;
    }

    TArray<TSharedPtr<FVisualElement>> Children;
    Element->GetChildren(Children);

    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        GatherInteractiveRects(Child, OutRects);
    }
}
