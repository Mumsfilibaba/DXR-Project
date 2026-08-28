#pragma once
#include "Application/Graph/GraphTypes.h"

class APPLICATION_API FGraphModel
{
public:
    FGraphModel();
    ~FGraphModel();

    /**
     * @brief Appends a node, assigning ids to it and to any pin that arrived without one.
     *
     * @param Node The node to add.
     * @return The node's id, or -1 when the model is read-only.
     */
    int32 AddNode(const FGraphNode& Node);

    /**
     * @brief Removes a node and every link touching one of its pins.
     *
     * @param NodeId The node to remove.
     */
    void RemoveNode(int32 NodeId);

    /**
     * @brief Connects two pins when the connection is legal.
     *
     * @param FromPinId The output pin.
     * @param ToPinId   The input pin.
     * @return The new link id, or -1 when the connection was refused.
     */
    int32 AddLink(int32 FromPinId, int32 ToPinId);

    /**
     * @brief Removes a link.
     *
     * @param LinkId The link to remove.
     */
    void RemoveLink(int32 LinkId);

    /**
     * @brief Whether two pins may be connected: opposite directions, different nodes, matching type
     * tags, not already connected, and no cycle introduced.
     *
     * @param FromPinId The candidate output pin.
     * @param ToPinId   The candidate input pin.
     * @return True when AddLink would succeed.
     */
    NODISCARD bool CanConnect(int32 FromPinId, int32 ToPinId) const;

    /**
     * @brief Moves a node, which is the one edit a drag makes.
     *
     * @param NodeId   The node to move.
     * @param Position The new top-left corner, in graph space.
     */
    void SetNodePosition(int32 NodeId, const Vector2& Position);

    /** @brief Drops every node and link, keeping the ids already handed out spent. */
    void Clear();

    /**
     * @brief Blocks every mutation, which is the mode the render graph viewer runs in.
     *
     * @param bInIsReadOnly True to refuse edits.
     */
    void SetReadOnly(bool bInIsReadOnly);

    /** @return True while the model refuses edits, dropping every mutation and leaving the graph as it is. */
    NODISCARD FORCEINLINE bool IsReadOnly() const
    {
        return bIsReadOnly;
    }

    NODISCARD FORCEINLINE const TArray<FGraphNode>& GetNodes() const
    {
        return Nodes;
    }

    NODISCARD FORCEINLINE const TArray<FGraphLink>& GetLinks() const
    {
        return Links;
    }

    /**
     * @brief The node with an id, or null when there is none.
     *
     * @param NodeId The node to look for.
     * @return The node, or null.
     */
    NODISCARD const FGraphNode* FindNode(int32 NodeId) const;

    /**
     * @brief The link with an id, or null when there is none.
     *
     * @param LinkId The link to look for.
     * @return The link, or null.
     */
    NODISCARD const FGraphLink* FindLink(int32 LinkId) const;

    /**
     * @brief Resolves a pin id to the pin and to the node holding it.
     *
     * @param PinId     The pin to look for.
     * @param OutNodeId Receives the owning node's id, or -1 when the pin is unknown.
     * @return The pin, or null.
     */
    NODISCARD const FGraphPin* FindPin(int32 PinId, int32& OutNodeId) const;

    /**
     * @brief The links touching a pin, which is what a pin draws its connected state from.
     *
     * @param PinId The pin to count links for.
     * @return How many links have the pin at either end.
     */
    NODISCARD int32 CountLinksOnPin(int32 PinId) const;

    /**
     * @brief Counts every edit made, so an observer can tell it is looking at a stale build.
     *
     * @return A number that changes whenever the nodes or the links do.
     */
    NODISCARD FORCEINLINE int32 GetRevision() const
    {
        return Revision;
    }

private:
    NODISCARD bool CanReachNode(int32 FromNodeId, int32 TargetNodeId) const;

    TArray<FGraphNode> Nodes;
    TArray<FGraphLink> Links;
    int32              NextNodeId;
    int32              NextPinId;
    int32              NextLinkId;
    int32              Revision;
    bool               bIsReadOnly;
};
