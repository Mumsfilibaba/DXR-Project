#pragma once
#include "DelegateBase.h"

#include "Core/Containers/Array.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMulticastDelegateBase

class FMulticastDelegateBase
{
public:

    /**
     * @brief: Copy-constructor 
     * 
     * @param Other: Other delegate to copy
     */
    FORCEINLINE FMulticastDelegateBase(const FMulticastDelegateBase& Other) noexcept
        : Delegates(Other.Delegates)
        , LockVariable(Other.LockVariable)
    { }

    /**
     * @brief: Move-constructor
     *
     * @param Other: Other delegate to move
     */
    FORCEINLINE FMulticastDelegateBase(FMulticastDelegateBase&& Other) noexcept
        : Delegates(Move(Other.Delegates))
        , LockVariable(Other.LockVariable)
    { }

    /**
     * @brief: Destructor 
     */
    FORCEINLINE ~FMulticastDelegateBase() noexcept
    {
        UnbindAll();
    }

    /**
     * @brief:  Unbind all bound delegates 
     */
    FORCEINLINE void UnbindAll() noexcept
    {
        if (IsLocked())
        {
            for (FDelegateBase& Delegate : Delegates)
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
     * @brief: Swap this instance with another delegate
     * 
     * @param Other: Delegate to swap with
     */
    FORCEINLINE void Swap(FMulticastDelegateBase& Other) noexcept
    {
        Delegates.Swap(Other.Delegates);
    }

    /**
     * @brief: Unbind a handle 
     * 
     * @param Handle: Handle to remove
     * @return: Returns true if the handle was found and unbound
     */
    FORCEINLINE bool Unbind(FDelegateHandle Handle) noexcept
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
     * @brief: Remove a delegate if an object is bound to it
     * 
     * @param Object: Object to check for
     * @return: Returns true if the object was unbound from any delegate
     */
    FORCEINLINE bool UnbindIfBound(const void* Object) noexcept
    {
        bool bResult = false;

        if (Object)
        {
            for (int32 Index = 0; Index < Delegates.Size(); Index++)
            {
                FDelegateBase& Delegate = Delegates[Index];

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
     * @brief: Checks if a valid delegate is bound 
     * 
     * @return: Returns true if there is any delegate bound
     */
    FORCEINLINE bool IsBound() const noexcept
    {
        for (const FDelegateBase& Delegate : Delegates)
        {
            FDelegateHandle Handle = Delegate.GetHandle();
            if (Handle.IsValid())
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Checks if an object is bound to any delegate
     * 
     * @param Object: Object to check for
     * @return: Returns true if any of the delegates has the object bound
     */
    FORCEINLINE bool IsObjectBound(const void* Object) const noexcept
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
     * @brief: Retrieve the number of delegates 
     * 
     * @return: Returns the number of delegates bound
     */
    FORCEINLINE uint32 GetCount() const noexcept
    {
        return static_cast<uint32>(Delegates.Size());
    }

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: Delegate to copy from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE FMulticastDelegateBase& operator=(const FMulticastDelegateBase& RHS) noexcept
    {
        CopyFrom(RHS);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: Delegate to move from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE FMulticastDelegateBase& operator=(FMulticastDelegateBase&& RHS) noexcept
    {
        MoveFrom(Forward<FMulticastDelegateBase>(RHS));
        return *this;
    }

protected:

    FORCEINLINE explicit FMulticastDelegateBase() noexcept
        : Delegates()
        , LockVariable(0)
    { }

    FORCEINLINE FDelegateHandle AddDelegate(const FDelegateBase& NewDelegate) noexcept
    {
        FDelegateHandle NewHandle = NewDelegate.GetHandle();
        if (!NewHandle.IsValid())
        {
            return NewHandle;
        }

        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            FDelegateHandle Handle = It->GetHandle();
            if (NewHandle == Handle)
            {
                return Handle;
            }
        }

        for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
        {
            FDelegateHandle Handle = It->GetHandle();
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

    FORCEINLINE void CopyFrom(const FMulticastDelegateBase& Other) noexcept
    {
        Delegates = Other.Delegates;
        LockVariable = Other.LockVariable;
    }

    FORCEINLINE void MoveFrom(FMulticastDelegateBase&& Other) noexcept
    {
        Delegates = Move(Other.Delegates);
        LockVariable = Other.LockVariable;
        Other.LockVariable = 0;
    }

    FORCEINLINE void CompactArray() noexcept
    {
        if (!IsLocked() && !Delegates.IsEmpty())
        {
            int32 Next = Delegates.LastElementIndex();
            for (int32 Index = Next; Index >= 0; Index--)
            {
                FDelegateBase& Delegate = Delegates[Index];

                FDelegateHandle DelegateHandle = Delegate.GetHandle();
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

    FORCEINLINE void Lock() const noexcept
    {
        LockVariable++;
    }

    FORCEINLINE void Unlock() const noexcept
    {
        Check(LockVariable > 0);
        LockVariable--;
    }

    FORCEINLINE bool IsLocked() const
    {
        return (LockVariable > 0);
    }

    TArray<FDelegateBase> Delegates;

     /** @brief: Lock protecting the delegate when removing during broadcasting */
    mutable uint64 LockVariable = 0;
};
