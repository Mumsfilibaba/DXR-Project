#include "Engine/EngineUI/EditorUI/EditorErrorDialog.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Application.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/PopupWindow.h"
#include "Core/Math/Math.h"

// The ring of empty space between the dialog's border and its text, in pixels
constexpr int32 ERROR_DIALOG_PADDING = 16;

// The gap under the title, under the message and above the button row, in pixels
constexpr int32 ERROR_DIALOG_TITLE_GAP   = 8;
constexpr int32 ERROR_DIALOG_MESSAGE_GAP = 8;
constexpr int32 ERROR_DIALOG_BUTTON_GAP  = 16;

// How wide the dismiss button is laid out, in pixels, taken from the ImGui dialog
constexpr int32 ERROR_DIALOG_BUTTON_WIDTH = 100;

// How far a detail row is indented past the message it belongs to, in pixels
constexpr int32 ERROR_DIALOG_DETAIL_INDENT = 12;

struct FErrorDialogState
{
    TSharedPtr<FWindow>  PopupWindow;
    FOnErrorDialogClosed OnClosed;
};

static void CloseDialog(const TSharedPtr<FErrorDialogState>& State)
{
    if (!State || !State->PopupWindow)
    {
        return;
    }

    const TSharedPtr<FWindow>  PopupWindow = State->PopupWindow;
    const FOnErrorDialogClosed OnClosed    = State->OnClosed;

    State->PopupWindow.Reset();

    Popups::Close(PopupWindow);
    OnClosed.ExecuteIfBound();
}

static TSharedPtr<FButton> CreateDismissButton(const String& Label, const FOnClicked& OnClicked)
{
    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = Label;
    LabelDesc.Font            = FEditorStyle::GetFonts().Body;
    LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    TSharedPtr<FTextBlock> LabelText = FTextBlock::Create(LabelDesc);
    if (!LabelText)
    {
        return nullptr;
    }

    TSharedPtr<FOverlay> Content = FOverlay::Create();
    Content->AddSlot(FSpacer::CreateHorizontal(ERROR_DIALOG_BUTTON_WIDTH));
    Content->AddSlot(LabelText).SetHorizontalAlignment(EHorizontalAlignment::Center).SetVerticalAlignment(EVerticalAlignment::Center);

    FButton::FDesc Desc;
    Desc.Font      = FEditorStyle::GetFonts().Body;
    Desc.Padding   = FMargin(0, 4, 0, 4);
    Desc.Content   = Content;
    Desc.OnClicked = OnClicked;

    return FButton::Create(Desc);
}

static TSharedPtr<FVisualElement> CreateDetailList(const TArray<String>& Details)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FVerticalBox> Rows = FVerticalBox::Create();
    for (const String& Detail : Details)
    {
        FTextBlock::FDesc RowDesc;
        RowDesc.Text            = Detail;
        RowDesc.Font            = Font;
        RowDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;
        RowDesc.Overflow        = ETextOverflow::Elide;

        Rows->AddSlot(FTextBlock::Create(RowDesc)).SetPadding(FMargin(ERROR_DIALOG_DETAIL_INDENT, 0, 0, 0));
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return nullptr;
    }

    ScrollBox->SetContent(Rows);
    return ScrollBox;
}

bool FEditorErrorDialog::Open(const TSharedPtr<FVisualElement>& AnchorElement, const FDesc& Desc)
{
    if (!AnchorElement || !FApplication::IsInitialized())
    {
        return false;
    }

    TSharedPtr<FWindow> ParentWindow = FApplication::Get().FindWindow(AnchorElement);
    if (!ParentWindow)
    {
        return false;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FErrorDialogState> State = MakeSharedPtr<FErrorDialogState>();
    State->OnClosed = Desc.OnClosed;

    TSharedPtr<FButton> DismissButton = CreateDismissButton(Desc.DismissLabel, FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State);
    }));

    if (!DismissButton)
    {
        return false;
    }

    TSharedPtr<FHorizontalBox> ButtonRow = FHorizontalBox::Create();
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
    ButtonRow->AddSlot(DismissButton).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    if (!Desc.Title.IsEmpty())
    {
        FTextBlock::FDesc TitleDesc;
        TitleDesc.Text            = Desc.Title;
        TitleDesc.Font            = FEditorStyle::GetFonts().BodyBold;
        TitleDesc.ColorAndOpacity = Style.Colors.Text;

        Column->AddSlot(FTextBlock::Create(TitleDesc));
        Column->AddSlot(FSpacer::CreateVertical(ERROR_DIALOG_TITLE_GAP));
    }

    FTextBlock::FDesc MessageDesc;
    MessageDesc.Text            = Desc.Message;
    MessageDesc.Font            = FEditorStyle::GetFonts().Body;
    MessageDesc.ColorAndOpacity = Style.Colors.Text;

    Column->AddSlot(FTextBlock::Create(MessageDesc));

    int32 ListOverflow = 0;
    if (!Desc.Details.IsEmpty())
    {
        TSharedPtr<FVisualElement> DetailList = CreateDetailList(Desc.Details);
        if (!DetailList)
        {
            return false;
        }

        Column->AddSlot(FSpacer::CreateVertical(ERROR_DIALOG_MESSAGE_GAP));
        Column->AddSlot(DetailList).SetFillCoefficient(1.0f);

        const int32 RowExtent = FEditorStyle::GetFonts().Body->GetLineHeight();
        ListOverflow = Math::Max((Desc.Details.Size() - MaxVisibleRows) * RowExtent, 0);
    }

    Column->AddSlot(FSpacer::CreateVertical(ERROR_DIALOG_BUTTON_GAP));
    Column->AddSlot(ButtonRow);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = Style.Colors.PanelBackground;
    BorderDesc.BorderColor     = Style.Colors.Border;
    BorderDesc.BorderThickness = Style.Metrics.BorderThickness;
    BorderDesc.CornerRadius    = FCornerRadii(Style.Metrics.CornerRadius);
    BorderDesc.Padding         = FMargin(ERROR_DIALOG_PADDING, ERROR_DIALOG_PADDING, ERROR_DIALOG_PADDING, ERROR_DIALOG_PADDING);
    BorderDesc.Content         = Column;

    TSharedPtr<FBorder> Frame = FBorder::Create(BorderDesc);
    if (!Frame)
    {
        return false;
    }

    Frame->PrepareDesiredSize();

    const IntVector2 DesiredSize = Frame->GetCachedDesiredSize();
    const IntVector2 DialogSize(Math::Max(DesiredSize.X, MinWidth), Math::Max(DesiredSize.Y - ListOverflow, 1));

    const IntVector2 ParentPosition = ParentWindow->GetPosition();
    const IntVector2 ParentSize     = ParentWindow->GetSize();
    const IntVector2 DialogPosition(
        ParentPosition.X + ((ParentSize.X - DialogSize.X) / 2),
        ParentPosition.Y + ((ParentSize.Y - DialogSize.Y) / 2));

    const FRectangle DialogBounds = Popups::ClampToWorkArea(FRectangle(DialogPosition, DialogSize.X, DialogSize.Y));

    State->PopupWindow = Popups::Open(ParentWindow, DialogBounds, Frame);
    return State->PopupWindow != nullptr;
}
