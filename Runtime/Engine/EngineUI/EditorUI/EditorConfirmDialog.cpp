#include "Engine/EngineUI/EditorUI/EditorConfirmDialog.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Application.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/PopupWindow.h"
#include "Core/Math/Math.h"

// The ring of empty space between the dialog's border and its text, in pixels
constexpr int32 DIALOG_PADDING = 16;

// The gap under the title and above the button row, in pixels
constexpr int32 DIALOG_TITLE_GAP  = 8;
constexpr int32 DIALOG_BUTTON_GAP = 16;

// How wide a button is laid out and how far the two are apart, in pixels, both taken from the ImGui dialog
constexpr int32 DIALOG_BUTTON_WIDTH   = 100;
constexpr int32 DIALOG_BUTTON_SPACING = 12;

struct FConfirmDialogState
{
    TSharedPtr<FWindow>    PopupWindow;
    FOnConfirmDialogClosed OnClosed;
};

static void CloseDialog(const TSharedPtr<FConfirmDialogState>& State, bool bConfirmed)
{
    if (!State || !State->PopupWindow)
    {
        return;
    }

    const TSharedPtr<FWindow>    PopupWindow = State->PopupWindow;
    const FOnConfirmDialogClosed OnClosed    = State->OnClosed;

    State->PopupWindow.Reset();

    Popups::Close(PopupWindow);
    OnClosed.ExecuteIfBound(bConfirmed);
}

static TSharedPtr<FButton> CreateDialogButton(const String& Label, const FOnClicked& OnClicked)
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
    Content->AddSlot(FSpacer::CreateHorizontal(DIALOG_BUTTON_WIDTH));
    Content->AddSlot(LabelText).SetHorizontalAlignment(EHorizontalAlignment::Center).SetVerticalAlignment(EVerticalAlignment::Center);

    const FMargin& ButtonPadding = FUIStyle::GetDefault().Metrics.ButtonPadding;

    FButton::FDesc Desc;
    Desc.Font      = FEditorStyle::GetFonts().Body;
    Desc.Padding   = FMargin(0, ButtonPadding.Top, 0, ButtonPadding.Bottom);
    Desc.MinHeight = FEditorStyle::ButtonHeight;
    Desc.Content   = Content;
    Desc.OnClicked = OnClicked;

    return FButton::Create(Desc);
}

bool FEditorConfirmDialog::Open(const TSharedPtr<FVisualElement>& AnchorElement, const FDesc& Desc)
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

    TSharedPtr<FConfirmDialogState> State = MakeSharedPtr<FConfirmDialogState>();
    State->OnClosed = Desc.OnClosed;

    TSharedPtr<FButton> ConfirmButton = CreateDialogButton(Desc.ConfirmLabel, FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State, true);
    }));

    TSharedPtr<FButton> CancelButton = CreateDialogButton(Desc.CancelLabel, FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State, false);
    }));

    if (!ConfirmButton || !CancelButton)
    {
        return false;
    }

    TSharedPtr<FHorizontalBox> ButtonRow = FHorizontalBox::Create();
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
    ButtonRow->AddSlot(ConfirmButton).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(DIALOG_BUTTON_SPACING));
    ButtonRow->AddSlot(CancelButton).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    if (!Desc.Title.IsEmpty())
    {
        FTextBlock::FDesc TitleDesc;
        TitleDesc.Text            = Desc.Title;
        TitleDesc.Font            = FEditorStyle::GetFonts().BodyBold;
        TitleDesc.ColorAndOpacity = Style.Colors.Text;

        Column->AddSlot(FTextBlock::Create(TitleDesc));
        Column->AddSlot(FSpacer::CreateVertical(DIALOG_TITLE_GAP));
    }

    FTextBlock::FDesc MessageDesc;
    MessageDesc.Text            = Desc.Message;
    MessageDesc.Font            = FEditorStyle::GetFonts().Body;
    MessageDesc.ColorAndOpacity = Style.Colors.Text;

    Column->AddSlot(FTextBlock::Create(MessageDesc));
    Column->AddSlot(FSpacer::CreateVertical(DIALOG_BUTTON_GAP)).SetFillCoefficient(1.0f);
    Column->AddSlot(ButtonRow);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = Style.Panel.Fill;
    BorderDesc.BorderColor     = Style.Panel.Border;
    BorderDesc.BorderThickness = Style.Panel.BorderThickness;
    BorderDesc.CornerRadius    = FCornerRadii(Style.Panel.CornerRadius);
    BorderDesc.Padding         = FMargin(DIALOG_PADDING, DIALOG_PADDING, DIALOG_PADDING, DIALOG_PADDING);
    BorderDesc.Content         = Column;

    TSharedPtr<FBorder> Frame = FBorder::Create(BorderDesc);
    if (!Frame)
    {
        return false;
    }

    Frame->PrepareDesiredSize();

    const IntVector2 DesiredSize = Frame->GetCachedDesiredSize();
    const IntVector2 DialogSize(Math::Max(DesiredSize.X, MinWidth), Math::Max(DesiredSize.Y, 1));

    const IntVector2 ParentPosition = ParentWindow->GetPosition();
    const IntVector2 ParentSize     = ParentWindow->GetSize();
    const IntVector2 DialogPosition(
        ParentPosition.X + ((ParentSize.X - DialogSize.X) / 2),
        ParentPosition.Y + ((ParentSize.Y - DialogSize.Y) / 2));

    const FRectangle DialogBounds = Popups::ClampToWorkArea(FRectangle(DialogPosition, DialogSize.X, DialogSize.Y));

    State->PopupWindow = Popups::Open(ParentWindow, DialogBounds, Frame);
    return State->PopupWindow != nullptr;
}
