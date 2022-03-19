#pragma once
#include "DelegateBase.h"

#include "Core/Containers/Array.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MulticastDelegateBase - Base-class for multi-cast delegates

class CMulticastDelegateBase
{
public:

    /**
     * Copy-constructor 
     * 
     * @param Other: Other delegate to copy
     */
    FORCEINLINE CMulticastDelegateBase(const CMulticastDelegateBase& Other)
        : Delegates(Other.Delegates)
        , LockVariable(Other.LockVariable)
    { }

    /**
     * Move-constructor
     *
     * @param Other: Other delegate to move
     */
    FORCEINLINE CMulticastDelegateBase(CMulticastDelegateBase&& Other)
        : Delegates(Move(Other.Delegates))
        , LockVariable(Other.LockVariable)
    { }

    /**
     * Destructor 
     */
    FORCEINLINE ~CMulticastDelegateBase()
    {
        UnbindAll();
    }

    /**
     *  Unbind all bound delegates 
     */
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

    /**
     * Swap this instance with another delegate
     * 
     * @param Other: Delegate to swap with
     */
    FORCEINLINE void Swap(CMulticastDelegateBase& Other)
    {
        Delegates.Swap(Other.Delegates);
    }

    /**
     * Unbind a handle 
     * 
     * @param Handle: Handle to remove
     * @return: Returns true if the handle was found and unbound
     */
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

    /**
     * Remove a delegate if an object is bound to it
     * 
     * @param Object: Object to check for
     * @return: Returns true if the object was unbound from any delegate
     */
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

    /**
     * Checks if a valid delegate is bound 
     * 
     * @return: Returns true if there is any delegate bound
     */
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

    /**
     * Checks if an object is bound to any delegate
     * 
     * @param Object: Object to check for
     * @return: Returns true if any of the delegates has the object bound
     */
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

    /**
     * Retrieve the number of delegates 
     * 
     * @return: Returns the number of delegates bound
     */
    FORCEINLINE uint32 GetCount() const
    {
        return static_cast<uint32>(Delegates.Size());
    }

    /**
     * Copy-assignment operator
     * 
     * @param Rhs: Delegate to copy from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE CMulticastDelegateBase& operator=(const CMulticastDelegateBase& Rhs)
    {
        CopyFrom(Rhs);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: Delegate to move from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE CMulticastDelegateBase& operator=(CMulticastDelegateBase&& Rhs)
    {
        MoveFrom(Forward<CMulticastDelegateBase>(Rhs));
        return *this;
    }

protected:

    FORCEINLINE explicit CMulticastDelegateBase()
        : Delegates()
        , LockVariable(0)
    { }

    FORCEINLINE CDelegateHandle AddDelegate(const CDelegateBase& NewDelegate)
    {
        CDelegateHandle NewHandle = NewDelegate.GetHandle();
        if (!NewHandle.IsValid())
        {
            return NewHandle;
        }

        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            CDelegateHandle Handle = It->GetHandle();
            if (NewHandle == Handle)
            {
                return Handle;
            }
        }

        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            CDelegateHandle Handle = It->GetHandle();
            if (!NewHandle.IsValid())
            {
                *It = NewDelegate;
                return Handle;
            }
        }

        CompactArray();

        Delegates.Push(NewDelegate);
        return NewHandle;
    }

    FORCEINLINE void CopyFrom(const CMulticastDelegateBase& Other)
    {
        Delegates = Other.Delegates;
        LockVariable = Other.LockVariable;
    }

    FORCEINLINE void MoveFrom(CMulticastDelegateBase&& Other)
    {
        Delegates = Move(Other.Delegates);
        LockVariable = Other.LockVariable;
        Other.LockVariable = 0;
    }

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

    FORCEINLINE void Lock() const
    {
        LockVariable++;
    }

    FORCEINLINE void Unlock() const
    {
        Assert(LockVariable > 0);
        LockVariable--;
    }

    FORCEINLINE bool IsLocked() const
    {
        return (LockVariable > 0);
    }

    TArray<CDelegateBase> Delegates;

    /* Lock protecting the delegate when removing during broadcasting */
    mutable uint64 LockVariable = 0;
};