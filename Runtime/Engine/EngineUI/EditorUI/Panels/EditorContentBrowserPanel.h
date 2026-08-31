#pragma once
#include "Core/Containers/Array.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

struct FTreeItem;

class FSearchBox;
class FTextBlock;
class FTileView;
class FToolBar;
class FTreeView;

class ENGINE_API FEditorContentBrowserPanel final : public FEditorPanel
{
public:
    FEditorContentBrowserPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorContentBrowserPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;

private:
    struct FEntry
    {
        String         Name;
        bool           bIsFolder = false;
        TArray<FEntry> Children;
    };

    void BuildPlaceholderTree();
    void RebuildTree();
    void RefreshTiles();

    NODISCARD TSharedPtr<FTreeItem> BuildTreeItem(FEntry& Entry);
    NODISCARD TSharedPtr<FToolBar>  BuildToolBar();

    void OnFolderSelectionChanged(const TArray<TSharedPtr<FTreeItem>>& Selection);
    void OnTileActivated(int32 Index);
    void OnSearchTextChanged(const String& SearchText);

    TSharedPtr<FTreeView>   FolderTree;
    TSharedPtr<FTileView>   TileView;
    TSharedPtr<FSearchBox>  SearchBox;
    TSharedPtr<FTextBlock>  PathLabel;
    TSharedPtr<FToolBar>    ToolBar;
    FEntry                  Root;
    FEntry*                 CurrentFolder;
    String                  FilterText;
};
