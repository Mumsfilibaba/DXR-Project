#pragma once
#include "Iterator.h"

template<typename T, typename CompareType>
class TSet
{
    enum class EColor
    {
        Red = 1,
        Black = 2,
    };

    struct STreeNode
    {
    public:

        /* TTreeIterator interface */

        FORCEINLINE STreeNode* GetNext() const noexcept
        {
            return Next;
        }

        FORCEINLINE STreeNode* GetPrevious() const noexcept
        {
            return Previous;
        }

        FORCEINLINE T* GetPointer() noexcept
        {
            return &Value;
        }

        FORCEINLINE const T* GetPointer() const noexcept
        {
            return &Value;
        }

    public:

        /* Color (Red-Black Tree) */
        EColor Color = EColor::Red;

        /* Pointers to neighbouring elements */
        STreeNode* Right = nullptr;
        STreeNode* Left = nullptr;
        STreeNode* Parent = nullptr;
        STreeNode* Next = nullptr;
        STreeNode* Previous = nullptr;

        /* Value for for this element */
        T Value;
    };

public:

    using ElementType = T;

    using IteratorType = TTreeIterator<STreeNode, ElementType>;
    using ConstIteratorType = TTreeIterator<const STreeNode, const ElementType>;

    /* Default constructor */
    FORCEINLINE TSet()
        : RootNode(nullptr)
        , NullNode(nullptr)
    {
        ConstructUninitialized();
    }

    /* Destruct nodes */
    FORCEINLINE ~TSet()
    {
        Assert(NullNode != nullptr);

        FreeRoot();

        delete NullNode;
        NullNode = nullptr;
    }

private:

    /* Initializes an empty node */
    FORCEINLINE void ConstructUninitialized()
    {
        NullNode = new STreeNode();
        NullNode->Parent = NullNode;
        NullNode->Left = NullNode;
        NullNode->Right = NullNode;
        NullNode->Color = EColor::Black;
    }

    /* Creates an empty root-node */
    FORCEINLINE void InititalizeRoot()
    {
        Assert(RootNode == nullptr);

        RootNode = new STreeNode();
        RootNode->Parent = NullNode;
        RootNode->Left = NullNode;
        RootNode->Right = NullNode;
        RootNode->Color = EColor::Black;
    }

    /* Releases the root node */
    FORCEINLINE void FreeRoot()
    {
        if (RootNode)
        {
            delete RootNode;
            RootNode = nullptr;
        }
    }

    /* Rotate tree left */
    FORCEINLINE void RotateLeft(STreeNode* Node)
    {
        Assert(Node != nullptr);
        Assert(Node != NullNode);

        STreeNode* Right = Node->Right;
        Node->Right = Right->Left;

        if (Right->Left != NullNode)
        {
            Right->Left->Parent = Node;
        }

        Right->Parent = Node->Parent;
        if (Node == Node->Parent->Left)
        {
            Node->Parent->Left = Right;
        }
        else
        {
            Node->Parent->Right = Right;
        }

        Right->Left = Node;
        Node->Parent = Right;
    }

    /* Rotate tree-node right */
    FORCEINLINE void RotateRight(STreeNode* Node)
    {
        Assert(Node != nullptr);
        Assert(Node != NullNode);

        STreeNode* Left = Node->Left;
        Node->Left = Left->Right;

        if (Left->Right != GetNull())
        {
            Left->Right->Parent = Node;
        }

        Left->Parent = Node->Parent;
        if (Node == Node->Parent->Left)
        {
            Node->Parent->Left = Left;
        }
        else
        {
            Node->Parent->Right = Left;
        }

        Left->Right = Node;
        Node->Parent = Left;
    }

    /* Retrieve the nullnode as constant */
    FORCEINLINE const STreeNode* GetNull() const noexcept
    {
        return NullNode;
    }

    STreeNode* RootNode;
    STreeNode* NullNode;
};