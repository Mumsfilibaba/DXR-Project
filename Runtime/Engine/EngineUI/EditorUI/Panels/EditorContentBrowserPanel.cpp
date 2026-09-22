#include "Engine/EngineUI/EditorUI/Panels/EditorContentBrowserPanel.h"
#include "Core/Algorithms/Algorithm.h"
#include "Engine/EngineUI/EditorUI/EditorConfirmDialog.h"
#include "Engine/EngineUI/EditorUI/EditorErrorDialog.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Application.h"
#include "Application/ElementPath.h"
#include "Application/Docking/Splitter.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/FractionWidthBox.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/TileView.h"
#include "Application/Elements/TreeView.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/DragDropService.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuItem.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Elements/IndexedPathMove.h"

static const CHAR* GItemDragDropPayloadId   = "CB_MOVE_ITEM";
static const CHAR* GFolderDragDropPayloadId = "CB_MOVE_FOLDER";
static const CHAR* GNewFolderName           = "New folder";

constexpr float DROP_INDICATOR_THICKNESS = 2.0f;
constexpr int32 FOLDER_PANEL_WIDTH = 220;
constexpr int32 FOLDER_PANEL_MIN   = 200;
constexpr int32 CONTENT_PANEL_MIN  = 250;

constexpr int32 COLUMN_PADDING = 4;

constexpr int32 BREADCRUMB_GAP            = FEditorStyle::ItemSpacing;
constexpr int32 BREADCRUMB_SEPARATOR_SIZE = 12;
constexpr int32 BREADCRUMB_NAV_SIZE       = FEditorStyle::FrameHeight;
constexpr int32 BREADCRUMB_NAV_ICON_SIZE  = 24;
constexpr int32 BREADCRUMB_NAV_GAP        = 4;
constexpr int32 BREADCRUMB_BAR_PADDING    = FEditorStyle::ItemSpacing;
constexpr int32 BREADCRUMB_FIELD_PADDING  = 4;
constexpr float BREADCRUMB_FIELD_CORNER   = 4.0f;

// A search field with the column's own padding above and below it, which is what the band holds
constexpr int32 FOLDER_HEADER_HEIGHT = FEditorStyle::FrameHeight + (2 * COLUMN_PADDING);
constexpr int32 FOLDER_ROW_HEIGHT    = FEditorStyle::RowHeight;

constexpr int32 TILE_WIDTH       = 132;
constexpr int32 TILE_HEIGHT      = 158;
constexpr int32 TILE_SPACING     = 8;
constexpr int32 TILE_VIEW_INSET  = 12;
constexpr int32 TILE_ICON_SIZE   = 110;
constexpr int32 TILE_LABEL_INSET = 8;
constexpr float TILE_CORNER      = 6.0f;

constexpr float CONTENT_SEARCH_FRACTION = 0.25f;
constexpr int32 CONTENT_SEARCH_MIN      = 120;

/** @brief Called with the target under a point, which is InvalidTarget when the point is over none. */
DECLARE_RETURN_DELEGATE(FOnBrowserHitTest, int32, const IntVector2& /*ClientPosition*/);

/** @brief Called with a target, for the bounds to outline or to place an edit field over. */
DECLARE_RETURN_DELEGATE(FOnBrowserTargetBounds, FRectangle, int32 /*Target*/);

/** @brief Called with a target, for the text an edit over it starts from. */
DECLARE_RETURN_DELEGATE(FOnBrowserTargetLabel, String, int32 /*Target*/);

/** @brief Called with the target a drag is over, for whether dropping there would move anything. */
DECLARE_RETURN_DELEGATE(FOnBrowserCanDrop, bool, int32 /*Target*/);

/** @brief Called with the target that was right-clicked, which is InvalidTarget below the items. */
DECLARE_DELEGATE(FOnBrowserContextMenu, int32 /*Target*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called with the name an edit committed. */
DECLARE_DELEGATE(FOnBrowserRenameCommitted, int32 /*Target*/, const String& /*NewName*/);

/** @brief Called when F2 went down on the view, for the host to pick what to rename. */
DECLARE_DELEGATE(FOnBrowserRenameRequested);

/** @brief Called when the Delete key went down on the view. */
DECLARE_DELEGATE(FOnBrowserDeleteRequested);

/** @brief Called with the payload dropped on a target, which is InvalidTarget for the space below the items. */
DECLARE_DELEGATE(FOnBrowserDropped, const String& /*PayloadId*/, int32 /*Target*/);
DECLARE_DELEGATE(FOnBrowserDragPreview, int32 /*Target*/);
DECLARE_RETURN_DELEGATE(FOnBrowserHoverTip, String, int32 /*Target*/);

static IntVector2 ScreenToClient(const TSharedPtr<FVisualElement>& Element, const IntVector2& ScreenPosition)
{
    const FRectangle ScreenBounds = FMenuStack::GetScreenBounds(Element);
    if (ScreenBounds.IsEmpty())
    {
        return ScreenPosition;
    }

    return Element->GetContentRectangle().Position + (ScreenPosition - ScreenBounds.Position);
}

class FBrowserGlyph final : public FVisualElement
{
public:
    static TSharedPtr<FBrowserGlyph> Create(const FUIBrush& InBrush, int32 InSize, const FFloatColor& InTint)
    {
        TSharedPtr<FBrowserGlyph> NewGlyph = MakeSharedPtr<FBrowserGlyph>();
        NewGlyph->Brush = InBrush;
        NewGlyph->Size  = InSize;
        NewGlyph->Tint  = InTint;
        return NewGlyph;
    }

    FBrowserGlyph()
        : Brush()
        , Tint(FFloatColor::White)
        , Size(0)
    {
    }

    virtual ~FBrowserGlyph() = default;

    virtual IntVector2 ComputeDesiredSize() const override final
    {
        return IntVector2(Size, Size);
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final
    {
        if (Brush.IsValid())
        {
            const FRectangle Bounds = FRectangle::AlignInBounds(AllottedGeometry.Bounds, IntVector2(Size, Size),
                EHorizontalAlignment::Center, EVerticalAlignment::Center);

            OutCommandList.AddImage(LayerId, Bounds, Brush, Tint);
        }

        return LayerId;
    }

private:
    FUIBrush    Brush;
    FFloatColor Tint;
    int32       Size;
};

class FEditorContentBrowserView final : public FVisualElement
{
public:

    /** @brief The target reported for the space below the items, which means the folder being shown. */
    static constexpr int32 InvalidTarget = -1;

    struct FDesc
    {
        /** @brief The element wrapped, whose bounds and children this one takes over. */
        TSharedPtr<FVisualElement> View;

        /** @brief The face an edit field is drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief What a drag out of this view is called. */
        String PayloadId;

        /** @brief Where a target is, for the outline a drop candidate carries. */
        FOnBrowserTargetBounds TargetBounds;

        /** @brief Where a target's label is, which is where an edit field goes. */
        FOnBrowserTargetBounds LabelBounds;

        /** @brief The text an edit over a target starts from. */
        FOnBrowserTargetLabel TargetLabel;

        /** @brief The target under a point. */
        FOnBrowserHitTest HitTest;

        /** @brief Whether dropping on a target would move anything, which drives the outline's color. */
        FOnBrowserCanDrop CanDrop;

        /** @brief Fired with the target that was right-clicked. */
        FOnBrowserContextMenu OnContextMenu;

        /** @brief Fired with the name an edit committed. */
        FOnBrowserRenameCommitted OnRenameCommitted;

        /** @brief Fired when F2 went down. */
        FOnBrowserRenameRequested OnRenameRequested;

        /** @brief Fired when Delete went down. */
        FOnBrowserDeleteRequested OnDeleteRequested;

        /** @brief Fired with the payload dropped on the view. */
        FOnBrowserDropped OnDropped;

        /** @brief Fired while a drag is over a target, so the ghost can show Move / Cannot move. */
        FOnBrowserDragPreview OnDragPreview;

        /** @brief The hover tip for a target, empty for none. */
        FOnBrowserHoverTip HoverTip;
    };

public:
    static TSharedPtr<FEditorContentBrowserView> Create(const FDesc& Desc);

public:
    FEditorContentBrowserView();
    virtual ~FEditorContentBrowserView();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override final;
    virtual void OnArrange(const FRectangle& AllottedBounds) override final;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override final;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;

    /**
     * @brief Puts an edit field over a target's label and hands it the keyboard, replacing any edit
     * already open.
     *
     * @param Target The target to rename, ignored when it has no label bounds.
     */
    void BeginRename(int32 Target);

    /**
     * @brief Starts a drag out of this view, which the service then tracks until the button comes up.
     *
     * @param DisplayText What the ghost following the cursor reads.
     * @param CursorEvent The move that crossed the drag threshold.
     */
    void BeginDrag(const String& DisplayText, const FUIBrush& Icon, int32 SelectionCount, const FCursorEvent& CursorEvent);

    /** @brief Drops the field without writing anything back, and returns the keyboard to the view. */
    void CancelRename();

    /** @return True while an edit field is open over a target. */
    NODISCARD FORCEINLINE bool IsRenaming() const
    {
        return RenameField != nullptr;
    }

private:
    void Initialize(const FDesc& Desc);
    void CommitRename(const String& NewName);
    void ReleaseRenameField();
    void HandleDrop(const FDragDropPayload& Payload, const IntVector2& ScreenPosition);
    void ClearDragState();

    NODISCARD EKeyInterceptResult HandleRenameFieldKeyDown(const FKeyEvent& KeyEvent);

    TSharedPtr<FVisualElement> View;
    TSharedPtr<IFontFace>      Font;
    TSharedPtr<FEditableText>  RenameField;
    TSharedPtr<FEditableText>  RetiredRenameField;
    String                     PayloadId;
    FOnBrowserTargetBounds     TargetBounds;
    FOnBrowserTargetBounds     LabelBounds;
    FOnBrowserTargetLabel      TargetLabel;
    FOnBrowserHitTest          HitTest;
    FOnBrowserCanDrop          CanDrop;
    FOnBrowserContextMenu      OnContextMenu;
    FOnBrowserRenameCommitted  OnRenameCommitted;
    FOnBrowserRenameRequested  OnRenameRequested;
    FOnBrowserDeleteRequested  OnDeleteRequested;
    FOnBrowserDropped          OnDropped;
    FOnBrowserDragPreview      OnDragPreview;
    FOnBrowserHoverTip         HoverTip;
    int32                      RenamedTarget;
    int32                      DropTarget;
    int32                      HoveredTipTarget;
    bool                       bIsDragging;
    bool                       bIsCursorInsideView;
    bool                       bRestoreViewFocus;
};

TSharedPtr<FEditorContentBrowserView> FEditorContentBrowserView::Create(const FDesc& Desc)
{
    TSharedPtr<FEditorContentBrowserView> NewView = MakeSharedPtr<FEditorContentBrowserView>();
    NewView->Initialize(Desc);
    return NewView;
}

FEditorContentBrowserView::FEditorContentBrowserView()
    : FVisualElement()
    , View(nullptr)
    , Font(nullptr)
    , RenameField(nullptr)
    , RetiredRenameField(nullptr)
    , PayloadId()
    , TargetBounds()
    , LabelBounds()
    , TargetLabel()
    , HitTest()
    , CanDrop()
    , OnContextMenu()
    , OnRenameCommitted()
    , OnRenameRequested()
    , OnDeleteRequested()
    , OnDropped()
    , OnDragPreview()
    , HoverTip()
    , RenamedTarget(InvalidTarget)
    , DropTarget(InvalidTarget)
    , HoveredTipTarget(InvalidTarget)
    , bIsDragging(false)
    , bIsCursorInsideView(false)
    , bRestoreViewFocus(false)
{
}

FEditorContentBrowserView::~FEditorContentBrowserView() = default;

void FEditorContentBrowserView::Initialize(const FDesc& Desc)
{
    View              = Desc.View;
    Font              = Desc.Font;
    PayloadId         = Desc.PayloadId;
    TargetBounds      = Desc.TargetBounds;
    LabelBounds       = Desc.LabelBounds;
    TargetLabel       = Desc.TargetLabel;
    HitTest           = Desc.HitTest;
    CanDrop           = Desc.CanDrop;
    OnContextMenu     = Desc.OnContextMenu;
    OnRenameCommitted = Desc.OnRenameCommitted;
    OnRenameRequested = Desc.OnRenameRequested;
    OnDeleteRequested = Desc.OnDeleteRequested;
    OnDropped         = Desc.OnDropped;
    OnDragPreview     = Desc.OnDragPreview;
    HoverTip          = Desc.HoverTip;

    if (View)
    {
        View->SetParentElement(AsWeakPtr());
    }

    FOnDragOver OnOver = FOnDragOver::CreateLambda([](const FDragDropPayload& Payload)
    {
        return Payload.TypeId == GItemDragDropPayloadId || Payload.TypeId == GFolderDragDropPayloadId;
    });

    FDragDropService::Get().RegisterTarget(AsSharedPtr(), FOnDragDropped::CreateRaw(this, &FEditorContentBrowserView::HandleDrop), OnOver);
}

IntVector2 FEditorContentBrowserView::ComputeDesiredSize() const
{
    return View ? View->GetCachedDesiredSize() : IntVector2(0, 0);
}

void FEditorContentBrowserView::OnArrange(const FRectangle& AllottedBounds)
{
    RetiredRenameField.Reset();

    if (bRestoreViewFocus)
    {
        bRestoreViewFocus = false;

        if (View && FApplication::IsInitialized())
        {
            FApplication::Get().SetFocusElement(View);
        }
    }

    if (RenameField && !RenameField->HasKeyboardFocus())
    {
        CommitRename(RenameField->GetText());
    }

    if (View)
    {
        View->Tick(AllottedBounds);
    }

    if (RenameField)
    {
        const FRectangle FieldBounds = LabelBounds.IsBound() ? LabelBounds.Execute(RenamedTarget) : FRectangle();
        if (FieldBounds.IsEmpty())
        {
            CancelRename();
        }
        else
        {
            RenameField->Tick(FieldBounds);
        }
    }
}

void FEditorContentBrowserView::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (View)
    {
        OutChildren.Add(View);
    }

    if (RenameField)
    {
        OutChildren.Add(RenameField);
    }
}

int32 FEditorContentBrowserView::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    int32 MaxLayerId = LayerId;

    if (View)
    {
        const FDrawGeometry ViewGeometry(View->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = View->OnDraw(ViewGeometry, OutCommandList, LayerId + 1);
    }

    MaxLayerId += 1;
    OutCommandList.PushClip(MaxLayerId, AllottedGeometry.Bounds);

    if (bIsDragging && bIsCursorInsideView)
    {
        const FRectangle Bounds = (DropTarget != InvalidTarget && TargetBounds.IsBound())
            ? TargetBounds.Execute(DropTarget)
            : AllottedGeometry.Bounds;

        if (!Bounds.IsEmpty())
        {
            const bool bIsAllowed = !CanDrop.IsBound() || CanDrop.Execute(DropTarget);
            OutCommandList.AddBoxOutline(MaxLayerId, Bounds, bIsAllowed ? Style.Colors.Accent : Style.Colors.TextDisabled, DROP_INDICATOR_THICKNESS);
        }
    }

    if (RenameField)
    {
        const FRectangle FieldBounds = RenameField->GetContentRectangle();
        if (!FieldBounds.IsEmpty())
        {
            const FCornerRadii Radii(Style.Metrics.CornerRadius);

            MaxLayerId += 1;
            OutCommandList.AddBox(MaxLayerId, FieldBounds, Style.Colors.WindowBackground, Radii);
            OutCommandList.AddBoxOutline(MaxLayerId, FieldBounds, Style.Colors.Accent, Style.Metrics.BorderThickness, Radii);

            const FDrawGeometry FieldGeometry(FieldBounds, AllottedGeometry.Scale);
            MaxLayerId = RenameField->OnDraw(FieldGeometry, OutCommandList, MaxLayerId + 1);
        }
    }

    OutCommandList.PopClip(MaxLayerId);
    return MaxLayerId;
}

void FEditorContentBrowserView::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (RenameField && RenameField->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        RenameField->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        return;
    }

    if (View)
    {
        View->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

FEventResponse FEditorContentBrowserView::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonRight)
    {
        return FEventResponse::Unhandled();
    }

    if (IsRenaming())
    {
        CancelRename();
    }

    const int32 Target = HitTest.IsBound() ? HitTest.Execute(CursorEvent.GetClientPosition()) : InvalidTarget;

    OnContextMenu.ExecuteIfBound(Target, CursorEvent.GetScreenPosition());
    return FEventResponse::Handled();
}

FEventResponse FEditorContentBrowserView::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bIsDragging)
    {
        const int32 Target = HitTest.IsBound() ? HitTest.Execute(CursorEvent.GetClientPosition()) : InvalidTarget;
        if (Target == HoveredTipTarget)
        {
            return FEventResponse::Unhandled();
        }

        FToolTipService::Get().CancelToolTip(AsSharedPtr());
        HoveredTipTarget = InvalidTarget;

        if (HoverTip.IsBound() && Target != InvalidTarget && !FDragDropService::Get().IsDragging())
        {
            const String Tip = HoverTip.Execute(Target);
            if (!Tip.IsEmpty())
            {
                HoveredTipTarget = Target;
                FToolTipService::Get().RequestTextToolTip(AsSharedPtr(), Tip, FEditorStyle::GetFonts().Body);
            }
        }

        return FEventResponse::Unhandled();
    }

    const IntVector2 ScreenPosition = CursorEvent.GetScreenPosition();
    const IntVector2 ClientPosition = ScreenToClient(AsSharedPtr(), ScreenPosition);

    DropTarget          = HitTest.IsBound() ? HitTest.Execute(ClientPosition) : InvalidTarget;
    bIsCursorInsideView = GetContentRectangle().EncapsulatesPoint(ClientPosition);

    FDragDropService::Get().UpdateDrag(ScreenPosition);
    OnDragPreview.ExecuteIfBound(DropTarget);

    return FEventResponse::Handled();
}

FEventResponse FEditorContentBrowserView::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    HoveredTipTarget = InvalidTarget;
    FToolTipService::Get().CancelToolTip(AsSharedPtr());
    return FEventResponse::Unhandled();
}

FEventResponse FEditorContentBrowserView::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!bIsDragging || CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    FDragDropService::Get().EndDrag(CursorEvent.GetScreenPosition());

    ClearDragState();
    return FEventResponse::Handled();
}

FEventResponse FEditorContentBrowserView::OnKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    if (Key == Keys::Escape && bIsDragging)
    {
        if (FApplication::IsInitialized())
        {
            FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
        }

        FDragDropService::Get().CancelDrag();

        ClearDragState();
        return FEventResponse::Handled();
    }

    if (Key == Keys::F2)
    {
        OnRenameRequested.ExecuteIfBound();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Delete)
    {
        OnDeleteRequested.ExecuteIfBound();
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

void FEditorContentBrowserView::BeginRename(int32 Target)
{
    if (Target == InvalidTarget || !LabelBounds.IsBound() || !FApplication::IsInitialized())
    {
        return;
    }

    CancelRename();

    if (LabelBounds.Execute(Target).IsEmpty())
    {
        return;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    FEditableText::FDesc Desc;
    Desc.Text            = TargetLabel.IsBound() ? TargetLabel.Execute(Target) : String();
    Desc.Font            = Font;
    Desc.ForegroundColor = Style.Colors.Text;
    Desc.SelectionColor  = Style.Colors.TextSelectionBackground;
    Desc.TextCursorColor = Style.Colors.Text;
    Desc.Padding         = FMargin(4, 0, 4, 0);

    RenameField = FEditableText::Create(Desc);
    if (!RenameField)
    {
        return;
    }

    RenamedTarget = Target;

    RenameField->SetParentElement(AsWeakPtr());
    RenameField->SelectAll();

    RenameField->GetOnTextCommitted()      = FOnTextCommittedDelegate::CreateRaw(this, &FEditorContentBrowserView::CommitRename);
    RenameField->GetOnKeyDownInterceptor() = FOnEditableTextKeyDownDelegate::CreateRaw(this, &FEditorContentBrowserView::HandleRenameFieldKeyDown);

    FApplication::Get().SetFocusElement(RenameField);
}

void FEditorContentBrowserView::BeginDrag(const String& DisplayText, const FUIBrush& Icon, int32 SelectionCount, const FCursorEvent& CursorEvent)
{
    if (IsRenaming() || !FApplication::IsInitialized())
    {
        return;
    }

    FDragDropPayload Payload;
    Payload.TypeId              = PayloadId;
    Payload.DisplayText         = DisplayText;
    Payload.Icon                = Icon;
    Payload.AllowedStatusIcon   = FEditorIcons::CircledCheckmark;
    Payload.ForbiddenStatusIcon = FEditorIcons::Forbidden;
    Payload.IconSize            = 64;
    Payload.SelectionCount      = Math::Max(SelectionCount, 1);

    bIsDragging         = true;
    bIsCursorInsideView = true;
    DropTarget          = InvalidTarget;
    HoveredTipTarget    = InvalidTarget;

    FApplication::Get().CaptureMouse(AsSharedPtr());
    FToolTipService::Get().CancelToolTip(AsSharedPtr());
    FDragDropService::Get().BeginDrag(Payload, CursorEvent.GetScreenPosition());
}

void FEditorContentBrowserView::CancelRename()
{
    ReleaseRenameField();
}

void FEditorContentBrowserView::CommitRename(const String& NewName)
{
    const int32 Target = RenamedTarget;

    ReleaseRenameField();

    OnRenameCommitted.ExecuteIfBound(Target, NewName);
}

void FEditorContentBrowserView::ReleaseRenameField()
{
    if (!RenameField)
    {
        return;
    }

    RetiredRenameField = RenameField;

    RenameField.Reset();
    RenamedTarget = InvalidTarget;

    bRestoreViewFocus = true;
}

EKeyInterceptResult FEditorContentBrowserView::HandleRenameFieldKeyDown(const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == Keys::Escape)
    {
        CancelRename();
        return EKeyInterceptResult::Handled;
    }

    return EKeyInterceptResult::NotHandled;
}

void FEditorContentBrowserView::HandleDrop(const FDragDropPayload& Payload, const IntVector2& ScreenPosition)
{
    const int32 Target = HitTest.IsBound() ? HitTest.Execute(ScreenToClient(AsSharedPtr(), ScreenPosition)) : InvalidTarget;
    if (CanDrop.IsBound() && !CanDrop.Execute(Target))
    {
        return;
    }

    OnDropped.ExecuteIfBound(Payload.TypeId, Target);
}

void FEditorContentBrowserView::ClearDragState()
{
    DropTarget          = InvalidTarget;
    HoveredTipTarget    = InvalidTarget;
    bIsDragging         = false;
    bIsCursorInsideView = false;
}

FEditorContentBrowserPanel::FEditorContentBrowserPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "ContentBrowser", "Content Browser")
    , Splitter(nullptr)
    , TreeWrapper(nullptr)
    , GridWrapper(nullptr)
    , FolderTree(nullptr)
    , TileView(nullptr)
    , FolderSearchBox(nullptr)
    , ContentSearchBox(nullptr)
    , BreadcrumbBar(nullptr)
    , EmptyStateText(nullptr)
    , Roots()
    , CurrentPath()
    , BackHistory()
    , ForwardHistory()
    , TreeItemPaths()
    , VisibleChildIndices()
    , DragParentPath()
    , DragChildIndices()
    , ClipboardParentPath()
    , ClipboardChildIndices()
    , FolderFilterText()
    , ContentFilterText()
{
}

FEditorContentBrowserPanel::~FEditorContentBrowserPanel()
{
}

bool FEditorContentBrowserPanel::Initialize()
{
    BuildPlaceholderTree();

    CurrentPath.Clear();
    CurrentPath.Add(0);

    TSharedPtr<FVisualElement> FolderColumn = BuildFolderColumn();
    if (!FolderColumn)
    {
        return false;
    }

    TSharedPtr<FVisualElement> ContentColumn = BuildContentColumn();
    if (!ContentColumn)
    {
        return false;
    }

    FSplitter::FDesc SplitterDesc;
    SplitterDesc.Orientation = EDockSplitOrientation::Horizontal;
    SplitterDesc.Fractions.Add(0.3f);
    SplitterDesc.Fractions.Add(0.7f);
    SplitterDesc.FixedLengths.Add(FOLDER_PANEL_WIDTH);
    SplitterDesc.FixedLengths.Add(0);

    Splitter = FSplitter::Create(SplitterDesc);
    if (!Splitter)
    {
        return false;
    }

    Splitter->AddChild(FolderColumn, IntVector2(FOLDER_PANEL_MIN, 0));
    Splitter->AddChild(ContentColumn, IntVector2(CONTENT_PANEL_MIN, 0));

    Content = Splitter;

    RebuildTree();
    RefreshTiles();
    RefreshBreadcrumbs();
    return true;
}

TSharedPtr<FVisualElement> FEditorContentBrowserPanel::BuildFolderColumn()
{
    FolderSearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Paths",
        FOnSearchTextChanged::CreateRaw(this, &FEditorContentBrowserPanel::OnFolderSearchTextChanged)));

    if (!FolderSearchBox)
    {
        return nullptr;
    }

    FTreeView::FDesc TreeDesc;
    TreeDesc.Font                = FEditorStyle::GetFonts().Body;
    TreeDesc.RowHeight           = FOLDER_ROW_HEIGHT;
    TreeDesc.bShowScrollBar      = true;
    TreeDesc.bAlternateRowColors = true;
    TreeDesc.bAllowMultiSelect   = true;
    TreeDesc.bHighlightAncestors = true;
    TreeDesc.OnSelectionChanged  = FOnTreeSelectionChanged::CreateRaw(this, &FEditorContentBrowserPanel::OnFolderSelectionChanged);
    TreeDesc.OnDragDetected      = FOnTreeItemDragDetected::CreateRaw(this, &FEditorContentBrowserPanel::OnFolderDragDetected);

    FEditorStyle::ApplyTreeViewArrows(TreeDesc);

    FolderTree = FTreeView::Create(TreeDesc);
    if (!FolderTree)
    {
        return nullptr;
    }

    FEditorContentBrowserView::FDesc WrapperDesc;
    WrapperDesc.View              = FolderTree;
    WrapperDesc.Font              = FEditorStyle::GetFonts().Body;
    WrapperDesc.PayloadId         = GFolderDragDropPayloadId;
    WrapperDesc.TargetBounds      = FOnBrowserTargetBounds::CreateRaw(this, &FEditorContentBrowserPanel::GetTreeRowBounds);
    WrapperDesc.LabelBounds       = FOnBrowserTargetBounds::CreateRaw(this, &FEditorContentBrowserPanel::GetTreeLabelBounds);
    WrapperDesc.TargetLabel       = FOnBrowserTargetLabel::CreateRaw(this, &FEditorContentBrowserPanel::GetTreeLabel);
    WrapperDesc.HitTest           = FOnBrowserHitTest::CreateRaw(this, &FEditorContentBrowserPanel::HitTestTree);
    WrapperDesc.CanDrop           = FOnBrowserCanDrop::CreateRaw(this, &FEditorContentBrowserPanel::CanDropOnTreeRow);
    WrapperDesc.OnContextMenu     = FOnBrowserContextMenu::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeContextMenu);
    WrapperDesc.OnRenameCommitted = FOnBrowserRenameCommitted::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeRenameCommitted);
    WrapperDesc.OnRenameRequested = FOnBrowserRenameRequested::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeRenameRequested);
    WrapperDesc.OnDeleteRequested = FOnBrowserDeleteRequested::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeDeleteRequested);
    WrapperDesc.OnDropped         = FOnBrowserDropped::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeDropped);
    WrapperDesc.OnDragPreview     = FOnBrowserDragPreview::CreateRaw(this, &FEditorContentBrowserPanel::UpdateTreeDragPreview);
    WrapperDesc.HoverTip          = FOnBrowserHoverTip::CreateRaw(this, &FEditorContentBrowserPanel::GetTreeHoverTip);

    TreeWrapper = FEditorContentBrowserView::Create(WrapperDesc);
    if (!TreeWrapper)
    {
        return nullptr;
    }

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FolderSearchBox).SetPadding(FMargin(0, 0, 0, COLUMN_PADDING));
    Column->AddSlot(TreeWrapper).SetFillCoefficient(1.0f);

    return FEditorStyle::MakeInnerFrame(Column);
}

TSharedPtr<FVisualElement> FEditorContentBrowserPanel::BuildContentColumn()
{
    TSharedPtr<FVisualElement> Breadcrumbs = BuildBreadcrumbBar();
    if (!Breadcrumbs)
    {
        return nullptr;
    }

    ContentSearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Content",
        FOnSearchTextChanged::CreateRaw(this, &FEditorContentBrowserPanel::OnContentSearchTextChanged)));

    if (!ContentSearchBox)
    {
        return nullptr;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    FTileView::FDesc TileDesc;
    TileDesc.Font            = FEditorStyle::GetFonts().Body;
    TileDesc.TileSize        = IntVector2(TILE_WIDTH, TILE_HEIGHT);
    TileDesc.TileSpacing     = TILE_SPACING;
    TileDesc.IconSize        = TILE_ICON_SIZE;
    TileDesc.LabelInset      = TILE_LABEL_INSET;
    TileDesc.CornerRadius    = TILE_CORNER;
    TileDesc.IdleFill        = Style.Colors.WindowBackground;
    TileDesc.HoveredFill     = Style.Colors.ControlHovered;
    TileDesc.SelectedFill    = Style.Colors.Accent;
    TileDesc.OnItemActivated = FOnTileActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnTileActivated);
    TileDesc.OnDragDetected  = FOnTileDragDetected::CreateRaw(this, &FEditorContentBrowserPanel::OnTileDragDetected);

    TileView = FTileView::Create(TileDesc);
    if (!TileView)
    {
        return nullptr;
    }

    FEditorContentBrowserView::FDesc WrapperDesc;
    WrapperDesc.View              = TileView;
    WrapperDesc.Font              = FEditorStyle::GetFonts().Body;
    WrapperDesc.PayloadId         = GItemDragDropPayloadId;
    WrapperDesc.TargetBounds      = FOnBrowserTargetBounds::CreateRaw(this, &FEditorContentBrowserPanel::GetTileBounds);
    WrapperDesc.LabelBounds       = FOnBrowserTargetBounds::CreateRaw(this, &FEditorContentBrowserPanel::GetTileLabelBounds);
    WrapperDesc.TargetLabel       = FOnBrowserTargetLabel::CreateRaw(this, &FEditorContentBrowserPanel::GetTileLabel);
    WrapperDesc.HitTest           = FOnBrowserHitTest::CreateRaw(this, &FEditorContentBrowserPanel::HitTestGrid);
    WrapperDesc.CanDrop           = FOnBrowserCanDrop::CreateRaw(this, &FEditorContentBrowserPanel::CanDropOnTile);
    WrapperDesc.OnContextMenu     = FOnBrowserContextMenu::CreateRaw(this, &FEditorContentBrowserPanel::OnGridContextMenu);
    WrapperDesc.OnRenameCommitted = FOnBrowserRenameCommitted::CreateRaw(this, &FEditorContentBrowserPanel::OnGridRenameCommitted);
    WrapperDesc.OnRenameRequested = FOnBrowserRenameRequested::CreateRaw(this, &FEditorContentBrowserPanel::OnGridRenameRequested);
    WrapperDesc.OnDeleteRequested = FOnBrowserDeleteRequested::CreateRaw(this, &FEditorContentBrowserPanel::OnGridDeleteRequested);
    WrapperDesc.OnDropped         = FOnBrowserDropped::CreateRaw(this, &FEditorContentBrowserPanel::OnGridDropped);
    WrapperDesc.OnDragPreview     = FOnBrowserDragPreview::CreateRaw(this, &FEditorContentBrowserPanel::UpdateGridDragPreview);
    WrapperDesc.HoverTip          = FOnBrowserHoverTip::CreateRaw(this, &FEditorContentBrowserPanel::GetGridHoverTip);

    GridWrapper = FEditorContentBrowserView::Create(WrapperDesc);
    if (!GridWrapper)
    {
        return nullptr;
    }

    FTextBlock::FDesc EmptyDesc;
    EmptyDesc.Font            = FEditorStyle::GetFonts().Body;
    EmptyDesc.ColorAndOpacity = Style.Colors.TextDisabled;
    EmptyDesc.Text            = "Folder is empty";

    EmptyStateText = FTextBlock::Create(EmptyDesc);
    if (!EmptyStateText)
    {
        return nullptr;
    }

    TSharedPtr<FOverlay> GridArea = FOverlay::Create();
    GridArea->AddSlot(GridWrapper).SetPadding(FMargin(TILE_VIEW_INSET));
    GridArea->AddSlot(EmptyStateText).SetHorizontalAlignment(EHorizontalAlignment::Center).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FFractionWidthBox> SearchBand = FFractionWidthBox::Create(ContentSearchBox, CONTENT_SEARCH_FRACTION, CONTENT_SEARCH_MIN);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Breadcrumbs).SetPadding(FMargin(0, 0, 0, COLUMN_PADDING));
    Column->AddSlot(SearchBand).SetPadding(FMargin(0, 0, 0, COLUMN_PADDING));
    Column->AddSlot(FEditorStyle::MakeInnerFrame(GridArea)).SetFillCoefficient(1.0f);

    return Column;
}

TSharedPtr<FVisualElement> FEditorContentBrowserPanel::BuildBreadcrumbBar()
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FButton::FDesc BackDesc;
    BackDesc.Font      = FEditorStyle::GetFonts().Body;
    BackDesc.Padding   = FMargin(4);
    BackDesc.MinHeight = BREADCRUMB_NAV_SIZE;
    BackDesc.bIsGhost  = true;
    BackDesc.Content   = FBrowserGlyph::Create(FEditorIcons::Previous, BREADCRUMB_NAV_ICON_SIZE, Style.Colors.Text);
    BackDesc.OnClicked = FOnClicked::CreateRaw(this, &FEditorContentBrowserPanel::NavigateBack);

    FButton::FDesc ForwardDesc;
    ForwardDesc.Font      = FEditorStyle::GetFonts().Body;
    ForwardDesc.Padding   = FMargin(4);
    ForwardDesc.MinHeight = BREADCRUMB_NAV_SIZE;
    ForwardDesc.bIsGhost  = true;
    ForwardDesc.Content   = FBrowserGlyph::Create(FEditorIcons::Next, BREADCRUMB_NAV_ICON_SIZE, Style.Colors.Text);
    ForwardDesc.OnClicked = FOnClicked::CreateRaw(this, &FEditorContentBrowserPanel::NavigateForward);

    TSharedPtr<FButton> BackButton    = FButton::Create(BackDesc);
    TSharedPtr<FButton> ForwardButton = FButton::Create(ForwardDesc);

    if (!BackButton || !ForwardButton)
    {
        return nullptr;
    }

    BreadcrumbBar = FHorizontalBox::Create();
    if (!BreadcrumbBar)
    {
        return nullptr;
    }

    FBorder::FDesc FieldDesc;
    FieldDesc.Content         = BreadcrumbBar;
    FieldDesc.BackgroundColor = Style.Colors.InputFieldFill;
    FieldDesc.BorderColor     = Style.Colors.InputFieldBorder;
    FieldDesc.BorderThickness = Style.Metrics.BorderThickness;
    FieldDesc.CornerRadius    = FCornerRadii(BREADCRUMB_FIELD_CORNER);
    FieldDesc.Padding         = FMargin(BREADCRUMB_FIELD_PADDING);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(BackButton).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(BREADCRUMB_NAV_GAP));
    Row->AddSlot(ForwardButton).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::CreateHorizontal(BREADCRUMB_NAV_GAP));
    Row->AddSlot(FBorder::Create(FieldDesc)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);

    FBorder::FDesc HeaderDesc;
    HeaderDesc.Content         = Row;
    HeaderDesc.BackgroundColor = Style.Header.Fill;
    HeaderDesc.Padding         = FMargin(0, BREADCRUMB_BAR_PADDING, 0, BREADCRUMB_BAR_PADDING);
    HeaderDesc.MinHeight       = FOLDER_HEADER_HEIGHT;

    return FBorder::Create(HeaderDesc);
}

void FEditorContentBrowserPanel::BuildPlaceholderTree()
{
    static const CHAR* const CategoryNames[] =
    {
        "MyOtherContent",
        "Materials",
        "Geometry",
        "Textures",
        "Scenes",
    };

    static const CHAR* const EvenSubFolders[] = { "Meshes", "Materials", "Textures" };
    static const CHAR* const EvenAssets[]     = { "Car.asset", "Door.asset", "Wood.asset", "Stone.asset", "Gold.asset" };

    static const CHAR* const OddSubFolders[] = { "Animations", "Shaders", "Icons" };
    static const CHAR* const OddAssets[]     = { "Bus.asset", "Train.asset", "Metal.asset", "Lava.asset", "Silver.asset", "WalkAnimation.asset", "JumpAnimation.asset" };

    static const CHAR* const LeafAssets[] =
    {
        "WalkAnimation.asset",
        "JumpAnimation.asset",
        "LavaTexture.asset",
        "GoldTexture.asset",
        "SpaceshipModel.asset",
    };

    const auto CreateEntry = [](const CHAR* Name, bool bIsFolder)
    {
        FEntry Entry;
        Entry.Name      = Name;
        Entry.bIsFolder = bIsFolder;
        return Entry;
    };

    Roots.Clear();

    static const CHAR* const RootNames[] = { "Content", "MoreContent" };
    for (const CHAR* RootName : RootNames)
    {
        FEntry Root = CreateEntry(RootName, true);

        for (int32 CategoryIndex = 0; CategoryIndex < int32(ARRAY_COUNT(CategoryNames)); ++CategoryIndex)
        {
            FEntry Category = CreateEntry(CategoryNames[CategoryIndex], true);

            const bool         bIsEven        = (CategoryIndex % 2) == 0;
            const CHAR* const* SubFolderNames = bIsEven ? EvenSubFolders : OddSubFolders;
            const int32        NumSubFolders  = int32(bIsEven ? ARRAY_COUNT(EvenSubFolders) : ARRAY_COUNT(OddSubFolders));

            for (int32 SubIndex = 0; SubIndex < NumSubFolders; ++SubIndex)
            {
                FEntry SubFolder = CreateEntry(SubFolderNames[SubIndex], true);
                if ((SubIndex % 2) == 0)
                {
                    for (const CHAR* AssetName : LeafAssets)
                    {
                        SubFolder.Children.Emplace(CreateEntry(AssetName, false));
                    }
                }

                Category.Children.Emplace(Move(SubFolder));
            }

            const CHAR* const* AssetNames = bIsEven ? EvenAssets : OddAssets;
            const int32        NumAssets   = int32(bIsEven ? ARRAY_COUNT(EvenAssets) : ARRAY_COUNT(OddAssets));

            for (int32 AssetIndex = 0; AssetIndex < NumAssets; ++AssetIndex)
            {
                Category.Children.Emplace(CreateEntry(AssetNames[AssetIndex], false));
            }

            Root.Children.Emplace(Move(Category));
        }

        Roots.Emplace(Move(Root));
    }
}

void FEditorContentBrowserPanel::Release()
{
    if (TreeWrapper)
    {
        FDragDropService::Get().UnregisterTarget(TreeWrapper);
    }

    if (GridWrapper)
    {
        FDragDropService::Get().UnregisterTarget(GridWrapper);
    }

    Splitter.Reset();
    TreeWrapper.Reset();
    GridWrapper.Reset();
    FolderTree.Reset();
    TileView.Reset();
    FolderSearchBox.Reset();
    ContentSearchBox.Reset();
    BreadcrumbBar.Reset();
    EmptyStateText.Reset();

    Roots.Clear();
    CurrentPath.Clear();
    BackHistory.Clear();
    ForwardHistory.Clear();
    TreeItemPaths.Clear();
    VisibleChildIndices.Clear();

    FEditorPanel::Release();
}

void FEditorContentBrowserPanel::RebuildTree()
{
    TreeItemPaths.Clear();

    FEntryPath Path;

    TArray<TSharedPtr<FTreeItem>> TreeRoots;

    struct FBuilder
    {
        TMap<const FTreeItem*, FEntryPath>* Paths;

        TSharedPtr<FTreeItem> Build(FEntry& Entry, FEntryPath& InPath) const
        {
            TSharedPtr<FTreeItem> Item = FTreeItem::Create(Entry.Name);
            Item->Icon         = FEditorIcons::FolderSmall;
            Item->ExpandedIcon = FEditorIcons::FolderOpenSmall;
            Item->bIsExpanded  = true;

            Paths->Add(Item.Get(), InPath);

            for (int32 Index = 0; Index < Entry.Children.Size(); ++Index)
            {
                if (!Entry.Children[Index].bIsFolder)
                {
                    continue;
                }

                InPath.Add(Index);
                Item->AddChild(Build(Entry.Children[Index], InPath));
                InPath.Pop();
            }

            return Item;
        }
    };

    const FBuilder Builder{ &TreeItemPaths };
    for (int32 RootIndex = 0; RootIndex < Roots.Size(); ++RootIndex)
    {
        Path.Clear();
        Path.Add(RootIndex);

        TreeRoots.Emplace(Builder.Build(Roots[RootIndex], Path));
    }

    FolderTree->SetRootItems(TreeRoots);

    if (TSharedPtr<FTreeItem> CurrentItem = FindTreeItem(CurrentPath))
    {
        TArray<TSharedPtr<FTreeItem>> Selection;
        Selection.Emplace(CurrentItem);

        FolderTree->SetSelection(Selection);
    }
}

void FEditorContentBrowserPanel::RefreshTiles()
{
    TArray<FTileItem> Items;
    VisibleChildIndices.Clear();

    FEntry* Folder = FindEntry(CurrentPath);
    if (Folder)
    {
        for (int32 Index = 0; Index < Folder->Children.Size(); ++Index)
        {
            const FEntry& Child = Folder->Children[Index];
            if (!ContentFilterText.IsEmpty() && !Child.Name.Contains(ContentFilterText, EStringCaseType::NoCase))
            {
                continue;
            }

            FTileItem Item;
            Item.Label = Child.Name;
            Item.Icon  = Child.bIsFolder ? FEditorIcons::Folder : FEditorIcons::Document;

            Items.Emplace(Item);
            VisibleChildIndices.Add(Index);
        }
    }

    TileView->SetFilterText(ContentFilterText);
    TileView->SetItems(Items);

    if (EmptyStateText)
    {
        const CHAR* Message = !Folder ? "No folder selected" : (Folder->Children.IsEmpty() ? "Folder is empty" : "No results");

        EmptyStateText->SetText(Message);
        EmptyStateText->SetVisibility(Items.IsEmpty() ? EVisibility::Visible : EVisibility::Hidden);
    }
}

void FEditorContentBrowserPanel::RefreshBreadcrumbs()
{
    if (!BreadcrumbBar)
    {
        return;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    BreadcrumbBar->ClearSlots();

    const auto AddCrumb = [&](const String& CrumbLabel, const FEntryPath& Path)
    {
        FButton::FDesc Desc;
        Desc.SetText(CrumbLabel).SetFont(FEditorStyle::GetFonts().Body);
        Desc.Padding   = FMargin(8, 2, 8, 2);
        Desc.bIsGhost  = true;
        Desc.OnClicked = FOnClicked::CreateLambda([this, Path]()
        {
            NavigateTo(Path, true);
        });

        BreadcrumbBar->AddSlot(FButton::Create(Desc)).SetVerticalAlignment(EVerticalAlignment::Center);
    };

    AddCrumb("Root", FEntryPath());

    FEntryPath Prefix;
    for (int32 Depth = 0; Depth < CurrentPath.Size(); ++Depth)
    {
        Prefix.Add(CurrentPath[Depth]);

        FEntry* Entry = FindEntry(Prefix);
        if (!Entry)
        {
            break;
        }

        BreadcrumbBar->AddSlot(FBrowserGlyph::Create(FEditorIcons::RightArrow, BREADCRUMB_SEPARATOR_SIZE, Style.Colors.TextDisabled))
            .SetPadding(FMargin(BREADCRUMB_GAP / 2, 0, BREADCRUMB_GAP / 2, 0))
            .SetVerticalAlignment(EVerticalAlignment::Center);

        AddCrumb(Entry->Name, Prefix);
    }

    BreadcrumbBar->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
}

void FEditorContentBrowserPanel::NavigateTo(const FEntryPath& NewPath, bool bAddToHistory)
{
    if (NewPath == CurrentPath)
    {
        return;
    }

    if (bAddToHistory)
    {
        BackHistory.Emplace(CurrentPath);
        ForwardHistory.Clear();
    }

    CurrentPath = NewPath;

    if (TreeWrapper)
    {
        TreeWrapper->CancelRename();
    }

    if (GridWrapper)
    {
        GridWrapper->CancelRename();
    }

    if (TSharedPtr<FTreeItem> CurrentItem = FindTreeItem(CurrentPath))
    {
        TArray<TSharedPtr<FTreeItem>> Selection;
        Selection.Emplace(CurrentItem);

        FolderTree->SetSelection(Selection);
        FolderTree->ScrollToItem(CurrentItem);
    }
    else
    {
        FolderTree->ClearSelection();
    }

    RefreshTiles();
    RefreshBreadcrumbs();
}

void FEditorContentBrowserPanel::NavigateBack()
{
    if (BackHistory.IsEmpty())
    {
        return;
    }

    const FEntryPath Previous = BackHistory.Last();
    BackHistory.Pop();

    ForwardHistory.Emplace(CurrentPath);

    NavigateTo(Previous, false);
}

void FEditorContentBrowserPanel::NavigateForward()
{
    if (ForwardHistory.IsEmpty())
    {
        return;
    }

    const FEntryPath Next = ForwardHistory.Last();
    ForwardHistory.Pop();

    BackHistory.Emplace(CurrentPath);

    NavigateTo(Next, false);
}

const FEditorContentBrowserPanel::FEntry* FEditorContentBrowserPanel::FindEntry(const FEntryPath& Path) const
{
    if (Path.IsEmpty() || !Roots.IsValidIndex(Path[0]))
    {
        return nullptr;
    }

    const FEntry* Entry = &Roots[Path[0]];
    for (int32 Depth = 1; Depth < Path.Size(); ++Depth)
    {
        if (!Entry->Children.IsValidIndex(Path[Depth]))
        {
            return nullptr;
        }

        Entry = &Entry->Children[Path[Depth]];
    }

    return Entry;
}

FEditorContentBrowserPanel::FEntry* FEditorContentBrowserPanel::FindEntry(const FEntryPath& Path)
{
    const FEntry* Entry = const_cast<const FEditorContentBrowserPanel*>(this)->FindEntry(Path);
    return const_cast<FEntry*>(Entry);
}

TArray<FEditorContentBrowserPanel::FEntry>* FEditorContentBrowserPanel::FindChildArray(const FEntryPath& ParentPath)
{
    if (ParentPath.IsEmpty())
    {
        return &Roots;
    }

    FEntry* Entry = FindEntry(ParentPath);
    return Entry ? &Entry->Children : nullptr;
}

FEditorContentBrowserPanel::FEntryPath FEditorContentBrowserPanel::FindTreeItemPath(const TSharedPtr<FTreeItem>& Item) const
{
    if (const FEntryPath* Path = Item ? TreeItemPaths.Find(Item.Get()) : nullptr)
    {
        return *Path;
    }

    return FEntryPath();
}

TSharedPtr<FTreeItem> FEditorContentBrowserPanel::FindTreeItem(const FEntryPath& Path) const
{
    if (!FolderTree || Path.IsEmpty())
    {
        return nullptr;
    }

    for (const TSharedPtr<FTreeItem>& Item : FolderTree->GetVisibleRows())
    {
        if (const FEntryPath* ItemPath = TreeItemPaths.Find(Item.Get()))
        {
            if (*ItemPath == Path)
            {
                return Item;
            }
        }
    }

    return nullptr;
}

FEditorContentBrowserPanel::FEntryPath FEditorContentBrowserPanel::ResolveTileTarget(int32 TileIndex) const
{
    if (!VisibleChildIndices.IsValidIndex(TileIndex))
    {
        return CurrentPath;
    }

    FEntryPath Path = CurrentPath;
    Path.Add(VisibleChildIndices[TileIndex]);
    return Path;
}

void FEditorContentBrowserPanel::NewFolder()
{
    TArray<FEntry>* Children = FindChildArray(CurrentPath);
    if (!Children)
    {
        return;
    }

    FEntry Folder;
    Folder.Name      = CreateUniqueName(*Children, GNewFolderName);
    Folder.bIsFolder = true;

    Children->Emplace(Move(Folder));

    const int32 NewChildIndex = Children->Size() - 1;

    ContentFilterText.Clear();

    if (ContentSearchBox)
    {
        ContentSearchBox->SetText(String());
    }

    RebuildTree();
    RefreshTiles();

    const int32 TileIndex = VisibleChildIndices.Find(NewChildIndex);
    if (TileIndex != TArray<int32>::InvalidIndex)
    {
        TileView->SetSelection(TileIndex);
        TileView->ScrollToTile(TileIndex);

        GridWrapper->BeginRename(TileIndex);
    }
}

void FEditorContentBrowserPanel::RenameEntry(const FEntryPath& Path, const String& NewName)
{
    FEntry* Entry = FindEntry(Path);
    if (!Entry || NewName.IsEmpty())
    {
        return;
    }

    if (Entry->bIsFolder)
    {
        Entry->Name = NewName;
    }
    else
    {
        const int32 Dot = Entry->Name.FindLastChar('.');
        Entry->Name = (Dot != String::InvalidIndex) 
            ? (NewName + Entry->Name.SubString(Dot, Entry->Name.Length() - Dot)) 
            : NewName;
    }

    RebuildTree();
    RefreshTiles();
    RefreshBreadcrumbs();
}

void FEditorContentBrowserPanel::DeleteEntries(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices)
{
    TArray<FEntry>* Children = FindChildArray(ParentPath);
    if (!Children || ChildIndices.IsEmpty())
    {
        return;
    }

    TArray<int32> Sorted = ChildIndices;
    Algorithm::Sort(Sorted, [](int32 Left, int32 Right) { return Left > Right; });

    for (const int32 ChildIndex : Sorted)
    {
        if (Children->IsValidIndex(ChildIndex))
        {
            Children->RemoveAt(ChildIndex);
        }
    }

    while (!CurrentPath.IsEmpty() && !FindEntry(CurrentPath))
    {
        CurrentPath.Pop();
    }

    ClipboardParentPath.Clear();
    ClipboardChildIndices.Clear();

    RebuildTree();
    RefreshTiles();
    RefreshBreadcrumbs();
}

String FEditorContentBrowserPanel::CreateUniqueName(const TArray<FEntry>& Siblings, const String& Name)
{
    const auto IsTaken = [&Siblings](const String& Candidate)
    {
        for (const FEntry& Sibling : Siblings)
        {
            if (Sibling.Name.Equals(Candidate, EStringCaseType::NoCase))
            {
                return true;
            }
        }

        return false;
    };

    if (!IsTaken(Name))
    {
        return Name;
    }

    const int32  Dot       = Name.FindLastChar('.');
    const String Stem      = (Dot != String::InvalidIndex) ? Name.SubString(0, Dot) : Name;
    const String Extension = (Dot != String::InvalidIndex) ? Name.SubString(Dot, Name.Length() - Dot) : String();

    for (int32 Suffix = 1;; ++Suffix)
    {
        const String Candidate = String::Printf("%s (%d)%s", Stem.Data(), Suffix, Extension.Data());
        if (!IsTaken(Candidate))
        {
            return Candidate;
        }
    }
}

void FEditorContentBrowserPanel::CopyEntries(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices)
{
    if (ChildIndices.IsEmpty() || !FindChildArray(ParentPath))
    {
        return;
    }

    ClipboardParentPath   = ParentPath;
    ClipboardChildIndices = ChildIndices;
}

void FEditorContentBrowserPanel::PasteEntries()
{
    TArray<FEntry>* Source = FindChildArray(ClipboardParentPath);
    TArray<FEntry>* Target = FindChildArray(CurrentPath);

    if (!Source || !Target || ClipboardChildIndices.IsEmpty())
    {
        return;
    }

    TArray<FEntry> Copies;
    for (const int32 ChildIndex : ClipboardChildIndices)
    {
        if (Source->IsValidIndex(ChildIndex))
        {
            Copies.Emplace((*Source)[ChildIndex]);
        }
    }

    for (FEntry& Copy : Copies)
    {
        Copy.Name = CreateUniqueName(*Target, Copy.Name);
        Target->Emplace(Move(Copy));
    }

    RebuildTree();
    RefreshTiles();
}

void FEditorContentBrowserPanel::MoveEntries(const FEntryPath& SourceParentPath, const TArray<int32>& ChildIndices, const FEntryPath& TargetPath)
{
    TArray<int32> Legal;
    TArray<int32> Illegal;
    FIndexedPathMove::Classify(SourceParentPath, ChildIndices, TargetPath, Legal, Illegal);

    if (Legal.IsEmpty())
    {
        return;
    }

    TArray<FEntry>* Source = FindChildArray(SourceParentPath);
    if (!Source)
    {
        ReportFailure("Could not find the folder these items came from", CollectEntryNames(SourceParentPath, Legal));
        return;
    }

    TArray<FEntry> Moved;
    TArray<int32>  Sorted = Legal;
    Algorithm::Sort(Sorted, [](int32 Left, int32 Right) { return Left > Right; });

    for (const int32 ChildIndex : Sorted)
    {
        if (Source->IsValidIndex(ChildIndex))
        {
            Moved.Emplace(Move((*Source)[ChildIndex]));
            Source->RemoveAt(ChildIndex);
        }
    }

    TArray<FEntry>* Target = FindChildArray(TargetPath);
    if (!Target)
    {
        TArray<String> FailedNames;
        FailedNames.Reserve(Moved.Size());

        for (FEntry& Entry : Moved)
        {
            FailedNames.Emplace(Entry.Name);
            Source->Emplace(Move(Entry));
        }

        ReportFailure("Could not move these items, so they were left where they were", FailedNames);
    }
    else
    {
        for (FEntry& Entry : Moved)
        {
            Entry.Name = CreateUniqueName(*Target, Entry.Name);
            Target->Emplace(Move(Entry));
        }
    }

    while (!CurrentPath.IsEmpty() && !FindEntry(CurrentPath))
    {
        CurrentPath.Pop();
    }

    RebuildTree();
    RefreshTiles();
    RefreshBreadcrumbs();
}

void FEditorContentBrowserPanel::RequestDelete(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices, bool bIsFolderPanel)
{
    if (ChildIndices.IsEmpty())
    {
        return;
    }

    TArray<FEntry>* Children = FindChildArray(ParentPath);
    if (!Children)
    {
        return;
    }

    FEditorConfirmDialog::FDesc Desc;

    if (bIsFolderPanel)
    {
        const String Name = Children->IsValidIndex(ChildIndices[0]) ? (*Children)[ChildIndices[0]].Name : String("this folder");

        Desc.Title   = "Delete Folder";
        Desc.Message = String::Printf("Are you sure you want to delete \"%s\" and its contents?", Name.Data());
    }
    else if (ChildIndices.Size() == 1)
    {
        const String Name = Children->IsValidIndex(ChildIndices[0]) ? (*Children)[ChildIndices[0]].Name : String("this item");

        Desc.Title   = "Delete";
        Desc.Message = String::Printf("Are you sure you want to delete \"%s\"?", Name.Data());
    }
    else
    {
        Desc.Title   = "Delete";
        Desc.Message = "Are you sure you want to delete the selected items?";
    }

    const FEntryPath  CapturedPath    = ParentPath;
    const TArray<int32> CapturedIndices = ChildIndices;

    Desc.OnClosed = FOnConfirmDialogClosed::CreateLambda([this, CapturedPath, CapturedIndices](bool bConfirmed)
    {
        if (bConfirmed)
        {
            DeleteEntries(CapturedPath, CapturedIndices);
        }
    });

    FEditorConfirmDialog::Open(bIsFolderPanel ? StaticCastSharedPtr<FVisualElement>(TreeWrapper) : StaticCastSharedPtr<FVisualElement>(GridWrapper), Desc);
}

bool FEditorContentBrowserPanel::HasClipboardContent() const
{
    return !ClipboardChildIndices.IsEmpty();
}

bool FEditorContentBrowserPanel::CanMoveInto(const FEntryPath& SourceParentPath, const TArray<int32>& ChildIndices, const FEntryPath& TargetPath) const
{
    return FIndexedPathMove::CanMoveAny(SourceParentPath, ChildIndices, TargetPath);
}

TArray<String> FEditorContentBrowserPanel::CollectEntryNames(const FEntryPath& ParentPath, const TArray<int32>& ChildIndices)
{
    TArray<String> Names;

    const TArray<FEntry>* Children = FindChildArray(ParentPath);
    if (!Children)
    {
        return Names;
    }

    Names.Reserve(ChildIndices.Size());
    for (const int32 ChildIndex : ChildIndices)
    {
        if (Children->IsValidIndex(ChildIndex))
        {
            Names.Emplace((*Children)[ChildIndex].Name);
        }
    }

    return Names;
}

void FEditorContentBrowserPanel::ReportFailure(const String& Message, const TArray<String>& FailedNames)
{
    if (FailedNames.IsEmpty())
    {
        return;
    }

    FEditorErrorDialog::FDesc Desc;
    Desc.Title   = "Content Browser";
    Desc.Message = Message;
    Desc.Details = FailedNames;

    FEditorErrorDialog::Open(GridWrapper, Desc);
}

TSharedPtr<FMenu> FEditorContentBrowserPanel::BuildContextMenu(bool bIsFolderPanel)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    const bool bHasSelection = bIsFolderPanel ? !FolderTree->GetSelection().IsEmpty() : !TileView->GetSelection().IsEmpty();

    Menu->AddSection("Create", Font);

    FMenuItem::FDesc NewFolderDesc;
    NewFolderDesc.Label       = "New folder";
    NewFolderDesc.Font        = Font;
    NewFolderDesc.OnActivated = FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::NewFolder);

    Menu->AddItem(FMenuItem::Create(NewFolderDesc));

    Menu->AddSection("Common", Font);

    FMenuItem::FDesc DeleteDesc;
    DeleteDesc.Label        = "Delete";
    DeleteDesc.ShortcutText = "Delete";
    DeleteDesc.Font         = Font;
    DeleteDesc.OnActivated  = bIsFolderPanel
        ? FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeDeleteRequested)
        : FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnGridDeleteRequested);

    TSharedPtr<FMenuItem> DeleteItem = FMenuItem::Create(DeleteDesc);
    DeleteItem->SetEnabled(bHasSelection);

    Menu->AddItem(DeleteItem);

    FMenuItem::FDesc RenameDesc;
    RenameDesc.Label        = "Rename";
    RenameDesc.ShortcutText = "F2";
    RenameDesc.Font         = Font;
    RenameDesc.OnActivated  = bIsFolderPanel
        ? FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnTreeRenameRequested)
        : FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnGridRenameRequested);

    TSharedPtr<FMenuItem> RenameItem = FMenuItem::Create(RenameDesc);
    RenameItem->SetEnabled(bHasSelection);

    Menu->AddItem(RenameItem);

    FMenuItem::FDesc CopyDesc;
    CopyDesc.Label       = "Copy";
    CopyDesc.Font        = Font;
    CopyDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, bIsFolderPanel]()
    {
        if (bIsFolderPanel)
        {
            const FEntryPath Path = FindTreeItemPath(FolderTree->GetSelection().IsEmpty() ? nullptr : FolderTree->GetSelection()[0]);
            if (Path.Size() > 1)
            {
                FEntryPath ParentPath = Path;
                ParentPath.Pop();

                TArray<int32> Indices;
                Indices.Add(Path.Last());

                CopyEntries(ParentPath, Indices);
            }
        }
        else
        {
            TArray<int32> Indices;
            for (const int32 TileIndex : TileView->GetSelection())
            {
                if (VisibleChildIndices.IsValidIndex(TileIndex))
                {
                    Indices.Add(VisibleChildIndices[TileIndex]);
                }
            }

            CopyEntries(CurrentPath, Indices);
        }
    });

    TSharedPtr<FMenuItem> CopyItem = FMenuItem::Create(CopyDesc);
    CopyItem->SetEnabled(bHasSelection);

    Menu->AddItem(CopyItem);

    FMenuItem::FDesc PasteDesc;
    PasteDesc.Label       = "Paste";
    PasteDesc.Font        = Font;
    PasteDesc.OnActivated = FOnMenuItemActivated::CreateRaw(this, &FEditorContentBrowserPanel::PasteEntries);

    TSharedPtr<FMenuItem> PasteItem = FMenuItem::Create(PasteDesc);
    PasteItem->SetEnabled(HasClipboardContent());

    Menu->AddItem(PasteItem);

    return Menu;
}

void FEditorContentBrowserPanel::OnFolderSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection)
{
    if (Selection.Size() == 1)
    {
        NavigateTo(FindTreeItemPath(Selection[0]), true);
    }
}

void FEditorContentBrowserPanel::OnFolderDragDetected(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent)
{
    const FEntryPath Path = FindTreeItemPath(Item);
    if (Path.Size() <= 1)
    {
        return;
    }

    DragParentPath = Path;
    DragParentPath.Pop();

    DragChildIndices.Clear();
    DragChildIndices.Add(Path.Last());

    TreeWrapper->BeginDrag(Item->Label, FEditorIcons::Folder, 1, CursorEvent);
}

void FEditorContentBrowserPanel::OnTileActivated(int32 Index)
{
    if (!VisibleChildIndices.IsValidIndex(Index))
    {
        return;
    }

    const FEntryPath Path  = ResolveTileTarget(Index);
    FEntry*          Entry = FindEntry(Path);

    if (Entry && Entry->bIsFolder)
    {
        NavigateTo(Path, true);
    }
}

void FEditorContentBrowserPanel::OnTileDragDetected(int32 Index, const FCursorEvent& CursorEvent)
{
    if (!VisibleChildIndices.IsValidIndex(Index))
    {
        return;
    }

    DragParentPath = CurrentPath;

    DragChildIndices.Clear();

    if (TileView->IsSelected(Index))
    {
        for (const int32 TileIndex : TileView->GetSelection())
        {
            if (VisibleChildIndices.IsValidIndex(TileIndex))
            {
                DragChildIndices.Add(VisibleChildIndices[TileIndex]);
            }
        }
    }
    else
    {
        DragChildIndices.Add(VisibleChildIndices[Index]);
    }

    const FTileItem& Item = TileView->GetItems()[Index];

    GridWrapper->BeginDrag(Item.Label, Item.Icon, DragChildIndices.Size(), CursorEvent);
}

void FEditorContentBrowserPanel::OnFolderSearchTextChanged(const String& SearchText)
{
    FolderFilterText = SearchText;
    FolderTree->SetFilterText(SearchText);
}

void FEditorContentBrowserPanel::OnContentSearchTextChanged(const String& SearchText)
{
    ContentFilterText = SearchText;
    RefreshTiles();
}

void FEditorContentBrowserPanel::OnTreeContextMenu(int32 Target, const IntVector2& ScreenPosition)
{
    if (!FApplication::IsInitialized())
    {
        return;
    }

    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (Rows.IsValidIndex(Target))
    {
        NavigateTo(FindTreeItemPath(Rows[Target]), true);
    }

    TSharedPtr<FMenu> Menu = BuildContextMenu(true);
    if (!Menu)
    {
        return;
    }

    if (TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(TreeWrapper))
    {
        FMenuStack::Get().PushMenu(OwningWindow, FRectangle(ScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
    }
}

void FEditorContentBrowserPanel::OnGridContextMenu(int32 Target, const IntVector2& ScreenPosition)
{
    if (!FApplication::IsInitialized())
    {
        return;
    }

    if (Target != FEditorContentBrowserView::InvalidTarget && !TileView->IsSelected(Target))
    {
        TileView->SetSelection(Target);
    }

    TSharedPtr<FMenu> Menu = BuildContextMenu(false);
    if (!Menu)
    {
        return;
    }

    if (TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(GridWrapper))
    {
        FMenuStack::Get().PushMenu(OwningWindow, FRectangle(ScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
    }
}

void FEditorContentBrowserPanel::OnTreeRenameCommitted(int32 Target, const String& NewName)
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (Rows.IsValidIndex(Target))
    {
        RenameEntry(FindTreeItemPath(Rows[Target]), NewName);
    }
}

void FEditorContentBrowserPanel::OnGridRenameCommitted(int32 Target, const String& NewName)
{
    if (VisibleChildIndices.IsValidIndex(Target))
    {
        RenameEntry(ResolveTileTarget(Target), NewName);
    }
}

void FEditorContentBrowserPanel::OnTreeRenameRequested()
{
    const TArray<TSharedPtr<FTreeItem>>& Selection = FolderTree->GetSelection();
    if (Selection.Size() != 1)
    {
        return;
    }

    const int32 Target = FolderTree->GetVisibleRows().Find(Selection[0]);
    if (Target != TArray<TSharedPtr<FTreeItem>>::InvalidIndex)
    {
        TreeWrapper->BeginRename(Target);
    }
}

void FEditorContentBrowserPanel::OnGridRenameRequested()
{
    const TArray<int32>& Selection = TileView->GetSelection();
    if (Selection.Size() == 1)
    {
        GridWrapper->BeginRename(Selection[0]);
    }
}

void FEditorContentBrowserPanel::OnTreeDeleteRequested()
{
    const TArray<TSharedPtr<FTreeItem>>& Selection = FolderTree->GetSelection();
    if (Selection.IsEmpty())
    {
        return;
    }

    const FEntryPath Path = FindTreeItemPath(Selection[0]);
    if (Path.Size() <= 1)
    {
        return;
    }

    FEntryPath ParentPath = Path;
    ParentPath.Pop();

    TArray<int32> Indices;
    Indices.Add(Path.Last());

    RequestDelete(ParentPath, Indices, true);
}

void FEditorContentBrowserPanel::OnGridDeleteRequested()
{
    TArray<int32> Indices;
    for (const int32 TileIndex : TileView->GetSelection())
    {
        if (VisibleChildIndices.IsValidIndex(TileIndex))
        {
            Indices.Add(VisibleChildIndices[TileIndex]);
        }
    }

    RequestDelete(CurrentPath, Indices, false);
}

void FEditorContentBrowserPanel::OnTreeDropped(const String& PayloadId, int32 Target)
{
    UNREFERENCED_VARIABLE(PayloadId);

    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (Rows.IsValidIndex(Target))
    {
        MoveEntries(DragParentPath, DragChildIndices, FindTreeItemPath(Rows[Target]));
    }

    DragParentPath.Clear();
    DragChildIndices.Clear();
}

void FEditorContentBrowserPanel::OnGridDropped(const String& PayloadId, int32 Target)
{
    UNREFERENCED_VARIABLE(PayloadId);

    FEntryPath TargetPath = CurrentPath;

    const FEntryPath TilePath = ResolveTileTarget(Target);
    if (const FEntry* Entry = FindEntry(TilePath))
    {
        if (Entry->bIsFolder)
        {
            TargetPath = TilePath;
        }
    }

    MoveEntries(DragParentPath, DragChildIndices, TargetPath);

    DragParentPath.Clear();
    DragChildIndices.Clear();
}

FRectangle FEditorContentBrowserPanel::GetTreeRowBounds(int32 Target) const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    return Rows.IsValidIndex(Target) ? FolderTree->GetItemRowBounds(Rows[Target]) : FRectangle();
}

FRectangle FEditorContentBrowserPanel::GetTreeLabelBounds(int32 Target) const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    return Rows.IsValidIndex(Target) ? FolderTree->GetItemLabelBounds(Rows[Target]) : FRectangle();
}

FRectangle FEditorContentBrowserPanel::GetTileBounds(int32 Target) const
{
    return TileView->GetTileBounds(Target);
}

FRectangle FEditorContentBrowserPanel::GetTileLabelBounds(int32 Target) const
{
    return TileView->GetTileLabelBounds(Target);
}

int32 FEditorContentBrowserPanel::HitTestTree(const IntVector2& ClientPosition) const
{
    const TSharedPtr<FTreeItem> Item = FolderTree->FindItemAt(ClientPosition);
    if (!Item)
    {
        return FEditorContentBrowserView::InvalidTarget;
    }

    const int32 Index = FolderTree->GetVisibleRows().Find(Item);
    return (Index != TArray<TSharedPtr<FTreeItem>>::InvalidIndex) ? Index : FEditorContentBrowserView::InvalidTarget;
}

int32 FEditorContentBrowserPanel::HitTestGrid(const IntVector2& ClientPosition) const
{
    const int32 Index = TileView->FindTileAt(ClientPosition);
    return (Index != FTileView::InvalidTileIndex) ? Index : FEditorContentBrowserView::InvalidTarget;
}

bool FEditorContentBrowserPanel::CanDropOnTreeRow(int32 Target) const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (!Rows.IsValidIndex(Target))
    {
        return false;
    }

    return CanMoveInto(DragParentPath, DragChildIndices, FindTreeItemPath(Rows[Target]));
}

bool FEditorContentBrowserPanel::CanDropOnTile(int32 Target) const
{
    if (Target == FEditorContentBrowserView::InvalidTarget)
    {
        return CanMoveInto(DragParentPath, DragChildIndices, CurrentPath);
    }

    const FEntryPath TilePath = ResolveTileTarget(Target);
    const FEntry*    Entry    = FindEntry(TilePath);

    if (!Entry || !Entry->bIsFolder)
    {
        return false;
    }

    return CanMoveInto(DragParentPath, DragChildIndices, TilePath);
}

String FEditorContentBrowserPanel::GetTreeLabel(int32 Target) const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    return Rows.IsValidIndex(Target) ? Rows[Target]->Label : String();
}

String FEditorContentBrowserPanel::GetTileLabel(int32 Target) const
{
    const TArray<FTileItem>& Items = TileView->GetItems();
    if (!Items.IsValidIndex(Target))
    {
        return String();
    }

    const String& ItemLabel = Items[Target].Label;
    const int32   Dot       = ItemLabel.FindLastChar('.');

    return (Dot != String::InvalidIndex) ? ItemLabel.SubString(0, Dot) : ItemLabel;
}

void FEditorContentBrowserPanel::UpdateTreeDragPreview(int32 Target)
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (!Rows.IsValidIndex(Target))
    {
        UpdateDragPreview(FEntryPath(), String());
        return;
    }

    UpdateDragPreview(FindTreeItemPath(Rows[Target]), Rows[Target]->Label);
}

void FEditorContentBrowserPanel::UpdateGridDragPreview(int32 Target)
{
    FEntryPath TargetPath = CurrentPath;
    String     TargetName;

    const FEntryPath TilePath = ResolveTileTarget(Target);
    if (const FEntry* Entry = FindEntry(TilePath))
    {
        if (Entry->bIsFolder)
        {
            TargetPath = TilePath;
            TargetName = Entry->Name;
        }
        else
        {
            const TArray<String> Names = CollectEntryNames(DragParentPath, DragChildIndices);
            const String         SourceName = Names.IsEmpty() ? String() : Names[0];
            FDragDropService::Get().SetPreview(EDragDropPreviewState::Forbidden,
                String::Printf("Cannot move %s to %s", *SourceName, *Entry->Name));
            return;
        }
    }

    if (TargetName.IsEmpty())
    {
        if (const FEntry* Folder = FindEntry(CurrentPath))
        {
            TargetName = Folder->Name;
        }
        else
        {
            TargetName = "Root";
        }
    }

    UpdateDragPreview(TargetPath, TargetName);
}

void FEditorContentBrowserPanel::UpdateDragPreview(const FEntryPath& TargetPath, const String& TargetName)
{
    TArray<int32> Legal;
    TArray<int32> Illegal;
    FIndexedPathMove::Classify(DragParentPath, DragChildIndices, TargetPath, Legal, Illegal);

    String SourceName;
    if (!DragChildIndices.IsEmpty())
    {
        FEntryPath SourcePath = DragParentPath;
        SourcePath.Add(DragChildIndices[0]);
        if (const FEntry* Entry = FindEntry(SourcePath))
        {
            SourceName = Entry->Name;
        }
    }

    EDragDropPreviewState State = EDragDropPreviewState::Neutral;
    String                Status;
    if (!TargetName.IsEmpty())
    {
        if (Legal.IsEmpty())
        {
            State  = EDragDropPreviewState::Forbidden;
            Status = String::Printf("Cannot move %s to %s", *SourceName, *TargetName);
        }
        else if (!Illegal.IsEmpty())
        {
            State  = EDragDropPreviewState::Partial;
            Status = String::Printf("Move %s and others to %s", *SourceName, *TargetName);
        }
        else
        {
            State  = EDragDropPreviewState::Allowed;
            Status = String::Printf("Move %s to %s", *SourceName, *TargetName);
        }
    }

    FDragDropService::Get().SetPreview(State, Status);
}

String FEditorContentBrowserPanel::GetTreeHoverTip(int32 Target) const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = FolderTree->GetVisibleRows();
    if (!Rows.IsValidIndex(Target))
    {
        return String();
    }

    const FEntryPath Path  = FindTreeItemPath(Rows[Target]);
    const FEntry*    Entry = FindEntry(Path);
    return Entry ? BuildHoverTip(Path, *Entry) : String();
}

String FEditorContentBrowserPanel::GetGridHoverTip(int32 Target) const
{
    const FEntryPath Path  = ResolveTileTarget(Target);
    const FEntry*    Entry = FindEntry(Path);
    return Entry ? BuildHoverTip(Path, *Entry) : String();
}

String FEditorContentBrowserPanel::BuildHoverTip(const FEntryPath& Path, const FEntry& Entry) const
{
    String FullPath;
    for (int32 Depth = 0; Depth < Path.Size(); ++Depth)
    {
        FEntryPath Prefix = Path;
        Prefix.Resize(Depth + 1);
        if (const FEntry* Node = FindEntry(Prefix))
        {
            if (!FullPath.IsEmpty())
            {
                FullPath += "/";
            }
            FullPath += Node->Name;
        }
    }

    return String::Printf("%s\n%s\nPath: %s", *Entry.Name, Entry.bIsFolder ? "Folder" : "Document", *FullPath);
}
