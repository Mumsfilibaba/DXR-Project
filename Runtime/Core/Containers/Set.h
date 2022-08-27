#pragma once

// TODO: Custom set implementation

#if 1
#include <set>

template<
    typename KeyType,
    typename PredicateType = std::less<KeyType>>
using TSet = std::set<KeyType, PredicateType>;

#else
#include "Iterator.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TSet - Contains unique elements which are stored in a tree structure

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

        /* Pointers to neighboring elements */
        STreeNode* Right = nullptr;
        STreeNode* Left = nullptr;
        STreeNode* Parent = nullptr;
        STreeNode* Next = nullptr;
        STreeNode* Previous = nullptr;

        T Value;
    };

public:

    using ElementType = T;

    using IteratorType = TTreeIterator<STreeNode, ElementType>;
    using ConstIteratorType = TTreeIterator<const STreeNode, const ElementType>;

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TSet()
        : RootNode(nullptr)
        , NullNode(nullptr)
    {
        ConstructUninitialized();
    }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~TSet()
    {
        Check(NullNode != nullptr);

        FreeRoot();

        delete NullNode;
        NullNode = nullptr;
    }

private:

    FORCEINLINE void ConstructUninitialized()
    {
        NullNode = new STreeNode();
        NullNode->Parent = NullNode;
        NullNode->Left = NullNode;
        NullNode->Right = NullNode;
        NullNode->Color = EColor::Black;
    }

    FORCEINLINE void InititalizeRoot()
    {
        Check(RootNode == nullptr);

        RootNode = new STreeNode();
        RootNode->Parent = NullNode;
        RootNode->Left = NullNode;
        RootNode->Right = NullNode;
        RootNode->Color = EColor::Black;
    }

    FORCEINLINE void FreeRoot()
    {
        if (RootNode)
        {
            delete RootNode;
            RootNode = nullptr;
        }
    }

    FORCEINLINE void RotateLeft(STreeNode* Node)
    {
        Check(Node != nullptr);
        Check(Node != NullNode);

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

    FORCEINLINE void RotateRight(STreeNode* Node)
    {
        Check(Node != nullptr);
        Check(Node != NullNode);

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

    FORCEINLINE const STreeNode* GetNull() const noexcept
    {
        return NullNode;
    }

    STreeNode* RootNode;
    STreeNode* NullNode;
};

#endif