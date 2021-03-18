#pragma once
#include "DelegateBase.h"

template<typename TInvokable>
class TDelegate;

template<typename TReturn, typename... TArgs>
class TDelegate<TReturn(TArgs...)> : TDelegateBase<TReturn(TArgs...)>
{
public:
    TDelegate()
        : TDelegateBase()
        , Delegate(nullptr)
    {
    }

    TDelegate(const TDelegate& Other)
        : TDelegateBase()
        , Delegate(nullptr)
    {
        if (Other.IsValid())
        {
            Delegate = Other.Delegate->Clone();
        }
    }

    TDelegate(TDelegate&& Other)
        : TDelegateBase()
        , Delegate(Other.Delegate)
    {
        Other.Delegate = nullptr;
    }

    ~TDelegate()
    {
        Unbind();
    }

    void BindFunction(TReturn(*Fn)(TArgs...))
    {
        Unbind();
        Delegate = new FunctionDelegate(Fn);
    }

    template<typename T>
    void BindObject(T* This, TReturn(T::* MemberFn)(TArgs...))
    {
        Unbind();
        Delegate = new TObjectDelegate<T>(This, MemberFn);
    }

    template<typename F>
    void BindLambda(F Functor)
    {
        Unbind();
        Delegate = new TLambdaDelegate<F>(Forward<F>(Functor));
    }

    void Unbind()
    {
        if (Delegate)
        {
            delete Delegate;
            Delegate = nullptr;
        }
    }

    TReturn Broadcast(TArgs... Args)
    {
        Assert(Delegate != nullptr);
        return Delegate->Invoke(Forward<TArgs>(Args)...);
    }

    void Swap(TDelegate& Other)
    {
        TDelegate Temp(Move(*this));
        *this = Move(Other);
        Other = Move(Temp);
    }

    bool IsValid() const
    {
        return Delegate != nullptr;
    }

    TReturn operator()(TArgs... Args)
    {
        return Broadcast(Forward<TArgs>(Args)...);
    }

    TDelegate& operator=(TDelegate&& RHS)
    {
        TDelegate(Move(RHS)).Swap(*this);
        return *this;
    }

    TDelegate& operator=(const TDelegate& RHS)
    {
        TDelegate(RHS).Swap(*this);
        return *this;
    }

    operator bool()
    {
        return IsValid();
    }

private:
    IDelegate* Delegate;
};