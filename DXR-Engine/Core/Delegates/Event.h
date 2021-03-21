#pragma once
#include "MulticastBase.h"

template<typename... TArgs>
class TEventBase : public TMulticastBase<TArgs...>
{
protected:
    typedef TMulticastBase<TArgs...> Base;

    typedef typename Base::IDelegate IDelegate;

    TEventBase()
        : Base()
    {
    }

    TEventBase(const TEventBase& Other)
        : Base()
    {
        for (IDelegate* Delegate : Other.Delegates)
        {
            Assert(Delegate != nullptr);
            Base::Delegates.EmplaceBack(Delegate->Clone());
        }
    }

    TEventBase(TEventBase&& Other)
        : Base()
    {
        Base::Delegates = Move(Other.Delegates);
    }

    ~TEventBase()
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

    void Swap(TEventBase& Other)
    {
        TEventBase Temp(Move(*this));
        Base::Delegates = Move(Other.Delegates);
        Other.Delegates  = Move(Temp.Delegates);
    }

    TEventBase& operator=(const TEventBase& RHS)
    {
        TEventBase(RHS).Swap(*this);
        return *this;
    }

    TEventBase& operator=(TEventBase&& RHS)
    {
        TEventBase(Move(RHS)).Swap(*this);
        return *this;
    }
};

template<typename... TArgs>
class TEvent : public TEventBase<TArgs...>
{
protected:
    typedef TEventBase<TArgs...> Base;

    typedef typename Base::IDelegate IDelegate;

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
class TEvent<void> : public TEventBase<void>
{
protected:
    typedef TEventBase<void> Base;

    typedef typename Base::IDelegate IDelegate;

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

#define DECLARE_EVENT(EventType, EventDispatcherType, ...) \
    class EventType : public TEvent<__VA_ARGS__> \
    { \
        friend class EventDispatcherType; \
    }; \