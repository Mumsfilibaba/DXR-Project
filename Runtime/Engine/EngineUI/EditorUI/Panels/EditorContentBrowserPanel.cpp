#include "Engine/EngineUI/EditorUI/Panels/EditorContentBrowserPanel.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/TileView.h"
#include "Application/Elements/ToolBar.h"
#include "Application/Elements/TreeView.h"

FEditorContentBrowserPanel::FEditorContentBrowserPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "ContentBrowser", "Content Browser")
    , FolderTree(nullptr)
    , TileView(nullptr)
    , SearchBox(nullptr)
    , PathLabel(nullptr)
    , ToolBar(nullptr)
    , Root()
    , CurrentFolder(nullptr)
    , FilterText()
{
}

FEditorContentBrowserPanel::~FEditorContentBrowserPanel()
{
}

bool FEditorContentBrowserPanel::Initialize()
{
    BuildPlaceholderTree();
    CurrentFolder = &Root;

    FTreeView::FDesc TreeDesc;
    TreeDesc.Font               = FEditorStyle::GetFonts().Body;
    TreeDesc.bAllowMultiSelect  = false;
    TreeDesc.OnSelectionChanged = FOnTreeSelectionChanged::CreateRaw(this, &FEditorContentBrowserPanel::OnFolderSelectionChanged);

    FolderTree = FTreeView::Create(TreeDesc);
    if (!FolderTree)
    {
        return false;
    }

    FTileView::FDesc TileDesc;
    TileDesc.Font            = FEditorStyle::GetFonts().Body;
    TileDesc.OnItemActivated = FOnTileActivated::CreateRaw(this, &FEditorContentBrowserPanel::OnTileActivated);

    TileView = FTileView::Create(TileDesc);
    if (!TileView)
    {
        return false;
    }

    ToolBar = BuildToolBar();
    if (!ToolBar)
    {
        return false;
    }

    FTextBlock::FDesc PathDesc;
    PathDesc.Font = FEditorStyle::GetFonts().Body;
    PathDesc.Text = "Content";

    PathLabel = FTextBlock::Create(PathDesc);

    TSharedPtr<FVerticalBox> RightColumn = FVerticalBox::Create();
    RightColumn->AddSlot(PathLabel).SetPadding(FMargin(6, 4, 6, 4));
    RightColumn->AddSlot(TileView).SetFillCoefficient(1.0f);

    TSharedPtr<FHorizontalBox> Split = FHorizontalBox::Create();
    Split->AddSlot(FolderTree).SetFillCoefficient(0.28f);
    Split->AddSlot(RightColumn).SetFillCoefficient(0.72f);

    TSharedPtr<FVerticalBox> Layout = FVerticalBox::Create();
    Layout->AddSlot(ToolBar);
    Layout->AddSlot(Split).SetFillCoefficient(1.0f);

    Content = Layout;

    RebuildTree();
    RefreshTiles();
    return true;
}

TSharedPtr<FToolBar> FEditorContentBrowserPanel::BuildToolBar()
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

    FSearchBox::FDesc SearchDesc;
    SearchDesc.HintText      = "Search assets";
    SearchDesc.Font          = FEditorStyle::GetFonts().Body;
    SearchDesc.SearchIcon    = FEditorIcons::Search;
    SearchDesc.ClearIcon     = FEditorIcons::Close;
    SearchDesc.OnTextChanged = FOnSearchTextChanged::CreateRaw(this, &FEditorContentBrowserPanel::OnSearchTextChanged);

    SearchBox = FSearchBox::Create(SearchDesc);
    Bar->AddWidget(SearchBox);

    return Bar;
}

void FEditorContentBrowserPanel::BuildPlaceholderTree()
{
    Root.Name      = "Content";
    Root.bIsFolder = true;

    Root.Children.Clear();

    const CHAR* const FolderNames[] =
    {
        "Models",
        "Textures",
        "Materials",
        "Scenes",
        "Shaders"
    };

    for (const CHAR* FolderName : FolderNames)
    {
        FEntry Folder;
        Folder.Name      = FolderName;
        Folder.bIsFolder = true;

        Root.Children.Emplace(Folder);
    }
}

void FEditorContentBrowserPanel::Release()
{
    FolderTree.Reset();
    TileView.Reset();
    SearchBox.Reset();
    PathLabel.Reset();
    ToolBar.Reset();

    Root.Children.Clear();
    CurrentFolder = nullptr;

    FEditorPanel::Release();
}

TSharedPtr<FTreeItem> FEditorContentBrowserPanel::BuildTreeItem(FEntry& Entry)
{
    TSharedPtr<FTreeItem> Item = FTreeItem::Create(Entry.Name, &Entry);
    Item->Icon        = FEditorIcons::FolderSmall;
    Item->bIsExpanded = true;

    for (FEntry& Child : Entry.Children)
    {
        if (Child.bIsFolder)
        {
            Item->AddChild(BuildTreeItem(Child));
        }
    }

    return Item;
}

void FEditorContentBrowserPanel::RebuildTree()
{
    TArray<TSharedPtr<FTreeItem>> Roots;
    Roots.Emplace(BuildTreeItem(Root));

    FolderTree->SetRootItems(Roots);
}

void FEditorContentBrowserPanel::RefreshTiles()
{
    TArray<FTileItem> Items;

    if (CurrentFolder)
    {
        for (FEntry& Child : CurrentFolder->Children)
        {
            if (!FilterText.IsEmpty() && !Child.Name.Contains(FilterText, EStringCaseType::NoCase))
            {
                continue;
            }

            FTileItem Item;
            Item.Label    = Child.Name;
            Item.Icon     = Child.bIsFolder ? FEditorIcons::Folder : FEditorIcons::Document;
            Item.UserData = &Child;

            Items.Emplace(Item);
        }

        PathLabel->SetText(CurrentFolder->Name);
    }

    TileView->SetItems(Items);
}

void FEditorContentBrowserPanel::OnFolderSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection)
{
    if (Selection.IsEmpty())
    {
        return;
    }

    CurrentFolder = static_cast<FEntry*>(Selection[0]->UserData);
    RefreshTiles();
}

void FEditorContentBrowserPanel::OnTileActivated(int32 Index)
{
    const TArray<FTileItem>& Items = TileView->GetItems();
    if (!Items.IsValidIndex(Index))
    {
        return;
    }

    FEntry* Entry = static_cast<FEntry*>(Items[Index].UserData);
    if (Entry && Entry->bIsFolder)
    {
        CurrentFolder = Entry;
        RefreshTiles();
    }
}

void FEditorContentBrowserPanel::OnSearchTextChanged(const String& SearchText)
{
    FilterText = SearchText;
    RefreshTiles();
}
