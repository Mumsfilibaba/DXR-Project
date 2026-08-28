#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Docking/DockNode.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Text/IFontFace.h"

class FSplitter;
class FTabStrip;

/** @brief Called when a tab was dragged clear of every docking area, which is what floats a panel. */
DECLARE_DELEGATE(FOnPanelTornOut, const String& /*PanelId*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called when a panel's tab was closed. */
DECLARE_DELEGATE(FOnPanelClosed, const String& /*PanelId*/);

class APPLICATION_API FDockingArea final : public FCompoundElement
{
public:
    struct FDesc
    {
        TSharedPtr<IFontFace> Font = nullptr;
        bool                  bAllowTearOut : 1 = true;
        FOnPanelTornOut       OnPanelTornOut;
        FOnPanelClosed        OnPanelClosed;
    };
    
public:

    /** @brief How far into a node an edge drop zone reaches, as a share of its width or height. */
    static constexpr float EdgeZoneFraction = 0.25f;

public:
    static TSharedPtr<FDockingArea> Create(const FDesc& Desc);

public:
    FDockingArea();
    virtual ~FDockingArea();

    /**
     * @brief Initializes the area and registers it as a drop target.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 PrepareDesiredSize() override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Registers a panel that tabs can refer to by id.
     *
     * @param PanelId The stable id used in a saved layout.
     * @param Label   The text its tab shows.
     * @param Panel   The content shown when its tab is active.
     */
    void RegisterPanel(const String& PanelId, const String& Label, const TSharedPtr<FVisualElement>& Panel);

    /**
     * @brief Forgets a panel, removing it from the tree if it was in it.
     *
     * @param PanelId The panel to forget.
     */
    void UnregisterPanel(const String& PanelId);

    /**
     * @brief Rebuilds the tree from a saved description, dropping ids with no registered panel.
     *
     * @param RootNode The tree to apply.
     */
    void RestoreLayout(const FDockNode& RootNode);

    /**
     * @brief Gets the tree as it stands, which is what a save writes out.
     *
     * @return The root node, holding the whole tree.
     */
    NODISCARD FORCEINLINE const FDockNode& SaveLayout() const
    {
        return Root;
    }

    /**
     * @brief Writes the tree to a standalone ini file, flattened to one section per node.
     *
     * @param Filename Where to write it.
     * @return True when the file was written.
     */
    bool SaveLayoutToFile(const String& Filename) const;

    /**
     * @brief Reads a tree back and applies it, leaving the layout alone when the file cannot be read.
     *
     * @param Filename The file to read.
     * @return True when a layout was applied.
     */
    bool RestoreLayoutFromFile(const String& Filename);

    /**
     * @brief Docks a panel beside or into an existing one, which is also what a drop performs.
     *
     * @param PanelId       The panel to place.
     * @param TargetPanelId The panel to place it against. An empty string targets the root.
     * @param Direction     Which side of the target, or Center to add it as a tab.
     */
    void DockPanel(const String& PanelId, const String& TargetPanelId, EDockDirection Direction);

    /**
     * @brief Removes a panel from the tree, collapsing whatever that empties.
     *
     * @param PanelId The panel to remove.
     */
    void UndockPanel(const String& PanelId);

    /**
     * @brief Finds the drop target under a point, which is a panel plus which edge or the center.
     *
     * @param ScreenPosition  Where the cursor is, in screen coordinates.
     * @param OutTargetPanelId The panel the drop would be placed against.
     * @param OutDirection     Which side of it, or Center to drop it in as a tab.
     * @return True when the point is over a panel of this area.
     */
    NODISCARD bool HitTestDropTarget(const IntVector2& ScreenPosition, String& OutTargetPanelId, EDockDirection& OutDirection) const;

    /**
     * @brief Shows the tab whose panel this is, opening whichever strip holds it.
     *
     * @param PanelId The panel to show.
     */
    void SetActivePanel(const String& PanelId);

    /** @return Every panel the tree holds, depth first through it and in tab order inside each strip. */
    NODISCARD TArray<String> GetDockedPanelIds() const;

    /**
     * @brief Gets whether a panel sits in the tree.
     *
     * @param PanelId The panel to look for.
     * @return True when a strip in the tree holds a tab for it.
     */
    NODISCARD bool IsPanelDocked(const String& PanelId) const;

    /**
     * @brief Gets every id the area knows about, whether or not the tree holds it.
     *
     * @return The registered ids, in no order the caller should rely on.
     */
    NODISCARD TArray<String> GetRegisteredPanelIds() const;

    /**
     * @brief Reads back what a panel was registered with, which is what moving it to another area needs.
     *
     * @param PanelId  The panel to look up.
     * @param OutLabel The text its tab shows.
     * @param OutPanel Its content.
     * @return True when the area knows the id.
     */
    NODISCARD bool GetPanelRegistration(const String& PanelId, String& OutLabel, TSharedPtr<FVisualElement>& OutPanel) const;

    /**
     * @brief Builds the elements the tree currently describes, if a change is waiting to be applied.
     * Rebuilding is deferred to the next measure because most changes arrive from a tab or a splitter
     * partway through handling an event, and rebuilding there would free the element still on the stack.
     * The measure pass calls this; anything that has to see the new elements sooner can call it too.
     */
    void FlushPendingRebuild();

private:
    struct FLeafGeometry
    {
        FLeafGeometry()
            : Strip(nullptr)
            , Column(nullptr)
        {
        }

        TSharedPtr<FTabStrip>      Strip;
        TSharedPtr<FVisualElement> Column;
        TArray<int32>              Path;
    };

    struct FPanelEntry
    {
        FPanelEntry()
            : Label()
            , Panel(nullptr)
        {
        }

        String                     Label;
        TSharedPtr<FVisualElement> Panel;
    };

    NODISCARD static EDockDirection ResolveDirection(const FRectangle& Bounds, const IntVector2& ClientPosition);

    TSharedPtr<FVisualElement> BuildNode(FDockNode& Node, const TArray<int32>& Path);
    void RequestRebuild();

    NODISCARD int32 FindLeafAt(const IntVector2& ClientPosition) const;
    NODISCARD IntVector2 ToScreenPosition(const IntVector2& ClientPosition) const;

    void DrawDropIndicator(FDrawCommandList& OutCommandList, int32 LayerId) const;
    void DockAgainstNode(FDockNode& TargetNode, const String& PanelId, EDockDirection Direction);

    void OnTabActivated(const String& PanelId);
    void OnTabClosed(const String& PanelId);
    void OnTabDetached(const String& PanelId, const IntVector2& ClientPosition);
    void OnTabDragMoved(const String& PanelId, const IntVector2& ClientPosition);
    void OnTabDragFinished(const String& PanelId, const IntVector2& ClientPosition);
    void OnSplitterFractionsChanged(const TArray<int32>& Path, const TArray<float>& Fractions);

    FDockNode                 Root;
    TMap<String, FPanelEntry> PanelsById;
    TArray<FLeafGeometry>     Leaves;
    TSharedPtr<IFontFace>     Font;
    bool                      bAllowTearOut;
    bool                      bNeedsRebuild;
    FOnPanelTornOut           OnPanelTornOutDelegate;
    FOnPanelClosed            OnPanelClosedDelegate;
};
