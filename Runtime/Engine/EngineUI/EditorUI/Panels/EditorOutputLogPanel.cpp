#include "Engine/EngineUI/EditorUI/Panels/EditorOutputLogPanel.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Application.h"
#include "Application/ElementPath.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/Image.h"
#include "Application/Elements/LogView.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuAnchor.h"
#include "Application/Menus/MenuItem.h"
#include "Application/Menus/MenuStack.h"

constexpr float LOG_SEARCH_FILL = 1.0f / 3.0f;

constexpr int32 LOG_TOOLBAR_GAP = 8;

constexpr int32 LOG_FILTER_FACE_GAP = 4;

FEditorLogContextArea::FEditorLogContextArea()
    : FCompoundElement()
    , OnContextMenu()
{
}

FEditorLogContextArea::~FEditorLogContextArea() = default;

TSharedPtr<FEditorLogContextArea> FEditorLogContextArea::Create(const TSharedPtr<FVisualElement>& InContent, const FOnLogContextMenu& InOnContextMenu)
{
    TSharedPtr<FEditorLogContextArea> NewArea = MakeSharedPtr<FEditorLogContextArea>();
    NewArea->OnContextMenu = InOnContextMenu;
    NewArea->SetContent(InContent);
    return NewArea;
}

FEventResponse FEditorLogContextArea::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonRight)
    {
        return FEventResponse::Unhandled();
    }

    OnContextMenu.ExecuteIfBound(CursorEvent.GetScreenPosition());
    return FEventResponse::Handled();
}

FEditorOutputLogPanel::FEditorOutputLogPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "OutputLog", "Output Log")
    , LogView(nullptr)
    , SearchBox(nullptr)
    , ToolBar(nullptr)
    , LogArea(nullptr)
{
}

FEditorOutputLogPanel::~FEditorOutputLogPanel()
{
}

bool FEditorOutputLogPanel::Initialize()
{
    FLogView::FDesc LogDesc;
    LogDesc.Font                = FEditorStyle::GetFonts().Monospace;
    LogDesc.MinimumSeverity     = ELogSeverity::Info;
    LogDesc.bAutoScroll         = true;
    LogDesc.bShowSeverityPrefix = true;

    LogView = FLogView::Create(LogDesc);
    if (!LogView)
    {
        return false;
    }

    LogView->RegisterWithLogger();

    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    LogArea = FEditorLogContextArea::Create(LogView, FOnLogContextMenu::CreateRaw(this, &FEditorOutputLogPanel::OnLogContextMenu));
    if (!LogArea)
    {
        return false;
    }

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(ToolBar).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Column->AddSlot(FEditorStyle::MakeInnerFrame(LogArea)).SetFillCoefficient(1.0f);

    Content = Column;
    return true;
}

TSharedPtr<FToolBar> FEditorOutputLogPanel::BuildToolBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.ItemSpacing    = LOG_TOOLBAR_GAP;
    Desc.Padding        = FMargin(0);
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Log",
        FOnSearchTextChanged::CreateRaw(this, &FEditorOutputLogPanel::OnSearchTextChanged)));

    Bar->AddWidget(SearchBox, LOG_SEARCH_FILL);
    Bar->AddWidget(BuildFilterButton());
    Bar->AddFlexibleSpace();

    return Bar;
}

TSharedPtr<FVisualElement> FEditorOutputLogPanel::BuildFilterButton()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    const auto MakeGlyph = [&Style](const FUIBrush& Brush)
    {
        FImage::FDesc IconDesc;
        IconDesc.Brush       = Brush;
        IconDesc.DesiredSize = IntVector2(FEditorStyle::IconSize, FEditorStyle::IconSize);
        IconDesc.Tint        = Style.Colors.Text;

        return FImage::Create(IconDesc);
    };

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = "Filter";
    LabelDesc.Font            = FEditorStyle::GetFonts().Body;
    LabelDesc.ColorAndOpacity = Style.Colors.Text;

    TSharedPtr<FHorizontalBox> Face = FHorizontalBox::Create();
    Face->AddSlot(MakeGlyph(FEditorIcons::Filter)).SetVerticalAlignment(EVerticalAlignment::Center);
    Face->AddSlot(FSpacer::CreateHorizontal(LOG_FILTER_FACE_GAP));
    Face->AddSlot(FTextBlock::Create(LabelDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Face->AddSlot(FSpacer::CreateHorizontal(LOG_FILTER_FACE_GAP));
    Face->AddSlot(MakeGlyph(FEditorIcons::DownArrow)).SetVerticalAlignment(EVerticalAlignment::Center);

    FButton::FDesc ButtonDesc;
    ButtonDesc.Font     = FEditorStyle::GetFonts().Body;
    ButtonDesc.Content  = Face;
    ButtonDesc.bIsGhost = true;

    FilterButton = FButton::Create(ButtonDesc);
    if (!FilterButton)
    {
        return nullptr;
    }

    FMenuAnchor::FDesc AnchorDesc;
    AnchorDesc.Content   = FilterButton;
    AnchorDesc.Placement = EMenuPlacement::BelowLeftAligned;

    AnchorDesc.OnGetMenuContent.BindRaw(this, &FEditorOutputLogPanel::BuildFilterMenu);

    FilterAnchor = FMenuAnchor::Create(AnchorDesc);
    if (!FilterAnchor)
    {
        return nullptr;
    }

    FilterButton->SetOnClicked(FOnClicked::CreateLambda([this]()
    {
        FilterAnchor->Toggle();
    }));

    return FilterAnchor;
}

void FEditorOutputLogPanel::Tick(float DeltaTime)
{
    FEditorPanel::Tick(DeltaTime);

    if (FilterButton && FilterAnchor)
    {
        FilterButton->SetHighlighted(FilterAnchor->IsOpen());
    }
}

TSharedPtr<FVisualElement> FEditorOutputLogPanel::BuildFilterMenu()
{
    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->AddSection("Verbosity", FEditorStyle::GetFonts().Body);

    const auto AddSeverityToggle = [this, &Menu](const CHAR* ItemLabel, ELogSeverity Severity)
    {
        FMenuItem::FDesc ItemDesc;
        ItemDesc.Label        = ItemLabel;
        ItemDesc.Font         = FEditorStyle::GetFonts().Body;
        ItemDesc.bIsCheckable = true;
        ItemDesc.CheckState   = LogView->IsSeverityVisible(Severity) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

        ItemDesc.OnActivated.BindLambda([this, Severity]()
        {
            LogView->SetSeverityVisible(Severity, !LogView->IsSeverityVisible(Severity));
        });

        Menu->AddItem(FMenuItem::Create(ItemDesc));
    };

    AddSeverityToggle("Messages", ELogSeverity::Info);
    AddSeverityToggle("Warnings", ELogSeverity::Warning);
    AddSeverityToggle("Errors", ELogSeverity::Error);

    return Menu;
}

void FEditorOutputLogPanel::Release()
{
    if (LogView)
    {
        LogView->UnregisterFromLogger();
        LogView.Reset();
    }

    SearchBox.Reset();
    ToolBar.Reset();
    LogArea.Reset();
    FilterButton.Reset();
    FilterAnchor.Reset();

    FEditorPanel::Release();
}

void FEditorOutputLogPanel::OnSearchTextChanged(const String& SearchText)
{
    LogView->SetSearchText(SearchText, !SearchText.IsEmpty());
}

void FEditorOutputLogPanel::OnLogContextMenu(const IntVector2& ScreenPosition)
{
    if (!FApplication::IsInitialized() || !LogView)
    {
        return;
    }

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return;
    }

    const auto AddCommand = [&Menu](const CHAR* ItemLabel, const TDelegate<void()>& OnActivated)
    {
        FMenuItem::FDesc ItemDesc;
        ItemDesc.Label       = ItemLabel;
        ItemDesc.Font        = FEditorStyle::GetFonts().Body;
        ItemDesc.OnActivated = OnActivated;

        Menu->AddItem(FMenuItem::Create(ItemDesc));
    };

    Menu->AddSection("Copy", FEditorStyle::GetFonts().Body);

    AddCommand("Select All", FOnMenuItemActivated::CreateLambda([this]()
    {
        LogView->SelectAll();
    }));

    AddCommand("Copy", FOnMenuItemActivated::CreateLambda([this]()
    {
        LogView->CopyToClipboard();
    }));

    AddCommand("Copy All", FOnMenuItemActivated::CreateLambda([this]()
    {
        LogView->SelectAll();
        LogView->CopyToClipboard();
    }));

    Menu->AddSection("Search", FEditorStyle::GetFonts().Body);

    AddCommand("Find", FOnMenuItemActivated::CreateLambda([this]()
    {
        if (SearchBox)
        {
            FApplication::Get().SetFocusElement(SearchBox->GetEditor());
        }
    }));

    Menu->AddSection("Log", FEditorStyle::GetFonts().Body);

    AddCommand("Clear Log", FOnMenuItemActivated::CreateLambda([this]()
    {
        LogView->Clear();
    }));

    FMenuStack::Get().PushMenu(LogArea, FRectangle(ScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
}

int32 FEditorOutputLogPanel::GetNumVisibleLines() const
{
    return LogView ? LogView->GetNumVisibleLines() : 0;
}
