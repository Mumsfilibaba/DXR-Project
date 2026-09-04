#include "Engine/EngineUI/EditorUI/Panels/EditorOutputLogPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/FractionWidthBox.h"
#include "Application/Elements/LogView.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Menus/ComboBox.h"

// The share of the tool bar the log filter takes, floored so it stays usable in a narrow panel
constexpr float LOG_SEARCH_FRACTION = 0.25f;
constexpr int32 LOG_SEARCH_MIN      = 120;

static const ELogSeverity GSeverityOptions[] =
{
    ELogSeverity::Info,
    ELogSeverity::Warning,
    ELogSeverity::Error,
};

FEditorOutputLogPanel::FEditorOutputLogPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "OutputLog", "Output Log")
    , LogView(nullptr)
    , SearchBox(nullptr)
    , ToolBar(nullptr)
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

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(ToolBar);
    Column->AddSlot(LogView).SetFillCoefficient(1.0f);

    Content = Column;
    return true;
}

TSharedPtr<FToolBar> FEditorOutputLogPanel::BuildToolBar()
{
    FToolBar::FDesc Desc;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.IconSize       = FEditorStyle::IconSize;
    Desc.bHasBackground = true;

    TSharedPtr<FToolBar> Bar = FToolBar::Create(Desc);
    if (!Bar)
    {
        return nullptr;
    }

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Log",
        FOnSearchTextChanged::CreateRaw(this, &FEditorOutputLogPanel::OnSearchTextChanged)));

    Bar->AddWidget(FFractionWidthBox::Create(SearchBox, LOG_SEARCH_FRACTION, LOG_SEARCH_MIN), 1.0f);

    FComboBox::FDesc SeverityDesc;
    SeverityDesc.Options            = { "Info", "Warning", "Error" };
    SeverityDesc.SelectedIndex      = 0;
    SeverityDesc.Font               = FEditorStyle::GetFonts().Body;
    SeverityDesc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([this](int32 Index)
    {
        if (Index >= 0 && Index < static_cast<int32>(ARRAY_COUNT(GSeverityOptions)))
        {
            OnSeverityChanged(GSeverityOptions[Index]);
        }
    });

    Bar->AddWidget(FComboBox::Create(SeverityDesc));

    Bar->AddSeparator();

    Bar->AddToggle(FToolBarItemDesc().SetLabel("Autoscroll"), ECheckBoxState::Checked,
        FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State)
        {
            LogView->SetAutoScroll(State == ECheckBoxState::Checked);
        }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Copy"), FOnClicked::CreateLambda([this]()
    {
        LogView->CopyToClipboard();
    }));

    Bar->AddButton(FToolBarItemDesc().SetLabel("Clear"), FOnClicked::CreateLambda([this]()
    {
        LogView->Clear();
    }));

    Bar->AddFlexibleSpace();

    return Bar;
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

    FEditorPanel::Release();
}

void FEditorOutputLogPanel::OnSearchTextChanged(const String& SearchText)
{
    LogView->SetSearchText(SearchText, !SearchText.IsEmpty());
}

void FEditorOutputLogPanel::OnSeverityChanged(ELogSeverity Severity)
{
    LogView->SetMinimumSeverity(Severity);
}

int32 FEditorOutputLogPanel::GetNumVisibleLines() const
{
    return LogView ? LogView->GetNumVisibleLines() : 0;
}
