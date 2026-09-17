#include "Engine/EngineUI/EditorUI/Panels/EditorSceneHierarchyPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/IEditorViewportHost.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/EditorEngine.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Application/Application.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Engine/World/ActorFilter.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TreeView.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/DragDropService.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuItem.h"
#include "Application/Menus/MenuStack.h"

static const CHAR* GActorDragDropPayloadId = "SCENE_HIERARCHY_ACTOR";
static const CHAR* GFilterTypeLabel = "Filter";

constexpr float DROP_INDICATOR_THICKNESS = 2.0f;

constexpr int32 HIERARCHY_ROW_HEIGHT = 30;

constexpr int32 HIERARCHY_TYPE_COLUMN_WIDTH = 120;

static const EEditorLightType GPlaceableLightTypes[] =
{
    EEditorLightType::Point,
    EEditorLightType::Directional,
    EEditorLightType::Sky,
};

/** @brief Called with the row that was right-clicked, which is null when the click landed below the rows. */
DECLARE_DELEGATE(FOnHierarchyContextMenu, const TSharedPtr<FTreeItem>& /*Item*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called when the Delete key went down on the tree. */
DECLARE_DELEGATE(FOnHierarchyDeleteRequested);

/** @brief Called with the name an edit committed, which the host writes to the model. */
DECLARE_DELEGATE(FOnHierarchyRenameCommitted, const TSharedPtr<FTreeItem>& /*Item*/, const String& /*NewName*/);

/** @brief Called with the dragged rows once they were dropped, whose target is null for the space below the rows. */
DECLARE_DELEGATE(FOnHierarchyItemsDropped, const TArray<TSharedPtr<FTreeItem>>& /*Items*/, const TSharedPtr<FTreeItem>& /*TargetItem*/);

static FHierarchyNode* GetItemNode(const TSharedPtr<FTreeItem>& Item)
{
    return Item ? static_cast<FHierarchyNode*>(Item->UserData) : nullptr;
}

static FActor* GetItemActor(const TSharedPtr<FTreeItem>& Item)
{
    const FHierarchyNode* Node = GetItemNode(Item);
    return Node ? Node->Actor : nullptr;
}

static FActorFilter* GetItemFilter(const TSharedPtr<FTreeItem>& Item)
{
    const FHierarchyNode* Node = GetItemNode(Item);
    return Node ? Node->Filter : nullptr;
}

static bool CanAttachActorTo(FActor* DraggedActor, FActor* TargetActor)
{
    if (!DraggedActor || !TargetActor || DraggedActor == TargetActor)
    {
        return false;
    }

    if (TargetActor->IsAttachedTo(DraggedActor))
    {
        return false;
    }

    return DraggedActor->GetParentActor() != TargetActor;
}

static bool CanReparentFilterTo(FActorFilter* DraggedFilter, FActorFilter* TargetFilter)
{
    if (!DraggedFilter || DraggedFilter == TargetFilter)
    {
        return false;
    }

    if (TargetFilter && TargetFilter->IsDescendantOf(DraggedFilter))
    {
        return false;
    }

    return DraggedFilter->GetParentFilter() != TargetFilter;
}

static bool CanDropItemOn(const TSharedPtr<FTreeItem>& Item, const TSharedPtr<FTreeItem>& TargetItem)
{
    FActor*       TargetActor  = GetItemActor(TargetItem);
    FActorFilter* TargetFilter = GetItemFilter(TargetItem);

    if (FActorFilter* Filter = GetItemFilter(Item))
    {
        return !TargetActor && CanReparentFilterTo(Filter, TargetFilter);
    }

    FActor* Actor = GetItemActor(Item);
    if (!Actor)
    {
        return false;
    }

    if (TargetActor)
    {
        return CanAttachActorTo(Actor, TargetActor);
    }

    if (TargetFilter)
    {
        return Actor->GetFilter() != TargetFilter;
    }

    return Actor->GetParentActor() != nullptr || Actor->GetFilter() != nullptr;
}

static IntVector2 ScreenToClient(const TSharedPtr<FVisualElement>& Element, const IntVector2& ScreenPosition)
{
    const FRectangle ScreenBounds = FMenuStack::GetScreenBounds(Element);
    if (ScreenBounds.IsEmpty())
    {
        return ScreenPosition;
    }

    return Element->GetContentRectangle().Position + (ScreenPosition - ScreenBounds.Position);
}

static Vector3 ResolvePlaceActorLocation(FEditorEngine* EditorEngine)
{
    constexpr float DefaultDistance = 10.0f;

    if (IEditorViewportHost* Host = EditorEngine->GetViewportHost())
    {
        if (FCameraComponent* Camera = Host->GetViewCamera())
        {
            return Camera->GetViewLocationAtDistance(DefaultDistance);
        }
    }

    return Vector3(0.0f, 0.0f, 0.0f);
}

static TSharedPtr<FMenu> BuildPlaceActorMenu(FEditorEngine* EditorEngine)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->AddSection("Primitives", Font);

    for (uint8 Index = 0; Index < static_cast<uint8>(EEditorPrimitiveType::Count); ++Index)
    {
        const EEditorPrimitiveType Type = static_cast<EEditorPrimitiveType>(Index);

        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetPrimitiveName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([EditorEngine, Type]()
        {
            if (FActor* Actor = EditorActorFactory::SpawnPrimitive(EditorEngine->GetWorld(), Type, ResolvePlaceActorLocation(EditorEngine)))
            {
                EditorEngine->SetSelectedActor(Actor);
            }
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    Menu->AddSection("Lights", Font);

    FWorld* World = EditorEngine->GetWorld();
    for (const EEditorLightType Type : GPlaceableLightTypes)
    {
        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetLightName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([EditorEngine, Type]()
        {
            FWorld* SpawnWorld = EditorEngine->GetWorld();
            if (!EditorActorFactory::CanSpawnLight(SpawnWorld, Type))
            {
                return;
            }

            if (FActor* Actor = EditorActorFactory::SpawnLight(SpawnWorld, Type, ResolvePlaceActorLocation(EditorEngine)))
            {
                EditorEngine->SetSelectedActor(Actor);
            }
        });

        TSharedPtr<FMenuItem> Item = FMenuItem::Create(Desc);
        Item->SetEnabled(EditorActorFactory::CanSpawnLight(World, Type));

        Menu->AddItem(Item);
    }

    Menu->AddSection("Camera", Font);

    FMenuItem::FDesc CameraDesc;
    CameraDesc.Label       = "Camera";
    CameraDesc.Font        = Font;
    CameraDesc.OnActivated = FOnMenuItemActivated::CreateLambda([EditorEngine]()
    {
        if (FActor* Actor = EditorActorFactory::SpawnCamera(EditorEngine->GetWorld(), ResolvePlaceActorLocation(EditorEngine)))
        {
            EditorEngine->SetSelectedActor(Actor);
        }
    });

    Menu->AddItem(FMenuItem::Create(CameraDesc));

    return Menu;
}

class FEditorSceneHierarchyView final : public FVisualElement
{
public:
    static TSharedPtr<FEditorSceneHierarchyView> Create(const TSharedPtr<FTreeView>& InTreeView, const TSharedPtr<IFontFace>& InFont);

public:
    FEditorSceneHierarchyView();
    virtual ~FEditorSceneHierarchyView();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override final;
    virtual void OnArrange(const FRectangle& AllottedBounds) override final;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override final;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;

    /**
     * @brief Puts an editable field over the row's label and hands it the keyboard, replacing any edit
     * already open.
     *
     * @param Item The row to rename, which may be null and is then ignored.
     */
    void BeginRename(const TSharedPtr<FTreeItem>& Item);

    /**
     * @brief Starts dragging a row, which is the whole selection when the row is part of it and the row
     * alone when it is not. Does nothing while an edit is open.
     *
     * @param Item        The row the drag started on.
     * @param CursorEvent The move that crossed the drag threshold.
     */
    void BeginDrag(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent);

    /** @brief Drops the field without writing anything back, and returns the keyboard to the tree. */
    void CancelRename();

    /** @return True while an editable field is open over a row. */
    NODISCARD bool IsRenaming() const
    {
        return RenameField != nullptr;
    }

    /** @brief Fired with the row that was right-clicked, which is null when the click landed below the rows. */
    FOnHierarchyContextMenu OnContextMenu;

    /** @brief Fired when the Delete key went down on the tree. */
    FOnHierarchyDeleteRequested OnDeleteRequested;

    /** @brief Fired with the name an edit committed. */
    FOnHierarchyRenameCommitted OnRenameCommitted;

    /** @brief Fired with the dragged rows once they were dropped. */
    FOnHierarchyItemsDropped OnItemsDropped;

private:
    void Initialize(const TSharedPtr<FTreeView>& InTreeView, const TSharedPtr<IFontFace>& InFont);
    void CommitRename(const String& NewName);
    void ReleaseRenameField();
    void HandleDrop(const FDragDropPayload& Payload, const IntVector2& ScreenPosition);
    void ClearDragState();

    NODISCARD EKeyInterceptResult HandleRenameFieldKeyDown(const FKeyEvent& KeyEvent);
    NODISCARD bool CanDropOn(const TSharedPtr<FTreeItem>& TargetItem) const;

    TSharedPtr<FTreeView>         TreeView;
    TSharedPtr<IFontFace>         Font;
    TSharedPtr<FEditableText>     RenameField;
    TSharedPtr<FEditableText>     RetiredRenameField;
    TSharedPtr<FTreeItem>         RenamedItem;
    TArray<TSharedPtr<FTreeItem>> DraggedItems;
    TSharedPtr<FTreeItem>         DropTargetItem;
    bool                          bIsDragging;
    bool                          bIsCursorInsideView;
    bool                          bRestoreTreeFocus;
};

TSharedPtr<FEditorSceneHierarchyView> FEditorSceneHierarchyView::Create(const TSharedPtr<FTreeView>& InTreeView, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FEditorSceneHierarchyView> NewView = MakeSharedPtr<FEditorSceneHierarchyView>();
    NewView->Initialize(InTreeView, InFont);
    return NewView;
}

FEditorSceneHierarchyView::FEditorSceneHierarchyView()
    : FVisualElement()
    , TreeView(nullptr)
    , Font(nullptr)
    , RenameField(nullptr)
    , RetiredRenameField(nullptr)
    , RenamedItem(nullptr)
    , DraggedItems()
    , DropTargetItem(nullptr)
    , bIsDragging(false)
    , bIsCursorInsideView(false)
    , bRestoreTreeFocus(false)
{
}

FEditorSceneHierarchyView::~FEditorSceneHierarchyView() = default;

void FEditorSceneHierarchyView::Initialize(const TSharedPtr<FTreeView>& InTreeView, const TSharedPtr<IFontFace>& InFont)
{
    TreeView = InTreeView;
    Font     = InFont;

    if (TreeView)
    {
        TreeView->SetParentElement(AsWeakPtr());
    }

    FOnDragOver OnOver = FOnDragOver::CreateLambda([](const FDragDropPayload& Payload)
    {
        return Payload.TypeId == GActorDragDropPayloadId;
    });

    FDragDropService::Get().RegisterTarget(AsSharedPtr(), FOnDragDropped::CreateRaw(this, &FEditorSceneHierarchyView::HandleDrop), OnOver);
}

IntVector2 FEditorSceneHierarchyView::ComputeDesiredSize() const
{
    return TreeView ? TreeView->GetCachedDesiredSize() : IntVector2(0, 0);
}

void FEditorSceneHierarchyView::OnArrange(const FRectangle& AllottedBounds)
{
    RetiredRenameField.Reset();

    if (bRestoreTreeFocus)
    {
        bRestoreTreeFocus = false;

        if (TreeView && FApplication::IsInitialized())
        {
            FApplication::Get().SetFocusElement(TreeView);
        }
    }

    if (RenameField && !RenameField->HasKeyboardFocus())
    {
        CommitRename(RenameField->GetText());
    }

    if (TreeView)
    {
        TreeView->Tick(AllottedBounds);
    }

    if (RenameField && TreeView)
    {
        const FRectangle LabelBounds = TreeView->GetItemLabelBounds(RenamedItem);
        if (LabelBounds.IsEmpty())
        {
            CancelRename();
        }
        else
        {
            RenameField->Tick(LabelBounds);
        }
    }
}

void FEditorSceneHierarchyView::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (TreeView)
    {
        OutChildren.Add(TreeView);
    }

    if (RenameField)
    {
        OutChildren.Add(RenameField);
    }
}

int32 FEditorSceneHierarchyView::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    int32 MaxLayerId = LayerId;
    if (TreeView)
    {
        const FDrawGeometry TreeGeometry(TreeView->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = TreeView->OnDraw(TreeGeometry, OutCommandList, LayerId + 1);
    }

    MaxLayerId += 1;

    OutCommandList.PushClip(MaxLayerId, AllottedGeometry.Bounds);

    if (bIsDragging && bIsCursorInsideView && TreeView)
    {
        const FRectangle DropBounds = DropTargetItem ? TreeView->GetItemRowBounds(DropTargetItem) : AllottedGeometry.Bounds;
        if (!DropBounds.IsEmpty())
        {
            OutCommandList.AddBoxOutline(MaxLayerId, DropBounds, CanDropOn(DropTargetItem) ? Style.Colors.Accent : Style.Colors.TextDisabled, DROP_INDICATOR_THICKNESS);
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

void FEditorSceneHierarchyView::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (RenameField && RenameField->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        RenameField->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        return;
    }

    if (TreeView)
    {
        TreeView->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

FEventResponse FEditorSceneHierarchyView::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonRight)
    {
        return FEventResponse::Unhandled();
    }

    if (IsRenaming())
    {
        CancelRename();
    }

    const IntVector2            ClientPosition = CursorEvent.GetClientPosition();
    const TSharedPtr<FTreeItem> Item           = TreeView ? TreeView->FindItemAt(ClientPosition) : nullptr;

    OnContextMenu.ExecuteIfBound(Item, CursorEvent.GetScreenPosition());

    return FEventResponse::Handled();
}

FEventResponse FEditorSceneHierarchyView::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bIsDragging)
    {
        return FEventResponse::Unhandled();
    }

    const IntVector2 ScreenPosition = CursorEvent.GetScreenPosition();
    const IntVector2 ClientPosition = ScreenToClient(AsSharedPtr(), ScreenPosition);

    DropTargetItem      = TreeView ? TreeView->FindItemAt(ClientPosition) : nullptr;
    bIsCursorInsideView = GetContentRectangle().EncapsulatesPoint(ClientPosition);

    FDragDropService::Get().UpdateDrag(ScreenPosition);
    return FEventResponse::Handled();
}

FEventResponse FEditorSceneHierarchyView::OnMouseButtonUp(const FCursorEvent& CursorEvent)
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

FEventResponse FEditorSceneHierarchyView::OnKeyDown(const FKeyEvent& KeyEvent)
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

    if (Key == Keys::F2 && TreeView)
    {
        const TArray<TSharedPtr<FTreeItem>>& Selection = TreeView->GetSelection();
        if (Selection.Size() == 1)
        {
            BeginRename(Selection[0]);
            return FEventResponse::Handled();
        }
    }

    if (Key == Keys::Delete)
    {
        OnDeleteRequested.ExecuteIfBound();
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

void FEditorSceneHierarchyView::BeginRename(const TSharedPtr<FTreeItem>& Item)
{
    if (!Item || !TreeView || !FApplication::IsInitialized())
    {
        return;
    }

    CancelRename();

    const FUIStyle& Style = FUIStyle::GetDefault();

    FEditableText::FDesc Desc;
    Desc.Text            = Item->Label;
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

    RenamedItem = Item;

    RenameField->SetParentElement(AsWeakPtr());
    RenameField->SelectAll();

    RenameField->GetOnTextCommitted()      = FOnTextCommittedDelegate::CreateRaw(this, &FEditorSceneHierarchyView::CommitRename);
    RenameField->GetOnKeyDownInterceptor() = FOnEditableTextKeyDownDelegate::CreateRaw(this, &FEditorSceneHierarchyView::HandleRenameFieldKeyDown);

    TreeView->ScrollToItem(Item);

    FApplication::Get().SetFocusElement(RenameField);
}

void FEditorSceneHierarchyView::CancelRename()
{
    ReleaseRenameField();
}

void FEditorSceneHierarchyView::BeginDrag(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent)
{
    if (!Item || !TreeView || IsRenaming() || !FApplication::IsInitialized())
    {
        return;
    }

    DraggedItems.Clear();

    if (TreeView->IsSelected(Item))
    {
        DraggedItems = TreeView->GetSelection();
    }
    else
    {
        DraggedItems.Emplace(Item);
    }

    FDragDropPayload Payload;
    Payload.TypeId      = GActorDragDropPayloadId;
    Payload.DisplayText = DraggedItems.Size() > 1 ? String::Printf("%s +%d", Item->Label.Data(), DraggedItems.Size() - 1) : Item->Label;
    Payload.UserData    = Item->UserData;

    bIsDragging         = true;
    bIsCursorInsideView = true;
    DropTargetItem      = Item;

    FApplication::Get().CaptureMouse(AsSharedPtr());
    FDragDropService::Get().BeginDrag(Payload, CursorEvent.GetScreenPosition());
}

void FEditorSceneHierarchyView::CommitRename(const String& NewName)
{
    const TSharedPtr<FTreeItem> Item = RenamedItem;

    ReleaseRenameField();

    OnRenameCommitted.ExecuteIfBound(Item, NewName);
}

void FEditorSceneHierarchyView::ReleaseRenameField()
{
    if (!RenameField)
    {
        return;
    }

    RetiredRenameField = RenameField;

    RenameField.Reset();
    RenamedItem.Reset();

    bRestoreTreeFocus = true;
}

EKeyInterceptResult FEditorSceneHierarchyView::HandleRenameFieldKeyDown(const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == Keys::Escape)
    {
        CancelRename();
        return EKeyInterceptResult::Handled;
    }

    return EKeyInterceptResult::NotHandled;
}

void FEditorSceneHierarchyView::HandleDrop(const FDragDropPayload& Payload, const IntVector2& ScreenPosition)
{
    if (Payload.TypeId != GActorDragDropPayloadId || DraggedItems.IsEmpty())
    {
        return;
    }

    const TSharedPtr<FTreeItem> TargetItem = TreeView ? TreeView->FindItemAt(ScreenToClient(AsSharedPtr(), ScreenPosition)) : nullptr;
    OnItemsDropped.ExecuteIfBound(DraggedItems, TargetItem);
}

void FEditorSceneHierarchyView::ClearDragState()
{
    DraggedItems.Clear();
    DropTargetItem.Reset();

    bIsDragging         = false;
    bIsCursorInsideView = false;
}

bool FEditorSceneHierarchyView::CanDropOn(const TSharedPtr<FTreeItem>& TargetItem) const
{
    for (const TSharedPtr<FTreeItem>& Item : DraggedItems)
    {
        if (CanDropItemOn(Item, TargetItem))
        {
            return true;
        }
    }

    return false;
}

FEditorSceneHierarchyPanel::FEditorSceneHierarchyPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "SceneHierarchy", "Scene Hierarchy")
    , HierarchyView(nullptr)
    , TreeView(nullptr)
    , SearchBox(nullptr)
    , Nodes()
    , ItemsByActor()
    , ItemsByFilter()
    , SelectedFilter(nullptr)
    , WorldRevision(0)
    , bIsSyncingSelection(false)
{
}

FEditorSceneHierarchyPanel::~FEditorSceneHierarchyPanel()
{
}

bool FEditorSceneHierarchyPanel::Initialize()
{
    FTreeView::FDesc TreeDesc;
    TreeDesc.Font                = FEditorStyle::GetFonts().Body;
    TreeDesc.RowHeight           = HIERARCHY_ROW_HEIGHT;
    TreeDesc.TypeColumnWidth     = HIERARCHY_TYPE_COLUMN_WIDTH;
    TreeDesc.LabelColumnHeader   = "Item Label";
    TreeDesc.TypeColumnHeader    = "Type";
    TreeDesc.bShowScrollBar      = true;
    TreeDesc.bAlternateRowColors = true;
    TreeDesc.bAllowMultiSelect   = true;
    TreeDesc.OnSelectionChanged  = FOnTreeSelectionChanged::CreateRaw(this, &FEditorSceneHierarchyPanel::OnTreeSelectionChanged);
    TreeDesc.OnItemActivated     = FOnTreeItemActivated::CreateRaw(this, &FEditorSceneHierarchyPanel::OnTreeItemActivated);
    TreeDesc.OnDragDetected      = FOnTreeItemDragDetected::CreateRaw(this, &FEditorSceneHierarchyPanel::OnTreeDragDetected);

    FEditorStyle::ApplyTreeViewArrows(TreeDesc);

    TreeView = FTreeView::Create(TreeDesc);
    if (!TreeView)
    {
        return false;
    }

    HierarchyView = FEditorSceneHierarchyView::Create(TreeView, FEditorStyle::GetFonts().Body);
    if (!HierarchyView)
    {
        return false;
    }

    HierarchyView->OnContextMenu     = FOnHierarchyContextMenu::CreateRaw(this, &FEditorSceneHierarchyPanel::OnRowContextMenu);
    HierarchyView->OnDeleteRequested = FOnHierarchyDeleteRequested::CreateRaw(this, &FEditorSceneHierarchyPanel::OnDeleteRequested);
    HierarchyView->OnRenameCommitted = FOnHierarchyRenameCommitted::CreateRaw(this, &FEditorSceneHierarchyPanel::OnRenameCommitted);
    HierarchyView->OnItemsDropped    = FOnHierarchyItemsDropped::CreateRaw(this, &FEditorSceneHierarchyPanel::OnItemsDropped);

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Actors",
        FOnSearchTextChanged::CreateRaw(this, &FEditorSceneHierarchyPanel::OnSearchTextChanged)));

    if (!SearchBox)
    {
        return false;
    }

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(SearchBox).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Column->AddSlot(HierarchyView).SetFillCoefficient(1.0f);

    Content = Column;

    RebuildTree();
    return true;
}

TSharedPtr<FMenu> FEditorSceneHierarchyPanel::BuildRowContextMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    const int32 SelectedCount     = EditorEngine->GetSelectedActors().Size();
    const bool  bHasActorSelected = SelectedCount > 0;
    const bool  bHasFilterOnly    = !bHasActorSelected && SelectedFilter != nullptr;

    FActorFilter* ContextFilter = GetContextFilter();

    FMenuItem::FDesc AddFilterDesc;
    AddFilterDesc.Label       = "Add Filter";
    AddFilterDesc.Font        = Font;
    AddFilterDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this, ContextFilter]()
    {
        AddFilter(ContextFilter);
    });

    FMenuItem::FDesc RenameDesc;
    RenameDesc.Label        = "Rename";
    RenameDesc.ShortcutText = "F2";
    RenameDesc.Font         = Font;
    RenameDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        FActor* Actor = EditorEngine->GetSelectedActor();

        TSharedPtr<FTreeItem>* Item = Actor ? ItemsByActor.Find(Actor) : (SelectedFilter ? ItemsByFilter.Find(SelectedFilter) : nullptr);
        if (Item)
        {
            HierarchyView->BeginRename(*Item);
        }
    });

    TSharedPtr<FMenuItem> RenameItem = FMenuItem::Create(RenameDesc);
    RenameItem->SetEnabled(SelectedCount == 1 || bHasFilterOnly);

    FMenuItem::FDesc DeleteDesc;
    DeleteDesc.Label        = SelectedCount > 1 ? String::Printf("Delete %d Actors", SelectedCount) : String("Delete");
    DeleteDesc.ShortcutText = "Del";
    DeleteDesc.Font         = Font;
    DeleteDesc.OnActivated  = FOnMenuItemActivated::CreateRaw(this, &FEditorSceneHierarchyPanel::OnDeleteRequested);

    TSharedPtr<FMenuItem> DeleteItem = FMenuItem::Create(DeleteDesc);
    DeleteItem->SetEnabled(bHasActorSelected || bHasFilterOnly);

    const bool bHasParent = EditorEngine->GetSelectedActors().ContainsWithPredicate([](FActor* Actor)
    {
        return Actor && Actor->GetParentActor() != nullptr;
    });

    FMenuItem::FDesc DetachDesc;
    DetachDesc.Label       = "Detach from Parent";
    DetachDesc.Font        = Font;
    DetachDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        for (FActor* Actor : EditorEngine->GetSelectedActors())
        {
            Actor->DetachFromParent(EAttachmentRule::KeepWorld);
        }

        WorldRevision = 0;
    });

    TSharedPtr<FMenuItem> DetachItem = FMenuItem::Create(DetachDesc);
    DetachItem->SetEnabled(bHasParent);

    FMenuItem::FDesc CopyDesc;
    CopyDesc.Label        = "Copy";
    CopyDesc.ShortcutText = "Ctrl+C";
    CopyDesc.Font         = Font;

    TSharedPtr<FMenuItem> CopyItem = FMenuItem::Create(CopyDesc);
    CopyItem->SetEnabled(false);

    FMenuItem::FDesc PasteDesc;
    PasteDesc.Label        = "Paste";
    PasteDesc.ShortcutText = "Ctrl+V";
    PasteDesc.Font         = Font;

    TSharedPtr<FMenuItem> PasteItem = FMenuItem::Create(PasteDesc);
    PasteItem->SetEnabled(false);

    Menu->AddSection("Create", Font);

    if (TSharedPtr<FMenu> AddActorMenu = BuildPlaceActorMenu(EditorEngine))
    {
        FMenuItem::FDesc AddActorDesc;
        AddActorDesc.Label   = "Add Actor";
        AddActorDesc.Font    = Font;
        AddActorDesc.SubMenu = AddActorMenu;

        Menu->AddItem(FMenuItem::Create(AddActorDesc));
    }

    Menu->AddItem(FMenuItem::Create(AddFilterDesc));

    Menu->AddSection("Common", Font);

    Menu->AddItem(DeleteItem);
    Menu->AddItem(RenameItem);

    if (TSharedPtr<FMenu> MoveToFilterMenu = BuildMoveToFilterMenu())
    {
        FMenuItem::FDesc MoveToDesc;
        MoveToDesc.Label   = "Move To Filter";
        MoveToDesc.Font    = Font;
        MoveToDesc.SubMenu = MoveToFilterMenu;

        TSharedPtr<FMenuItem> MoveToItem = FMenuItem::Create(MoveToDesc);
        MoveToItem->SetEnabled(bHasActorSelected || bHasFilterOnly);

        Menu->AddItem(MoveToItem);
    }

    Menu->AddItem(DetachItem);
    Menu->AddItem(CopyItem);
    Menu->AddItem(PasteItem);

    return Menu;
}

TSharedPtr<FMenu> FEditorSceneHierarchyPanel::BuildMoveToFilterMenu()
{
    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    Menu->AddSection("Clear", Font);

    FMenuItem::FDesc NoneDesc;
    NoneDesc.Label       = "None";
    NoneDesc.Font        = Font;
    NoneDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        MoveSelectionToFilter(nullptr);
    });

    Menu->AddItem(FMenuItem::Create(NoneDesc));

    Menu->AddSection("Filters", Font);

    for (FActorFilter* Filter : World->GetActorFilters())
    {
        if (Filter && !Filter->GetParentFilter())
        {
            AddMoveToFilterItems(Menu, Filter);
        }
    }

    return Menu;
}

void FEditorSceneHierarchyPanel::AddMoveToFilterItems(const TSharedPtr<FMenu>& Menu, FActorFilter* Filter)
{
    FMenuItem::FDesc Desc;
    Desc.Label       = Filter->GetName();
    Desc.Font        = FEditorStyle::GetFonts().Body;
    Desc.OnActivated = FOnMenuItemActivated::CreateLambda([this, Filter]()
    {
        MoveSelectionToFilter(Filter);
    });

    const TArray<FActorFilter*>& Children = Filter->GetChildFilters();
    if (!Children.IsEmpty())
    {
        TSharedPtr<FMenu> SubMenu = FMenu::Create();
        for (FActorFilter* Child : Children)
        {
            AddMoveToFilterItems(SubMenu, Child);
        }

        Desc.SubMenu = SubMenu;
    }

    Menu->AddItem(FMenuItem::Create(Desc));
}

void FEditorSceneHierarchyPanel::MoveSelectionToFilter(FActorFilter* Filter)
{
    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    if (SelectedFilter && EditorEngine->GetSelectedActors().IsEmpty())
    {
        World->SetActorFilterParent(SelectedFilter, Filter);
    }

    for (FActor* Actor : EditorEngine->GetSelectedActors())
    {
        Actor->DetachFromParent(EAttachmentRule::KeepWorld);
        Actor->SetFilter(Filter);
    }

    WorldRevision = 0;
}

void FEditorSceneHierarchyPanel::Release()
{
    if (HierarchyView)
    {
        FDragDropService::Get().UnregisterTarget(HierarchyView);
    }

    HierarchyView.Reset();
    TreeView.Reset();
    SearchBox.Reset();
    ItemsByActor.Clear();
    ItemsByFilter.Clear();
    Nodes.Clear();

    SelectedFilter = nullptr;

    FEditorPanel::Release();
}

uint64 FEditorSceneHierarchyPanel::ComputeWorldRevision() const
{
    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return 0;
    }

    uint64 Revision = static_cast<uint64>(World->GetActors().Size());
    for (FActor* Actor : World->GetActors())
    {
        Revision = Revision * 31 + THash<String>::GetHash(Actor->GetName());
        Revision = Revision * 31 + reinterpret_cast<uint64>(Actor->GetParentActor());
        Revision = Revision * 31 + reinterpret_cast<uint64>(Actor->GetFilter());
    }

    for (FActorFilter* Filter : World->GetActorFilters())
    {
        Revision = Revision * 31 + THash<String>::GetHash(Filter->GetName());
        Revision = Revision * 31 + reinterpret_cast<uint64>(Filter->GetParentFilter());
    }

    return Revision;
}

void FEditorSceneHierarchyPanel::Tick(float /*DeltaTime*/)
{
    const uint64 Revision = ComputeWorldRevision();

    const bool bIsRenaming = HierarchyView && HierarchyView->IsRenaming();
    if (Revision != WorldRevision && !bIsRenaming)
    {
        WorldRevision = Revision;
        RebuildTree();
    }

    SyncSelectionFromEngine();
}

void FEditorSceneHierarchyPanel::RebuildTree()
{
    ItemsByActor.Clear();
    ItemsByFilter.Clear();
    Nodes.Clear();

    TArray<TSharedPtr<FTreeItem>> Roots;

    if (FWorld* World = EditorEngine->GetWorld())
    {
        for (FActorFilter* Filter : World->GetActorFilters())
        {
            if (!Filter->GetParentFilter())
            {
                AddFilterItem(Filter, nullptr, Roots);
            }
        }

        for (FActor* Actor : World->GetActors())
        {
            if (!Actor->GetParentActor() && !Actor->GetFilter())
            {
                AddActorItem(Actor, nullptr, Roots);
            }
        }
    }

    TreeView->SetRootItems(Roots);
    SyncSelectionFromEngine();
}

TSharedPtr<FTreeItem> FEditorSceneHierarchyPanel::CreateItem(const String& Label, FActor* Actor, FActorFilter* Filter)
{
    TUniquePtr<FHierarchyNode>& Node = Nodes.Emplace(new FHierarchyNode());
    Node->Actor  = Actor;
    Node->Filter = Filter;

    TSharedPtr<FTreeItem> Item = FTreeItem::Create(Label, Node.Get());
    Item->bIsExpanded = true;

    return Item;
}

void FEditorSceneHierarchyPanel::AddFilterItem(FActorFilter* Filter, const TSharedPtr<FTreeItem>& ParentItem, TArray<TSharedPtr<FTreeItem>>& OutRoots)
{
    TSharedPtr<FTreeItem> Item = CreateItem(Filter->GetName(), nullptr, Filter);
    Item->TypeLabel    = GFilterTypeLabel;
    Item->Icon         = FEditorIcons::FolderSmall;
    Item->ExpandedIcon = FEditorIcons::FolderOpenSmall;

    if (ParentItem)
    {
        ParentItem->AddChild(Item);
    }
    else
    {
        OutRoots.Emplace(Item);
    }

    ItemsByFilter.Add(Filter, Item);

    for (FActorFilter* Child : Filter->GetChildFilters())
    {
        AddFilterItem(Child, Item, OutRoots);
    }

    if (FWorld* World = EditorEngine->GetWorld())
    {
        for (FActor* Actor : World->GetActors())
        {
            if (Actor->GetFilter() == Filter && !Actor->GetParentActor())
            {
                AddActorItem(Actor, Item, OutRoots);
            }
        }
    }
}

void FEditorSceneHierarchyPanel::AddActorItem(FActor* Actor, const TSharedPtr<FTreeItem>& ParentItem, TArray<TSharedPtr<FTreeItem>>& OutRoots)
{
    TSharedPtr<FTreeItem> Item = CreateItem(Actor->GetName(), Actor, nullptr);
    Item->TypeLabel = Actor->GetTypeLabel();

    if (ParentItem)
    {
        ParentItem->AddChild(Item);
    }
    else
    {
        OutRoots.Emplace(Item);
    }

    ItemsByActor.Add(Actor, Item);

    for (FActor* Child : Actor->GetChildActors())
    {
        AddActorItem(Child, Item, OutRoots);
    }
}

void FEditorSceneHierarchyPanel::SyncSelectionFromEngine()
{
    if (bIsSyncingSelection)
    {
        return;
    }

    TArray<TSharedPtr<FTreeItem>> Selection;
    for (FActor* Actor : EditorEngine->GetSelectedActors())
    {
        if (TSharedPtr<FTreeItem>* Item = ItemsByActor.Find(Actor))
        {
            Selection.Emplace(*Item);
        }
    }

    if (Selection.IsEmpty() && SelectedFilter)
    {
        if (TSharedPtr<FTreeItem>* Item = ItemsByFilter.Find(SelectedFilter))
        {
            Selection.Emplace(*Item);
        }
        else
        {
            SelectedFilter = nullptr;
        }
    }

    if (Selection.Size() == TreeView->GetSelection().Size())
    {
        bool bIsSame = true;
        for (const TSharedPtr<FTreeItem>& Item : Selection)
        {
            if (!TreeView->IsSelected(Item))
            {
                bIsSame = false;
                break;
            }
        }

        if (bIsSame)
        {
            return;
        }
    }

    TreeView->SetSelection(Selection);
}

void FEditorSceneHierarchyPanel::OnTreeSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection)
{
    TArray<FActor*> Actors;
    Actors.Reserve(Selection.Size());

    SelectedFilter = nullptr;

    for (const TSharedPtr<FTreeItem>& Item : Selection)
    {
        if (FActor* Actor = GetItemActor(Item))
        {
            Actors.Emplace(Actor);
        }
        else if (FActorFilter* Filter = GetItemFilter(Item))
        {
            SelectedFilter = Filter;
        }
    }

    bIsSyncingSelection = true;

    EditorEngine->SetSelectedActors(Actors);

    bIsSyncingSelection = false;
}

void FEditorSceneHierarchyPanel::OnTreeItemActivated(const TSharedPtr<FTreeItem>& Item)
{
    if (FActor* Actor = GetItemActor(Item))
    {
        if (IEditorViewportHost* Host = EditorEngine->GetViewportHost())
        {
            Host->FocusOnActor(Actor);
        }
    }
}

void FEditorSceneHierarchyPanel::OnTreeDragDetected(const TSharedPtr<FTreeItem>& Item, const FCursorEvent& CursorEvent)
{
    HierarchyView->BeginDrag(Item, CursorEvent);
}

void FEditorSceneHierarchyPanel::OnSearchTextChanged(const String& SearchText)
{
    TreeView->SetFilterText(SearchText);
}

void FEditorSceneHierarchyPanel::OnRowContextMenu(const TSharedPtr<FTreeItem>& Item, const IntVector2& ScreenPosition)
{
    if (!FApplication::IsInitialized())
    {
        return;
    }

    if (FActor* Actor = GetItemActor(Item))
    {
        if (!EditorEngine->IsActorSelected(Actor))
        {
            EditorEngine->SetSelectedActor(Actor);
        }
    }
    else if (TreeView)
    {
        TArray<TSharedPtr<FTreeItem>> Selection;
        if (Item)
        {
            Selection.Emplace(Item);
        }

        TreeView->SetSelection(Selection);
        OnTreeSelectionChanged(Selection);
    }

    TSharedPtr<FMenu> Menu = BuildRowContextMenu();
    if (!Menu)
    {
        return;
    }

    TSharedPtr<FWindow> OwningWindow = FApplication::Get().FindWindow(HierarchyView);
    if (!OwningWindow)
    {
        return;
    }

    FMenuStack::Get().PushMenu(OwningWindow, FRectangle(ScreenPosition, 0, 0), EMenuPlacement::AtCursor, Menu);
}

void FEditorSceneHierarchyPanel::OnDeleteRequested()
{
    const TArray<FActor*> SelectedActors = EditorEngine->GetSelectedActors();
    if (!SelectedActors.IsEmpty())
    {
        EditorEngine->RequestDeleteActors(SelectedActors);
        return;
    }

    if (FWorld* World = SelectedFilter ? EditorEngine->GetWorld() : nullptr)
    {
        World->DestroyActorFilter(SelectedFilter);

        SelectedFilter = nullptr;
        WorldRevision  = 0;
    }
}

void FEditorSceneHierarchyPanel::AddFilter(FActorFilter* ParentFilter)
{
    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    String Name("New Filter");
    for (int32 Suffix = 1; World->FindActorFilter(Name, ParentFilter); ++Suffix)
    {
        Name = String::Printf("New Filter %d", Suffix);
    }

    if (!World->CreateActorFilter(Name, ParentFilter))
    {
        return;
    }

    WorldRevision = 0;
}

FActorFilter* FEditorSceneHierarchyPanel::GetContextFilter() const
{
    if (SelectedFilter)
    {
        return SelectedFilter;
    }

    FActor* Actor = EditorEngine->GetSelectedActor();
    return Actor ? Actor->GetFilter() : nullptr;
}

void FEditorSceneHierarchyPanel::OnRenameCommitted(const TSharedPtr<FTreeItem>& Item, const String& NewName)
{
    if (NewName.IsEmpty())
    {
        return;
    }

    if (FActor* Actor = GetItemActor(Item))
    {
        Actor->SetName(NewName);
    }
    else if (FActorFilter* Filter = GetItemFilter(Item))
    {
        Filter->SetName(NewName);
        WorldRevision = 0;
    }
}

void FEditorSceneHierarchyPanel::OnItemsDropped(const TArray<TSharedPtr<FTreeItem>>& Items, const TSharedPtr<FTreeItem>& TargetItem)
{
    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    FActor*       TargetActor  = GetItemActor(TargetItem);
    FActorFilter* TargetFilter = GetItemFilter(TargetItem);

    for (const TSharedPtr<FTreeItem>& Item : Items)
    {
        if (!CanDropItemOn(Item, TargetItem))
        {
            continue;
        }

        if (FActorFilter* Filter = GetItemFilter(Item))
        {
            World->SetActorFilterParent(Filter, TargetFilter);
            continue;
        }

        FActor* Actor = GetItemActor(Item);
        if (TargetActor)
        {
            Actor->AttachToActor(TargetActor, EAttachmentRule::KeepWorld);
        }
        else
        {
            Actor->DetachFromParent(EAttachmentRule::KeepWorld);
            Actor->SetFilter(TargetFilter);
        }
    }

    WorldRevision = 0;
}

void FEditorSceneHierarchyPanel::OnActorRemoved(FActor* Actor)
{
    if (!ItemsByActor.Contains(Actor))
    {
        return;
    }

    if (HierarchyView && HierarchyView->IsRenaming())
    {
        HierarchyView->CancelRename();
    }

    ItemsByActor.Remove(Actor);
    WorldRevision = 0;
}
