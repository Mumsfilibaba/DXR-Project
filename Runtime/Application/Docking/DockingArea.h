#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Docking/DockNode.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class FSplitter;
class FTabStrip;

/** @brief Called when a tab was dragged clear of every docking area, which is what floats a panel. */
DECLARE_DELEGATE(FOnPanelTornOut, const String& /*PanelId*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called when a panel's tab was closed. */
DECLARE_DELEGATE(FOnPanelClosed, const String& /*PanelId*/);

struct FDropZone
{
    /** @brief Where it is, in the client coordinates of the area that gathered it. */
    FRectangle Bounds;

    /** @brief The panel a drop here lands against, empty when the area holds none to land against. */
    String TargetPanelId;

    /** @brief Which side of that panel it takes, or Center to join it as a tab. */
    EDockDirection Direction = EDockDirection::Center;
};

class APPLICATION_API FDockingArea final : public FCompoundElement
{
public:
    struct FDesc
    {
        /** @brief The face every tab label is drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The look of every tab strip the area builds, which defaults to the shared tab style. */
        FUITabStyle TabStyle = FUIStyle::GetDefault().Tab;

        /** @brief The glyph every close button draws, which falls back to a drawn cross while no texture is set. */
        FUIBrush TabCloseIcon;

        /** @brief Whether a drag far enough off a tab strip detaches the tab instead of reordering it. */
        bool bAllowTearOut : 1 = true;

        /**
         * @brief False keeps the area out of drop-target hit-testing and leaves it drawing no drop zones,
         * which the decorator an in-flight drag follows the cursor with needs, since it would otherwise
         * resolve as a target for its own drag.
         */
        bool bIsDropTarget : 1 = true;

        /** @brief Fired once a drag leaves a tab strip, which is what starts a dock drag. */
        FOnPanelTornOut OnPanelTornOut;

        /** @brief Fired when a panel's tab is closed. */
        FOnPanelClosed OnPanelClosed;
    };
    
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
     * @brief Gathers the zones a drop can land in for where the cursor is, which is the tab strip of the
     * panel under it followed by the four direction chips over that panel. Hit-testing and drawing both
     * come through here, so what the user aims at is what a drop resolves against.
     *
     * @param ClientPosition Where the cursor is, in client coordinates.
     * @param OutZones       Filled with the zones, the tab strip first. Empty when the cursor is over no
     *                       panel, except for an area holding none at all, which offers the whole of itself.
     */
    void GatherDropZones(const IntVector2& ClientPosition, TArray<FDropZone>& OutZones) const;

    /**
     * @brief Finds the drop zone under a point, which is a panel plus which side of it or the center. The
     * middle of a panel is in no zone, so a drop there lands nowhere rather than joining it as a tab.
     *
     * @param ScreenPosition  Where the cursor is, in screen coordinates.
     * @param OutTargetPanelId The panel the drop would be placed against.
     * @param OutDirection     Which side of it, or Center to drop it in as a tab.
     * @return True when the point is inside a zone of this area.
     */
    NODISCARD bool HitTestDropTarget(const IntVector2& ScreenPosition, String& OutTargetPanelId, EDockDirection& OutDirection) const;

    /**
     * @brief Gets where a drop resolved against this area would put the panel, which is what the drag
     * places its preview over.
     *
     * @param TargetPanelId The panel the drop would land against, empty when it would target the root.
     * @param Direction     Which side of that panel it would land on.
     * @param OutBounds     Set to the rectangle, in screen coordinates.
     * @return True when the area is in a window and the rectangle is not empty.
     */
    NODISCARD bool GetDropPreviewBounds(const String& TargetPanelId, EDockDirection Direction, FRectangle& OutBounds) const;

    /**
     * @brief Shows the tab whose panel this is, opening whichever strip holds it.
     *
     * @param PanelId The panel to show.
     */
    void SetActivePanel(const String& PanelId);

    /** @return Every panel the tree holds, depth first through it and in tab order inside each strip. */
    NODISCARD TArray<String> GetDockedPanelIds() const;

    /**
     * @brief Finds the strip a docked panel's tab sits in.
     *
     * The area already knows which strip belongs to which leaf, so asking it is both cheaper and
     * steadier than walking the element tree, whose shape is an implementation detail of how a
     * leaf is framed.
     *
     * @param PanelId The panel whose strip is wanted. An empty string returns the first strip there is.
     * @return The strip, or null when the tree holds no such panel.
     */
    NODISCARD TSharedPtr<FTabStrip> FindPanelTabStrip(const String& PanelId) const;

    /**
     * @brief Gets whether a panel sits in the tree.
     *
     * @param PanelId The panel to look for.
     * @return True when a strip in the tree holds a tab for it.
     */
    NODISCARD bool IsPanelDocked(const String& PanelId) const;

    /**
     * @brief Gets whether a panel's content is on screen, which a docked panel behind another tab is not.
     *
     * @param PanelId The panel to look for.
     * @return True when the tree holds it and it is the front tab of its strip.
     */
    NODISCARD bool IsPanelVisible(const String& PanelId) const;

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
            , Frame(nullptr)
        {
        }

        TSharedPtr<FTabStrip>      Strip;
        TSharedPtr<FVisualElement> Column;
        TSharedPtr<FVisualElement> Frame;
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
    
    NODISCARD int32 FindLeafAt(const IntVector2& ClientPosition) const;
    NODISCARD FRectangle ComputeDropBounds(const String& TargetPanelId, EDockDirection Direction) const;
    NODISCARD const FVisualElement* FindFocusedFrame() const;
    NODISCARD int32 DrawDropZones(FDrawCommandList& OutCommandList, int32 LayerId) const;

    TSharedPtr<FVisualElement> BuildNode(FDockNode& Node, const TArray<int32>& Path);
    void RequestRebuild();

    void DockAgainstNode(FDockNode& TargetNode, const String& PanelId, EDockDirection Direction);
    void OnTabActivated(const String& PanelId);
    void OnTabClosed(const String& PanelId);
    void OnTabReordered(const TArray<int32>& Path, const String& PanelId, int32 NewIndex);
    void OnTabDetached(const String& PanelId, const IntVector2& ClientPosition, const IntVector2& ScreenPosition);
    void OnTabDragMoved(const String& PanelId, const IntVector2& ClientPosition, const IntVector2& ScreenPosition);
    void OnTabDragFinished(const String& PanelId, const IntVector2& ClientPosition, const IntVector2& ScreenPosition);
    void OnSplitterFractionsChanged(const TArray<int32>& Path, const TArray<float>& Fractions);

    FDockNode                 Root;
    TMap<String, FPanelEntry> PanelsById;
    TArray<FLeafGeometry>     Leaves;
    TSharedPtr<IFontFace>     Font;
    FUITabStyle               TabStyle;
    FUIBrush                  TabCloseIcon;
    bool                      bAllowTearOut;
    bool                      bIsDropTarget;
    bool                      bNeedsRebuild;
    FOnPanelTornOut           OnPanelTornOutDelegate;
    FOnPanelClosed            OnPanelClosedDelegate;
};
