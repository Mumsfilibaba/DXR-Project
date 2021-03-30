#pragma once
#include "DelegateBase.h"

template<typename TInvokable>
class TDelegate;

template<typename TReturn, typename... TArgs>
class TDelegate<TReturn(TArgs...)> : private TDelegateBase<TReturn(TArgs...)>
{
    typedef TDelegateBase<TReturn(TArgs...)> Base;

    typedef typename Base::IDelegate        IDelegate;
    typedef typename Base::FunctionType     FunctionType;
    typedef typename Base::FunctionDelegate FunctionDelegate;

    template<typename T>
    using MemberFunctionType = typename Base::MemberFunctionType<T>;
    template<typename T>
    using ConstMemberFunctionType = typename Base::ConstMemberFunctionType<T>;
    template<typename T>
    using ObjectDelegate = typename Base::ObjectDelegate<T>;
    template<typename T>
    using ConstObjectDelegate = typename Base::ConstObjectDelegate<T>;
    template<typename F>
    using LambdaDelegate = typename Base::LambdaDelegate<F>;

    template<typename... TArgs>
    friend class TMulticastBase;

public:
    TDelegate()
        : Base()
        , Delegate(nullptr)
    {
    }

    TDelegate(const TDelegate& Other)
        : Base()
        , Delegate(nullptr)
    {
        if (Other.IsBound())
        {
            Delegate = Other.Delegate->Clone();
        }
    }

    TDelegate(TDelegate&& Other)
        : Base()
        , Delegate(Other.Delegate)
    {
        Other.Delegate = nullptr;
    }

    ~TDelegate()
    {
        Unbind();
    }

    void BindFunction(FunctionType Fn)
    {
        Unbind();
        Delegate = new FunctionDelegate(Fn);
    }

    template<typename T>
    void BindObject(T* This, MemberFunctionType<T> Fn)
    {
        Unbind();
        Delegate = new ObjectDelegate<T>(This, Fn);
    }

    template<typename T>
    void BindObject(const T* This, ConstMemberFunctionType<T> Fn)
    {
        Unbind();
        Delegate = new ConstObjectDelegate<T>(This, Fn);
    }

    template<typename F>
    void BindLambda(F Functor)
    {
        Unbind();
        Delegate = new LambdaDelegate<F>(Forward<F>(Functor));
    }

    void Unbind()
    {
        if (Delegate)
        {
            delete Delegate;
            Delegate = nullptr;
        }
    }

    TReturn Execute(TArgs... Args)
    {
        Assert(Delegate != nullptr);
        return Delegate->Execute(Forward<TArgs>(Args)...);
    }

    bool ExecuteIfBound(TArgs... Args)
    {
        if (IsBound())
        {
            Delegate->Execute(Forward<TArgs>(Args)...);
            return true;
        }
        else
        {
            return false;
        }
    }

    void Swap(TDelegate& Other)
    {
        TDelegate Temp(Move(*this));
        Delegate       = Other.Delegate;
        Other.Delegate = Temp.Delegate;
        Temp.Delegate  = nullptr;
    }

    bool IsBound() const
    {
        return Delegate != nullptr;
    }

    TReturn operator()(TArgs... Args)
    {
        return Execute(Forward<TArgs>(Args)...);
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

    operator bool() const
    {
        return IsBound();
    }

private:
    IDelegate* Delegate;
};