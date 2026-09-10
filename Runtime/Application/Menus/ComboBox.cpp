#include "Application/Menus/ComboBox.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 COMBO_ARROW_WIDTH      = 16;
constexpr float COMBO_ARROW_HALF_WIDTH = 6.0f;

TSharedPtr<FComboBoxButton> FComboBoxButton::Create(const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FComboBoxButton> NewButton = MakeSharedPtr<FComboBoxButton>();
    NewButton->Font = InFont;
    NewButton->SetPadding(FUIStyle::GetDefault().Metrics.ButtonPadding);
    return NewButton;
}

FComboBoxButton::FComboBoxButton()
    : FInteractiveElement()
    , Label()
    , Font(nullptr)
    , ReservedTextWidth(0)
    , Anchor(nullptr)
{
}

FComboBoxButton::~FComboBoxButton() = default;

IntVector2 FComboBoxButton::ComputeDesiredSize() const
{
    const FUIStyle& Style = FUIStyle::GetDefault();
    const FMargin&  Inset = GetPadding();

    IntVector2 DesiredSize(Inset.GetTotalHorizontal() + COMBO_ARROW_WIDTH, Inset.GetTotalVertical());
    if (Font)
    {
        DesiredSize.X += Math::Max(ReservedTextWidth, Font->MeasureWidth(StringView(Label.Data(), Label.Length())));
        DesiredSize.Y += Font->GetLineHeight();
    }

    DesiredSize.Y = Math::Max(DesiredSize.Y, Style.Metrics.ButtonHeight);
    return DesiredSize;
}

int32 FComboBoxButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style   = FUIStyle::GetDefault();
    const EInteractionState State   = GetInteractionState();
    const FRectangle        Bounds  = AllottedGeometry.Bounds;
    const FCornerRadii      Radii(Style.Metrics.CornerRadius);
    const bool              bIsOpen = Anchor && Anchor->IsOpen();
    const bool              bIsLit  = bIsOpen || State == EInteractionState::Hovered || State == EInteractionState::Pressed;

    OutCommandList.AddBox(LayerId, Bounds, Style.ComboBox.Fill, Radii);
    OutCommandList.AddBoxOutline(LayerId, Bounds, bIsLit ? Style.Colors.InputFieldBorderHovered : Style.Colors.InputFieldBorder, Style.Metrics.BorderThickness, Radii);

    const FRectangle   Inner     = Bounds.Deflate(GetPadding());
    const FFloatColor& TextColor = State == EInteractionState::Disabled ? Style.Colors.TextDisabled : (bIsLit ? Style.ComboBox.TextHovered : Style.ComboBox.Text);

    if (Font && !Label.IsEmpty())
    {
        FRectangle TextBounds = Inner;
        TextBounds.Width      = Math::Max(Inner.Width - COMBO_ARROW_WIDTH, 0);

        OutCommandList.AddText(LayerId, TextBounds, Font->ElideText(StringView(Label.Data(), Label.Length()), TextBounds.Width), Font.Get(), TextColor);
    }

    const float CenterX = static_cast<float>(Bounds.GetRight()) - (static_cast<float>(Bounds.Height) * 0.5f);
    const float CenterY = static_cast<float>(Bounds.Position.Y) + (static_cast<float>(Bounds.Height) * 0.5f);

    OutCommandList.AddTriangle(LayerId,
        Vector2(CenterX - COMBO_ARROW_HALF_WIDTH, CenterY - (COMBO_ARROW_HALF_WIDTH * 0.5f)),
        Vector2(CenterX + COMBO_ARROW_HALF_WIDTH, CenterY - (COMBO_ARROW_HALF_WIDTH * 0.5f)),
        Vector2(CenterX,                          CenterY + (COMBO_ARROW_HALF_WIDTH * 0.5f)),
        Style.ComboBox.Arrow);

    return LayerId;
}

void FComboBoxButton::SetAnchor(FMenuAnchor* InAnchor)
{
    Anchor = InAnchor;
}

void FComboBoxButton::SetText(const String& InText)
{
    Label = InText;
}

void FComboBoxButton::SetReservedTextWidth(int32 InTextWidth)
{
    ReservedTextWidth = Math::Max(InTextWidth, 0);
}

void FComboBoxButton::OnClicked()
{
    if (Anchor)
    {
        Anchor->Toggle();
    }
}

TSharedPtr<FComboBox> FComboBox::Create(const FDesc& Desc)
{
    TSharedPtr<FComboBox> NewComboBox = MakeSharedPtr<FComboBox>();
    NewComboBox->Initialize(Desc);
    return NewComboBox;
}

FComboBox::FComboBox()
    : FCompoundElement()
    , Options()
    , SelectedIndex(-1)
    , PlaceholderText()
    , Font(nullptr)
    , Button(nullptr)
    , Anchor(nullptr)
    , OnSelectionChangedDelegate()
{
}

FComboBox::~FComboBox() = default;

void FComboBox::Initialize(const FDesc& Desc)
{
    Options                    = Desc.Options;
    SelectedIndex              = Options.IsValidIndex(Desc.SelectedIndex) ? Desc.SelectedIndex : -1;
    PlaceholderText            = Desc.PlaceholderText;
    Font                       = Desc.Font;
    OnSelectionChangedDelegate = Desc.OnSelectionChanged;

    Button = FComboBoxButton::Create(Font);

    FMenuAnchor::FDesc AnchorDesc;
    AnchorDesc.Content   = Button;
    AnchorDesc.Placement = EMenuPlacement::BelowLeftAligned;

    AnchorDesc.OnGetMenuContent.BindLambda([this]() -> TSharedPtr<FVisualElement>
    {
        return BuildMenu();
    });

    Anchor = FMenuAnchor::Create(AnchorDesc);
    Button->SetAnchor(Anchor.Get());

    SetContent(Anchor);
    RefreshButtonText();
}

void FComboBox::SetSelectedIndex(int32 Index)
{
    const int32 NewIndex = Options.IsValidIndex(Index) ? Index : -1;
    if (NewIndex == SelectedIndex)
    {
        return;
    }

    SelectedIndex = NewIndex;
    RefreshButtonText();

    OnSelectionChangedDelegate.ExecuteIfBound(SelectedIndex);
}

const String& FComboBox::GetSelectedText() const
{
    return Options.IsValidIndex(SelectedIndex) ? Options[SelectedIndex] : PlaceholderText;
}

void FComboBox::SetOptions(const TArray<String>& InOptions)
{
    Options       = InOptions;
    SelectedIndex = -1;

    RefreshButtonText();
}

void FComboBox::OpenMenu()
{
    if (Anchor)
    {
        Anchor->Open();
    }
}

void FComboBox::CloseMenu()
{
    if (Anchor)
    {
        Anchor->Close();
    }
}

bool FComboBox::IsMenuOpen() const
{
    return Anchor && Anchor->IsOpen();
}

TSharedPtr<FVisualElement> FComboBox::BuildMenu()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FMenu> Menu = FMenu::Create();
    Menu->SetChrome(Style.ComboBox.PopupFill, Style.ComboBox.PopupBorder);

    for (int32 Index = 0; Index < Options.Size(); ++Index)
    {
        FMenuItem::FDesc ItemDesc;
        ItemDesc.Label        = Options[Index];
        ItemDesc.Font         = Font;
        ItemDesc.bIsCheckable = true;
        ItemDesc.CheckState   = (Index == SelectedIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

        ItemDesc.OnActivated.BindLambda([this, Index]()
        {
            SetSelectedIndex(Index);
        });

        TSharedPtr<FMenuItem> Item = FMenuItem::Create(ItemDesc);
        Item->SetHighlightFill(Style.ComboBox.SelectionFill);

        Menu->AddItem(Item);
    }

    Menu->SetMinDesiredWidth(GetContentRectangle().Width);
    Menu->SetHighlightedIndex(SelectedIndex);

    return Menu;
}

void FComboBox::RefreshButtonText()
{
    if (!Button)
    {
        return;
    }

    Button->SetText(GetSelectedText());

    if (Font)
    {
        int32 WidestOption = Font->MeasureWidth(StringView(PlaceholderText.Data(), PlaceholderText.Length()));
        for (const String& Option : Options)
        {
            WidestOption = Math::Max(WidestOption, Font->MeasureWidth(StringView(Option.Data(), Option.Length())));
        }

        Button->SetReservedTextWidth(WidestOption);
    }
}
