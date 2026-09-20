#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Application/Docking/DockNode.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Text/TrueTypeFontFace.h"

static FEditorFonts GFonts;
static FUIStyle     GStyle;

constexpr int32 BODY_PIXEL_HEIGHT      = 20;
constexpr int32 TITLE_PIXEL_HEIGHT     = 20;
constexpr int32 MONOSPACE_PIXEL_HEIGHT = 14;

constexpr float INPUT_FIELD_BORDER_THICKNESS = 1.0f;
constexpr float SEARCH_FIELD_CORNER_RADIUS   = 6.0f;
constexpr float CONSOLE_FIELD_CORNER_RADIUS  = 6.0f;

constexpr int32 INPUT_FIELD_PADDING_X = 12;
constexpr int32 INPUT_FIELD_PADDING_Y = 6;

constexpr int32 TOOL_TIP_PADDING = 4;

constexpr int32 SECTION_HEADER_HEIGHT   = 34;
constexpr int32 SECTION_CONTENT_INDENT  = 12;
constexpr int32 SECTION_OUTER_SPACING   = 4;
constexpr int32 SECTION_STACK_SPACING   = 3;

constexpr int32 HEADER_CARD_PADDING      = 16;
constexpr int32 INFO_LABEL_COLUMN_WIDTH  = 128;

static FFloatColor FromBytes(int32 R, int32 G, int32 B, int32 A = 255)
{
    return FFloatColor(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
}

static TSharedPtr<IFontFace> LoadFace(const CHAR* Filename, int32 PixelHeight)
{
    const String Path = Paths::GetAssetDir() + "/Editor/Fonts/" + Filename;

    TSharedPtr<FTrueTypeFontFace> Face = FTrueTypeFontFace::CreateFromFile(Path, PixelHeight);
    if (!Face)
    {
        LOG_ERROR("[FEditorStyle]: Failed to load '%s'", *Path);
        return nullptr;
    }

    return Face;
}

bool FEditorStyle::Initialize()
{
    GFonts.Body      = LoadFace("segoeui.ttf", BODY_PIXEL_HEIGHT);
    GFonts.BodyBold  = LoadFace("segoeuib.ttf", BODY_PIXEL_HEIGHT);
    GFonts.Title     = LoadFace("segoeui.ttf", TITLE_PIXEL_HEIGHT);
    GFonts.Monospace = LoadFace("consola.ttf", MONOSPACE_PIXEL_HEIGHT);

    if (!GFonts.Body || !GFonts.BodyBold || !GFonts.Title || !GFonts.Monospace)
    {
        Release();
        return false;
    }

    GStyle.Colors.WindowBackground        = FromBytes(24, 24, 24);
    GStyle.Colors.PanelBackground         = FromBytes(24, 24, 24);
    GStyle.Colors.ControlNormal           = FromBytes(33, 33, 33);
    GStyle.Colors.ControlHovered          = FromBytes(46, 46, 46);
    GStyle.Colors.ControlPressed          = FromBytes(51, 51, 51);
    GStyle.Colors.ControlDisabled         = FromBytes(41, 41, 41);
    GStyle.Colors.ButtonNormal            = FromBytes(56, 56, 56);
    GStyle.Colors.ButtonHovered           = FromBytes(87, 87, 87);
    GStyle.Colors.ButtonPressed           = FromBytes(87, 87, 87);
    GStyle.Colors.Border                  = FromBytes(21, 21, 21);
    GStyle.Colors.SeparatorHovered        = FromBytes(56, 56, 56);
    GStyle.Colors.Text                    = FromBytes(230, 230, 230);
    GStyle.Colors.TextDisabled            = FromBytes(115, 115, 115);
    GStyle.Colors.TextSelectionBackground = FromBytes(0, 112, 224);
    GStyle.Colors.SearchTextHighlight     = FromBytes(214, 154, 26, 140);
    GStyle.Colors.Accent                  = FromBytes(9, 92, 176);
    GStyle.Colors.AccentHovered           = FromBytes(15, 110, 205);

    GStyle.Metrics.ControlPadding         = FMargin(10, 6, 10, 6);
    GStyle.Metrics.ButtonPadding          = FMargin(12, 4, 12, 4);
    GStyle.Metrics.CornerRadius           = 4.0f;
    GStyle.Metrics.ButtonCornerRadius     = 6.0f;
    GStyle.Metrics.BorderThickness        = 1.0f;
    GStyle.Metrics.RowHeight              = RowHeight;
    GStyle.Metrics.FrameHeight            = FrameHeight;
    GStyle.Metrics.ButtonHeight           = ButtonHeight;
    GStyle.Metrics.ScrollBarThickness     = 12;
    GStyle.Metrics.SeparatorThickness     = 1;
    GStyle.Metrics.MenuSeparatorThickness = 1;

    GStyle.Panel.Fill                     = FromBytes(30, 30, 30);
    GStyle.Panel.Border                   = FromBytes(48, 48, 48);
    GStyle.Panel.BorderFocused            = FromBytes(9, 92, 176);
    GStyle.Panel.CornerRadius             = 8.0f;
    GStyle.Panel.BorderThickness          = 1.0f;
    GStyle.Panel.Gap                      = 6;

    GStyle.InnerFrame.Fill                = FromBytes(24, 24, 24);
    GStyle.InnerFrame.Border              = FromBytes(48, 48, 48);
    GStyle.InnerFrame.CornerRadius        = 6.0f;
    GStyle.InnerFrame.BorderThickness     = 1.0f;
    GStyle.InnerFrame.Padding             = FMargin(6);

    GStyle.Header.Fill                    = FromBytes(30, 30, 30);
    GStyle.Header.Border                  = FromBytes(48, 48, 48);
    GStyle.Header.BottomBorder            = FromBytes(48, 48, 48);
    GStyle.Header.CornerRadius            = 8.0f;
    GStyle.Header.BorderThickness         = 1.0f;
    GStyle.Header.ExpandDuration          = 0.15f;
    
    GStyle.ScrollBar.Track                = GStyle.Colors.WindowBackground;

    GStyle.Tab.StripFill                  = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    GStyle.Tab.FillHovered                = FromBytes(37, 37, 37);
    GStyle.Tab.FillActive                 = FromBytes(52, 52, 52);
    GStyle.Tab.ActiveStrip                = GStyle.Colors.Accent;
    GStyle.Tab.ActiveStripThickness       = 0;
    GStyle.Tab.Spacing                    = 4;
    GStyle.Tab.TopInset                   = 4;
    GStyle.Tab.BottomInset                = 4;
    GStyle.Tab.CornerRadius               = 8.0f;
    GStyle.Tab.StripHeight                = FDockMetrics::TabStripHeight;
    GStyle.Tab.LabelOffsetY               = 0;

    GStyle.MenuBar.ItemHovered            = FromBytes(87, 87, 87);
    GStyle.MenuBar.ItemActive             = FromBytes(104, 104, 104);

    GStyle.Menu.Background                = GStyle.Colors.WindowBackground;
    GStyle.Menu.Border                    = GStyle.Panel.Border;
    GStyle.Menu.ItemHovered               = FromBytes(0, 112, 224);
    GStyle.Menu.ItemShortcut              = FromBytes(175, 175, 175);
    GStyle.Menu.Separator                 = GStyle.Panel.Border;
    GStyle.Menu.SectionText               = FromBytes(160, 160, 160);

    GStyle.NormalFont    = GFonts.Body.Get();
    GStyle.MonospaceFont = GFonts.Monospace.Get();

    FUIStyle::SetDefault(GStyle);
    return true;
}

void FEditorStyle::Release()
{
    FUIStyle::ResetDefault();

    GFonts.Body.Reset();
    GFonts.BodyBold.Reset();
    GFonts.Title.Reset();
    GFonts.Monospace.Reset();

    GStyle = FUIStyle();
}

const FEditorFonts& FEditorStyle::GetFonts()
{
    return GFonts;
}

const FUIStyle& FEditorStyle::GetStyle()
{
    return FUIStyle::GetDefault();
}

FFloatColor FEditorStyle::GetFooterColor()
{
    return FUIStyle::GetDefault().Colors.WindowBackground;
}

FFloatColor FEditorStyle::GetCandidateListColor()
{
    return FromBytes(26, 26, 26);
}

FFloatColor FEditorStyle::GetCandidateListBorderColor()
{
    return FromBytes(56, 56, 56);
}

FFloatColor FEditorStyle::GetCandidateTextColor()
{
    return FromBytes(192, 192, 192);
}

FFloatColor FEditorStyle::GetCandidateSelectionColor()
{
    return FUIStyle::GetDefault().Colors.TextSelectionBackground;
}

FFloatColor FEditorStyle::GetCandidateHighlightColor()
{
    return FromBytes(139, 194, 74);
}

FFloatColor FEditorStyle::GetToolTipColor()
{
    return FUIStyle::GetDefault().Menu.Background;
}

FFloatColor FEditorStyle::GetToolTipBorderColor()
{
    return FUIStyle::GetDefault().Menu.Border;
}

TSharedPtr<FVisualElement> FEditorStyle::MakeToolTipFrame(const TSharedPtr<FVisualElement>& Content)
{
    const FUIStyleMetrics& Metrics = FUIStyle::GetDefault().Metrics;

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = GetToolTipColor();
    FrameDesc.BorderColor     = GetToolTipBorderColor();
    FrameDesc.BorderThickness = Metrics.BorderThickness;
    FrameDesc.CornerRadius    = FCornerRadii(Metrics.CornerRadius);
    FrameDesc.Padding         = FMargin(TOOL_TIP_PADDING);
    FrameDesc.Content         = Content;

    return FBorder::Create(FrameDesc);
}

TSharedPtr<FVisualElement> FEditorStyle::MakeInnerFrame(const TSharedPtr<FVisualElement>& Content)
{
    return MakeInnerFrame(Content, FUIStyle::GetDefault().InnerFrame.Padding);
}

TSharedPtr<FVisualElement> FEditorStyle::MakeInnerFrame(const TSharedPtr<FVisualElement>& Content, const FMargin& Padding)
{
    const FUIInnerFrameStyle& Frame = FUIStyle::GetDefault().InnerFrame;

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor        = Frame.Fill;
    FrameDesc.BorderColor            = Frame.Border;
    FrameDesc.BorderThickness        = Frame.BorderThickness;
    FrameDesc.CornerRadius           = FCornerRadii(Frame.CornerRadius);
    FrameDesc.Padding                = Padding;
    FrameDesc.Content                = Content;
    FrameDesc.bDrawBorderOverContent = true;

    return FBorder::Create(FrameDesc);
}

TSharedPtr<FVisualElement> FEditorStyle::MakeHeaderCard(const String& Title, const String& Subtitle, TSharedPtr<FTextBlock>* OutSubtitleText)
{
    FTextBlock::FDesc TitleDesc;
    TitleDesc.Text     = Title;
    TitleDesc.Font     = GFonts.BodyBold;
    TitleDesc.Overflow = ETextOverflow::Elide;

    FTextBlock::FDesc SubtitleDesc;
    SubtitleDesc.Text            = Subtitle;
    SubtitleDesc.Font            = GFonts.Body;
    SubtitleDesc.ColorAndOpacity = GStyle.Colors.TextDisabled;
    SubtitleDesc.Overflow        = ETextOverflow::Elide;

    TSharedPtr<FTextBlock> SubtitleText = FTextBlock::Create(SubtitleDesc);

    TSharedPtr<FVerticalBox> Text = FVerticalBox::Create();
    Text->AddSlot(FTextBlock::Create(TitleDesc));
    Text->AddSlot(SubtitleText).SetPadding(FMargin(0, ItemSpacing, 0, 0));

    if (OutSubtitleText)
    {
        *OutSubtitleText = SubtitleText;
    }

    const FUIHeaderStyle& HeaderStyle = GStyle.Header;

    FBorder::FDesc CardDesc;
    CardDesc.BackgroundColor = HeaderStyle.Fill;
    CardDesc.BorderColor     = HeaderStyle.Border;
    CardDesc.BorderThickness = HeaderStyle.BorderThickness;
    CardDesc.CornerRadius    = FCornerRadii(HeaderStyle.CornerRadius);
    CardDesc.Padding         = FMargin(HEADER_CARD_PADDING);
    CardDesc.Content         = Text;

    return FBorder::Create(CardDesc);
}

FInputFrameStyle FEditorStyle::GetInputFrameStyle()
{
    FInputFrameStyle FrameStyle;
    FrameStyle.BorderFocused   = FromBytes(9, 92, 176);
    FrameStyle.Text            = FFloatColor::White;
    FrameStyle.HintNormal      = FromBytes(76, 76, 76);
    FrameStyle.HintFocused     = FromBytes(97, 97, 97);
    FrameStyle.Selection       = FromBytes(0, 112, 224);
    FrameStyle.IconNormal      = FromBytes(192, 192, 192);
    FrameStyle.IconFocused     = FFloatColor::White;
    FrameStyle.BorderThickness = INPUT_FIELD_BORDER_THICKNESS;
    FrameStyle.CornerRadius    = SEARCH_FIELD_CORNER_RADIUS;
    return FrameStyle;
}

FInputFrameStyle FEditorStyle::GetConsoleInputFrameStyle()
{
    FInputFrameStyle FrameStyle = GetInputFrameStyle();
    FrameStyle.HintNormal       = FromBytes(77, 77, 77);
    FrameStyle.HintFocused      = FromBytes(99, 99, 99);
    FrameStyle.CornerRadius     = CONSOLE_FIELD_CORNER_RADIUS;
    return FrameStyle;
}

FSearchBox::FDesc FEditorStyle::MakeSearchBoxDesc(const String& Hint, const FOnSearchTextChanged& OnChanged)
{
    FSearchBox::FDesc Desc;
    Desc.HintText      = Hint;
    Desc.Font          = GFonts.Body;
    Desc.SearchIcon    = FEditorIcons::Search;
    Desc.ClearIcon     = FEditorIcons::Close;
    Desc.IconSize      = IconSize;
    Desc.Padding       = FMargin(INPUT_FIELD_PADDING_X, INPUT_FIELD_PADDING_Y);
    Desc.Style         = GetInputFrameStyle();
    Desc.OnTextChanged = OnChanged;
    return Desc;
}

FMargin FEditorStyle::GetSectionSpacing()
{
    return FMargin(0, SECTION_OUTER_SPACING, 0, SECTION_OUTER_SPACING);
}

FMargin FEditorStyle::GetSectionStackSpacing()
{
    return FMargin(0, 0, 0, SECTION_STACK_SPACING);
}

FExpander::FDesc FEditorStyle::MakeExpanderDesc(const String& Label, const TSharedPtr<FVisualElement>& Content, bool bIsExpanded)
{
    FExpander::FDesc Desc;
    Desc.Label          = Label;
    Desc.Font           = GFonts.BodyBold;
    Desc.Content        = Content;
    Desc.bIsExpanded    = bIsExpanded;
    Desc.HeaderHeight   = SECTION_HEADER_HEIGHT;
    Desc.ExpandedArrow  = FEditorIcons::CollapseArrowDown;
    Desc.CollapsedArrow = FEditorIcons::CollapseArrowRight;
    Desc.ArrowSize      = IconSize;
    Desc.ContentPadding = FMargin(SECTION_CONTENT_INDENT, 8, 8, 8);
    Desc.Style          = FUIStyle::GetDefault().Header;
    Desc.bDrawBottomBorderWhenClosed = false;
    return Desc;
}

FPropertyTable::FDesc FEditorStyle::MakePropertyTableDesc(float LabelColumnFraction, int32 LabelColumnWidth)
{
    FPropertyTable::FDesc Desc;
    Desc.Font                = GFonts.Body;
    Desc.HeaderFont          = GFonts.BodyBold;
    Desc.RowHeight           = FrameHeight + Desc.Style.CellPadding.GetTotalVertical();
    Desc.HeaderRowHeight     = SECTION_HEADER_HEIGHT;
    Desc.LabelColumnFraction = LabelColumnFraction;
    Desc.LabelColumnWidth    = LabelColumnWidth;
    Desc.RevertIcon          = FEditorIcons::Undo;
    Desc.bShowRevertColumn   = true;
    return Desc;
}

FPropertyTable::FDesc FEditorStyle::MakeDataTableDesc()
{
    FPropertyTable::FDesc Desc;
    Desc.Font                = GFonts.Body;
    Desc.HeaderFont          = GFonts.BodyBold;
    Desc.RowHeight           = RowHeight;
    Desc.HeaderRowHeight     = SECTION_HEADER_HEIGHT;
    Desc.bAlternateRowColors = true;
    return Desc;
}

FPropertyTable::FDesc FEditorStyle::MakeInfoTableDesc()
{
    FPropertyTable::FDesc Desc = MakeDataTableDesc();
    Desc.LabelColumnWidth      = INFO_LABEL_COLUMN_WIDTH;
    Desc.EditorColumnInset     = Desc.Style.CellPadding.Left;
    return Desc;
}

void FEditorStyle::ApplyTreeViewArrows(FTreeView::FDesc& OutDesc)
{
    OutDesc.ExpandedArrow  = FEditorIcons::CollapseArrowDown;
    OutDesc.CollapsedArrow = FEditorIcons::CollapseArrowRight;
    OutDesc.ArrowSize      = IconSize;
}
