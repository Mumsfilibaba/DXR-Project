#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Layout/LayoutTypes.h"

enum class EDockSplitOrientation : uint8
{
    Horizontal,
    Vertical,
};

enum class EDockDirection : uint8
{
    Center,
    Left,
    Right,
    Top,
    Bottom,
};

enum class EDockNodeKind : uint8
{
    Split,
    Tabs,
};

struct FDockMetrics
{
    /** @brief The least a docked panel can be squeezed to, not counting the strip above it. */
    static constexpr int32 MinimumPanelWidth  = 120;
    static constexpr int32 MinimumPanelHeight = 60;

    /** @brief The height of a tab strip, which every Tabs node carries on top of its panel. */
    static constexpr int32 TabStripHeight = 26;

    /** @brief How wide the draggable handle between two split children is. */
    static constexpr int32 SplitterThickness = 4;
};

struct APPLICATION_API FDockNode
{
    /**
     * @brief Builds a leaf holding a stack of panels.
     *
     * @param InTabIds The panels, in tab order.
     * @return The new node.
     */
    NODISCARD static FDockNode MakeTabs(const TArray<String>& InTabIds);

    /**
     * @brief Builds a split of two children, sharing their space evenly.
     *
     * @param InOrientation Which way the split divides.
     * @param First         The child at the left or the top.
     * @param Second        The child at the right or the bottom.
     * @return The new node.
     */
    NODISCARD static FDockNode MakeSplit(EDockSplitOrientation InOrientation, const FDockNode& First, const FDockNode& Second);

    FDockNode();
    FDockNode(const FDockNode& Other);
    FDockNode(FDockNode&& Other);
    ~FDockNode();

    FDockNode& operator=(const FDockNode& Other);
    FDockNode& operator=(FDockNode&& Other);

    /**
     * @brief Gets the least size this subtree can be squeezed to, which bounds the splitter drags above it.
     *
     * @return The minimum size in pixels, counting the strip on every leaf and the handles between children.
     */
    NODISCARD IntVector2 ComputeMinimumSize() const;

    /**
     * @brief Gets whether the node holds nothing, which is what a collapse deletes.
     *
     * @return True when a Tabs node has no tabs, or a Split node has no children.
     */
    NODISCARD bool IsEmpty() const;

    /**
     * @brief Collects every panel id in this subtree, depth first and in tab order inside each leaf.
     *
     * @param OutPanelIds Receives the ids, appended to whatever it already holds.
     */
    void GatherPanelIds(TArray<String>& OutPanelIds) const;

    /**
     * @brief Finds the leaf holding a panel.
     *
     * @param PanelId The panel to look for.
     * @return The Tabs node holding it, or null when the tree does not have it.
     */
    NODISCARD FDockNode*       FindTabsNode(const String& PanelId);
    NODISCARD const FDockNode* FindTabsNode(const String& PanelId) const;

    /**
     * @brief Follows a path of child indices from this node.
     *
     * @param Path The indices to follow, which is empty for this node itself.
     * @return The node at the end of the path, or null when the path leaves the tree.
     */
    NODISCARD FDockNode*       FindByPath(const TArray<int32>& Path);
    NODISCARD const FDockNode* FindByPath(const TArray<int32>& Path) const;

    /**
     * @brief Restores the tree invariants after a removal, bottom-up from this node. Three of them have to
     * hold or the tree degenerates into nested single-child splits after a few dozen drags: an empty Tabs
     * node is deleted, a Split of one child is replaced by that child, and a Split whose child splits the
     * same way absorbs that child's children and its fractions.
     */
    void CollapseDegenerateNodes();

    /** @brief Shares the space evenly between the children, which is what a fresh split starts at. */
    void NormalizeFractions();

    /** @brief Whether the node divides its space between children or stacks panels behind tabs. */
    EDockNodeKind Kind;

    /** @brief Which way a Split node divides. Unused for Tabs. */
    EDockSplitOrientation Orientation;

    /** @brief How a Split node shares its space. Sums to one. */
    TArray<float> ChildFractions;

    /** @brief Populated for a Split node. */
    TArray<FDockNode> Children;

    /** @brief Populated for a Tabs node, naming panels by stable id. */
    TArray<String> TabIds;

    /** @brief Which tab in TabIds a Tabs node shows. A collapse clamps it back into range. */
    int32 ActiveTabIndex;
};
