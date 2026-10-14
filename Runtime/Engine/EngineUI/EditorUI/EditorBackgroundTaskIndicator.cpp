#include "Engine/EngineUI/EditorUI/EditorBackgroundTaskIndicator.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/Core/BackgroundTasks.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/ProgressBar.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/MenuAnchor.h"

static constexpr int32 FooterBarWidth  = 120;
static constexpr int32 FooterBarHeight = 6;
static constexpr int32 ListBarWidth    = 220;
static constexpr int32 ListBarHeight   = 6;
static constexpr int32 ListPadding     = 10;
static constexpr int32 ListRowGap      = 10;
static constexpr int32 LabelGap        = 8;

static TSharedPtr<FProgressBar> CreateBar(int32 Height)
{
    FProgressBar::FDesc Desc;
    Desc.PreferredHeight = Height;
    Desc.CornerRadius    = FCornerRadii(static_cast<float>(Height) * 0.5f);
    return FProgressBar::Create(Desc);
}

static TSharedPtr<FVisualElement> FixWidth(const TSharedPtr<FVisualElement>& Content, int32 Width, int32 Height)
{
    TSharedPtr<FOverlay> Overlay = FOverlay::Create();
    Overlay->AddSlot(FSpacer::Create(FIntVector2(Width, Height)));
    Overlay->AddSlot(Content).SetHorizontalAlignment(EHorizontalAlignment::Fill).SetVerticalAlignment(EVerticalAlignment::Center);
    return Overlay;
}

static TSharedPtr<FTextBlock> CreateLabel(const FFloatColor& Color)
{
    FTextBlock::FDesc Desc;
    Desc.Font            = FEditorStyle::GetFonts().Body;
    Desc.ColorAndOpacity = Color;
    return FTextBlock::Create(Desc);
}

void FEditorBackgroundTaskIndicator::UpdateBar(const TSharedPtr<FProgressBar>& Bar, const TSharedPtr<FBackgroundTask>& Task)
{
    const float Progress = Task->GetProgress();
    Bar->SetIndeterminate(Progress == FBackgroundTask::UnknownProgress);
    if (Progress != FBackgroundTask::UnknownProgress)
    {
        Bar->SetPercent(Progress);
    }
}

FEditorBackgroundTaskIndicator::FEditorBackgroundTaskIndicator()
    : Tasks()
    , TaskRows()
    , Label(nullptr)
    , Bar(nullptr)
    , TaskList(nullptr)
    , Anchor(nullptr)
    , Element(nullptr)
{
}

FEditorBackgroundTaskIndicator::~FEditorBackgroundTaskIndicator() = default;

bool FEditorBackgroundTaskIndicator::Initialize()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    Label = CreateLabel(Style.Colors.Text);
    Bar   = CreateBar(FooterBarHeight);
    if (!Label || !Bar)
    {
        return false;
    }

    TSharedPtr<FHorizontalBox> ButtonContent = FHorizontalBox::Create();
    ButtonContent->AddSlot(Label).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonContent->AddSlot(FSpacer::CreateHorizontal(LabelGap));
    ButtonContent->AddSlot(FixWidth(Bar, FooterBarWidth, FooterBarHeight)).SetVerticalAlignment(EVerticalAlignment::Center);

    FButton::FDesc ButtonDesc;
    ButtonDesc.Font      = FEditorStyle::GetFonts().Body;
    ButtonDesc.MinHeight = FEditorStyle::ButtonHeight;
    ButtonDesc.bIsGhost  = true;
    ButtonDesc.Content   = ButtonContent;

    TSharedPtr<FButton> Button = FButton::Create(ButtonDesc);

    TaskList = FVerticalBox::Create();

    FBorder::FDesc ListDesc;
    ListDesc.BackgroundColor = Style.Panel.Fill;
    ListDesc.BorderColor     = Style.Panel.Border;
    ListDesc.BorderThickness = Style.Panel.BorderThickness;
    ListDesc.CornerRadius    = FCornerRadii(Style.Panel.CornerRadius);
    ListDesc.Padding         = FMargin(ListPadding, ListPadding, ListPadding, ListPadding);
    ListDesc.Content         = TaskList;

    FMenuAnchor::FDesc AnchorDesc;
    AnchorDesc.SetContent(Button);
    AnchorDesc.SetMenuContent(FBorder::Create(ListDesc));
    AnchorDesc.Placement = EMenuPlacement::BelowLeftAligned;

    Anchor = FMenuAnchor::Create(AnchorDesc);
    if (!Button || !TaskList || !Anchor)
    {
        return false;
    }

    Button->SetOnClicked(FOnClicked::CreateLambda([this]()
    {
        RebuildTaskRows();
        Anchor->Toggle();
    }));

    Element = Anchor;
    Element->SetVisibility(EVisibility::Hidden);
    return true;
}

void FEditorBackgroundTaskIndicator::Refresh()
{
    const TArray<TSharedPtr<FBackgroundTask>> Visible = FBackgroundTasks::Get().GetVisibleTasks();

    if (Visible.IsEmpty())
    {
        if (!Tasks.IsEmpty())
        {
            Tasks.Clear();
            Anchor->Close();
            Element->SetVisibility(EVisibility::Hidden);
        }

        return;
    }

    const bool bChanged = !(Visible == Tasks);
    Tasks = Visible;

    if (bChanged)
    {
        Label->SetText(Tasks.Size() > 1 ? FString::Printf(TEXT("%s  +%d more"), *Tasks[0]->GetLabel(), Tasks.Size() - 1) : Tasks[0]->GetLabel());
        Element->SetVisibility(EVisibility::Visible);

        if (Anchor->IsOpen())
        {
            RebuildTaskRows();
        }
    }

    UpdateBar(Bar, Tasks[0]);
    for (const FTaskRow& Row : TaskRows)
    {
        UpdateBar(Row.Bar, Row.Task);
    }
}

void FEditorBackgroundTaskIndicator::RebuildTaskRows()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TaskList->ClearSlots();
    TaskRows.Clear();

    for (int32 Index = 0; Index < Tasks.Size(); ++Index)
    {
        const TSharedPtr<FBackgroundTask> Task = Tasks[Index];

        TSharedPtr<FTextBlock> RowLabel = CreateLabel(Style.Colors.Text);
        RowLabel->SetText(Task->GetLabel());

        TSharedPtr<FProgressBar> RowBar = CreateBar(ListBarHeight);
        UpdateBar(RowBar, Task);

        TSharedPtr<FHorizontalBox> BarRow = FHorizontalBox::Create();
        BarRow->AddSlot(FixWidth(RowBar, ListBarWidth, ListBarHeight)).SetVerticalAlignment(EVerticalAlignment::Center);

        if (Task->CanCancel())
        {
            FButton::FDesc CancelDesc;
            CancelDesc.Text      = TEXT("Cancel");
            CancelDesc.Font      = FEditorStyle::GetFonts().Body;
            CancelDesc.MinHeight = FUIStyle::GetDefault().Metrics.ButtonHeight;
            CancelDesc.OnClicked = FOnClicked::CreateLambda([Task]()
            {
                Task->RequestCancel();
            });

            BarRow->AddSlot(FSpacer::CreateHorizontal(LabelGap));
            BarRow->AddSlot(FButton::Create(CancelDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
        }

        if (Index > 0)
        {
            TaskList->AddSlot(FSpacer::CreateVertical(ListRowGap));
        }

        TaskList->AddSlot(RowLabel);
        TaskList->AddSlot(FSpacer::CreateVertical(4));
        TaskList->AddSlot(BarRow);

        TaskRows.Add({ Task, RowBar });
    }
}
