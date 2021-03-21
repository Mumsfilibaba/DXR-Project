#pragma once
#include "MulticastBase.h"

template<typename... TArgs>
class TMulticastDelegateBase : public TMulticastBase<TArgs...>
{
protected:
    typedef TMulticastBase<TArgs...> Base;

    typedef typename Base::IDelegate IDelegate;

public:
    TMulticastDelegateBase()
        : Base()
    {
    }

    TMulticastDelegateBase(const TMulticastDelegateBase& Other)
        : Base()
    {
        for (IDelegate* Delegate : Other.Delegates)
        {
            Assert(Delegate != nullptr);
            Base::Delegates.EmplaceBack(Delegate->Clone());
        }
    }

    TMulticastDelegateBase(TMulticastDelegateBase&& Other)
        : Base()
    {
        Base::Delegates = Move(Other.Delegates);
    }

    ~TMulticastDelegateBase()
    {
        UnbindAll();
    }

    void UnbindAll()
    {
        for (IDelegate* Delegate : Base::Delegates)
        {
            Assert(Delegate != nullptr);
            delete Delegate;
        }

        Base::Delegates.Clear();
    }

    void Swap(TMulticastDelegateBase& Other)
    {
        TMulticastDelegateBase Temp(Move(*this));
        Base::Delegates = Move(Other.Delegates);
        Other.Delegates  = Move(Temp.Delegates);
    }

    TMulticastDelegateBase& operator=(const TMulticastDelegateBase& RHS)
    {
        TMulticastDelegateBase(RHS).Swap(*this);
        return *this;
    }

    TMulticastDelegateBase& operator=(TMulticastDelegateBase&& RHS)
    {
        TMulticastDelegateBase(Move(RHS)).Swap(*this);
        return *this;
    }
};

template<typename... TArgs>
class TMulticastDelegate : public TMulticastDelegateBase<TArgs...>
{
    typedef TMulticastDelegateBase<TArgs...> Base;

    typedef typename Base::IDelegate IDelegate;

public:
    void Broadcast(TArgs... Args)
    {
        for (IDelegate* Delegate : Base::Delegates)
        {
            Assert(Delegate != nullptr);
            Delegate->Execute(Forward<TArgs>(Args)...);
        }
    }

    void operator()(TArgs... Args)
    {
        return Broadcast(Forward<TArgs>(Args)...);
    }
};

template<>
class TMulticastDelegate<void> : public TMulticastDelegateBase<void>
{
    typedef TMulticastDelegateBase<void> Base;

    typedef typename Base::IDelegate IDelegate;

public:
    void Broadcast()
    {
        for (IDelegate* Delegate : Base::Delegates)
        {
            Assert(Delegate != nullptr);
            Delegate->Execute();
        }
    }

    void operator()()
    {
        return Broadcast();
    }
};