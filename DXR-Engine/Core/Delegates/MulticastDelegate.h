#pragma once
#include "Delegate.h"

#include "Core/Containers/Array.h"

/* Base of multicast delegate */
template<typename... ArgTypes>
class TMulticastDelegate
{
    using DelegateType = TDelegate<void(ArgTypes...)>;

    template<typename... PayloadTypes>
    using FunctionType = typename TFunctionType<void(ArgTypes..., PayloadTypes...)>::Type;

    template<typename ClassType>
    using MemberFunctionType = typename TMemberFunctionType<false, ClassType, void(ArgTypes...)>::Type;

    template<typename ClassType>
    using ConstMemberFunctionType = typename TMemberFunctionType<true, ClassType, void(ArgTypes...)>::Type;

public:

    /* Empty constructor */
    FORCEINLINE TMulticastDelegate()
        : Delegates()
        , LockVariable(0)
    {
    }

    /* Copy constructor */
    FORCEINLINE TMulticastDelegate(const TMulticastDelegate& Other)
        : Delegates(Other.Delegates)
        , LockVariable(Other.LockVariable)
    {
    }

    /* Move constructor */
    FORCEINLINE TMulticastDelegate(TMulticastDelegate&& Other)
        : Delegates(Move(Other.Delegates))
        , LockVariable(Other.LockVariable)
    {
    }

    /* Destructor Unbind all delegates */
    FORCEINLINE ~TMulticastDelegate()
    {
        UnbindAll();
    }

    /* Bind "normal" function */
    template<typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddStatic(FunctionType<PayloadTypes...> Function, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateStatic<PayloadTypes...>(Function, Forward<PayloadTypes>(Payload)...));
    }

    /* Bind member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<InstanceType> Function, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /* Bind member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, MemberFunctionType<ClassType> Function, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /* Bind const member function */
    template<typename InstanceType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<InstanceType> Function, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateRaw<InstanceType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /* Bind const member function */
    template<typename InstanceType, typename ClassType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddRaw(InstanceType* This, ConstMemberFunctionType<ClassType> Function, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateRaw<InstanceType, ClassType, PayloadTypes...>(This, Function, Forward<PayloadTypes>(Payload)...));
    }

    /* Bind Lambda or other functor */
    template<typename FunctorType, typename... PayloadTypes>
    FORCEINLINE CDelegateHandle AddLambda(FunctorType Functor, PayloadTypes... Payload)
    {
        return AddDelegate(DelegateType::CreateLambda<FunctorType, PayloadTypes...>(Functor, Forward<PayloadTypes>(Payload)...));
    }

    /* Add a "standard" delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle Add(const DelegateType& Delegate)
    {
        return AddDelegate(Delegate);
    }

    /* Broadcast to all bound delegates */
    FORCEINLINE void Broadcast(ArgTypes&&... Args)
    {
        Lock();

        for (DelegateType& Delegate : Delegates)
        {
            CDelegateHandle Handle = Delegate.GetHandle();
            if (Handle.IsValid())
            {
                Delegate.Execute(Forward<ArgTypes>(Args)...);
            }
        }

        Unlock();
    }

    /* Unbind all bound delegates */
    FORCEINLINE void UnbindAll()
    {
        if (IsLocked())
        {
            for (DelegateType& Delegate : Delegates)
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
    FORCEINLINE void Swap(TMulticastDelegate& Other)
    {
        TArray<DelegateType> TempDelegates(Move(Delegates));
        Delegates = Move(Other.Delegates);
        Other.Delegates = Move(TempDelegates);
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
                        Delegates.Erase(It);
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
        bool Result = false;

        if (Object)
        {
            for (auto It = Delegates.StartIterator(); It != Delegates.EndIterator(); It++)
            {
                const void* BoundObject = It->GetBoundObject();
                if (BoundObject != nullptr && BoundObject == Object)
                {
                    if (IsLocked())
                    {
                        It->Unbind();
                    }
                    else
                    {
                        It = Delegates.Remove(It);
                    }

                    Result = true;
                }
            }
        }

        return Result;
    }

    /* Checks if a valid delegate is bound */
    FORCEINLINE bool IsBound() const
    {
        for (const DelegateType& Delegate : Delegates)
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

    /* Broadcast to all bound delegates */
    FORCEINLINE void operator()(ArgTypes&&... Args)
    {
        return Broadcast(Forward<ArgTypes>(Args)...);
    }

    /* Copy constructor */
    FORCEINLINE TMulticastDelegate& operator=(const TMulticastDelegate& Other)
    {
        CopyFrom(Other);
        return *this;
    }

    /* Move constructor */
    FORCEINLINE TMulticastDelegate& operator=(TMulticastDelegate&& Other)
    {
        MoveFrom(Other);
        return *this;
    }

    /* Checks if a delegate is bound */
    FORCEINLINE operator bool() const
    {
        return IsBound();
    }

protected:

    /* Add a new delegate to the multicast delegate */
    FORCEINLINE CDelegateHandle AddDelegate(const DelegateType& NewDelegate)
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

        /* If not pushback */
        Delegates.PushBack(NewDelegate);
        return NewHandle;
    }

    /* Copy from function */
    FORCEINLINE void CopyFrom(const TMulticastDelegate& Other)
    {
        Delegates = Other.Delegates;
        LockVariable = Other.LockVariable;
    }

    /* Move from function */
    FORCEINLINE void MoveFrom(TMulticastDelegate&& Other)
    {
        Delegates = Move(Other.Delegates);
        LockVariable = Other.LockVariable;
        Other.LockVariable = 0;
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
    TArray<DelegateType> Delegates;

    /* Lock protecting the delegate when removing during broadcasting */
    mutable uint64 LockVariable = 0;
};