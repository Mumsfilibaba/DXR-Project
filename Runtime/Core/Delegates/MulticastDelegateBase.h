#pragma once
#include "DelegateBase.h"

#include "Core/Containers/Array.h"

/* Baseclass for multicastdelegates */
class CMulticastDelegateBase
{
public:

    /* Copy constructor */
    FORCEINLINE CMulticastDelegateBase(const CMulticastDelegateBase& Other)
        : Delegates(Other.Delegates)
        , LockVariable(Other.LockVariable)
    {
    }

    /* Move constructor */
    FORCEINLINE CMulticastDelegateBase(CMulticastDelegateBase&& Other)
        : Delegates(Move(Other.Delegates))
        , LockVariable(Other.LockVariable)
    {
    }

    /* Destructor Unbind all delegates */
    FORCEINLINE ~CMulticastDelegateBase()
    {
        UnbindAll();
    }

    /* Unbind all bound delegates */
    FORCEINLINE void UnbindAll()
    {
        if (IsLocked())
        {
            for (CDelegateBase& Delegate : Delegates)
            {
                Delegate.Unbind();
            }
        }
        else
        {
            Delegates.Clear();
        }
    }

    /* Swap */
    FORCEINLINE void Swap(CMulticastDelegateBase& Other)
    {
        Delegates.Swap(Other.Delegates);
    }

    /* Unbind a handle */
    FORCEINLINE bool Unbind(CDelegateHandle Handle)
    {
        if (Handle.IsValid())
        {
            for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
            {
                if (Handle == It->GetHandle())
                {
                    if (IsLocked())
                    {
                        It->Unbind();
                    }
                    else
                    {
                        Delegates.RemoveAt(It);
                    }

                    return true;
                }
            }
        }

        return false;
    }

    /* Remove a delegete if an object is bound to it */
    FORCEINLINE bool UnbindIfBound(const void* Object)
    {
        bool bResult = false;

        if (Object)
        {
            for (int32 Index = 0; Index < Delegates.Size(); Index++)
            {
                CDelegateBase& Delegate = Delegates[Index];

                const void* BoundObject = Delegate.GetBoundObject();
                if (BoundObject != nullptr && BoundObject == Object)
                {
                    if (IsLocked())
                    {
                        Delegate.Unbind();
                    }
                    else
                    {
                        int32 LastIndex = Delegates.LastElementIndex();
                        ::Swap(Delegate, Delegates[LastIndex]);
                        Delegates.Pop();
                    }

                    bResult = true;
                }
            }
        }

        return bResult;
    }

    /* Checks if a valid delegate is bound */
    FORCEINLINE bool IsBound() const
    {
        for (const CDelegateBase& Delegate : Delegates)
        {
            CDelegateHandle Handle = Delegate.GetHandle();
            if (Handle.IsValid())
            {
                return true;
            }
        }

        return false;
    }

    /* Checks if an object is bound to any delegate */
    FORCEINLINE bool IsObjectBound(const void* Object) const
    {
        if (Object)
        {
            for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
            {
                const void* BoundObject = It->GetBoundObject();
                if (BoundObject != nullptr && BoundObject != Object)
                {
                    return true;
                }
            }
        }

        return false;
    }

    /* Returns the number of delegates */
    FORCEINLINE uint32 GetCount() const
    {
        return static_cast<uint32>(Delegates.Size());
    }

    /* Copy constructor */
    FORCEINLINE CMulticastDelegateBase& operator=(const CMulticastDelegateBase& Other)
    {
        CopyFrom(Other);
        return *this;
    }

    /* Move constructor */
    FORCEINLINE CMulticastDelegateBase& operator=(CMulticastDelegateBase&& Other)
    {
        MoveFrom(Forward<CMulticastDelegateBase>(Other));
        return *this;
    }

protected:

    /* Empty constructor */
    FORCEINLINE explicit CMulticastDelegateBase()
        : Delegates()
        , LockVariable(0)
    {
    }

    /* Add a new delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle AddDelegate(const CDelegateBase& NewDelegate)
    {
        CDelegateHandle NewHandle = NewDelegate.GetHandle();
        if (!NewHandle.IsValid())
        {
            return NewHandle;
        }

        // Make sure this delegate is not bound already
        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            CDelegateHandle Handle = It->GetHandle();
            if (NewHandle == Handle)
            {
                return Handle;
            }
        }

        /* Check if there is an opening */
        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            CDelegateHandle Handle = It->GetHandle();
            if (!NewHandle.IsValid())
            {
                *It = NewDelegate;
                return Handle;
            }
        }

        /* Remove empty slots */
        CompactArray();

        /* If not pushback */
        Delegates.Push(NewDelegate);
        return NewHandle;
    }

    /* Copy from function */
    FORCEINLINE void CopyFrom(const CMulticastDelegateBase& Other)
    {
        Delegates = Other.Delegates;
        LockVariable = Other.LockVariable;
    }

    /* Move from function */
    FORCEINLINE void MoveFrom(CMulticastDelegateBase&& Other)
    {
        Delegates = Move(Other.Delegates);
        LockVariable = Other.LockVariable;
        Other.LockVariable = 0;
    }

    /* Compact the array */
    FORCEINLINE void CompactArray()
    {
        if (!IsLocked() && !Delegates.IsEmpty())
        {
            int32 Next = Delegates.LastElementIndex();
            for (int32 Index = Next; Index >= 0; Index--)
            {
                CDelegateBase& Delegate = Delegates[Index];

                CDelegateHandle DelegateHandle = Delegate.GetHandle();
                if (!DelegateHandle.IsValid())
                {
                    // NOTE: It can be that we swap the same element when Index = Next
                    ::Swap(Delegate, Delegates[Next]);
                    Next--;
                }
            }

            int32 NumEmptyElements = Delegates.LastElementIndex() - Next;
            if (NumEmptyElements > 0)
            {
                Delegates.PopRange(NumEmptyElements);
            }
        }
    }

    /* Locks the delegates */
    FORCEINLINE void Lock() const
    {
        LockVariable++;
    }

    /* Unlocks the delegates */
    FORCEINLINE void Unlock() const
    {
        Assert(LockVariable > 0);
        LockVariable--;
    }

    /* Check if the delegates are locked */
    FORCEINLINE bool IsLocked() const
    {
        return (LockVariable > 0);
    }

    /* All bound delegates */
    TArray<CDelegateBase> Delegates;

    /* Lock protecting the delegate when removing during broadcasting */
    mutable uint64 LockVariable = 0;
};