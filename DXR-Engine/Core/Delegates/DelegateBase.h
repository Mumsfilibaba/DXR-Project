#pragma once
#include "Core/Containers/Utilities.h"

template<typename TInvokable>
class TDelegateBase;

template<typename TReturn, typename... TArgs>
class TDelegateBase<TReturn(TArgs...)>
{
protected:
    typedef TReturn(*FunctionType)(TArgs...);
    
    template<typename T>
    using MemberFunctionType = TReturn (T::*)(TArgs...);
    template<typename T>
    using ConstMemberFunctionType = TReturn (T::*)(TArgs...) const;

    struct IDelegate
    {
        virtual ~IDelegate() = default;

        virtual TReturn Execute(TArgs... Args) const = 0;
        virtual IDelegate* Clone() const = 0;
    };

    struct FunctionDelegate : public IDelegate
    {
        FunctionDelegate(FunctionType InFn)
            : IDelegate()
            , Fn(InFn)
        {
        }

        virtual TReturn Execute(TArgs... Args) const override
        {
            return Fn(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() const override
        {
            return new FunctionDelegate(Fn);
        }

        FunctionType Fn;
    };

    template<typename T>
    struct ObjectDelegate : public IDelegate
    {
        ObjectDelegate(T* InThis, MemberFunctionType<T> InFn)
            : IDelegate()
            , This(InThis)
            , Fn(InFn)
        {
        }

        virtual TReturn Execute(TArgs... Args) const override
        {
            return ((*This).*Fn)(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() const override
        {
            return new ObjectDelegate(This, Fn);
        }

        T* This;
        MemberFunctionType<T> Fn;
    };

    template<typename T>
    struct ConstObjectDelegate : public IDelegate
    {
        ConstObjectDelegate(const T* InThis, ConstMemberFunctionType<T> InFn)
            : IDelegate()
            , This(InThis)
            , Fn(InFn)
        {
        }

        virtual TReturn Execute(TArgs... Args) const override
        {
            return ((*This).*Fn)(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() const override
        {
            return new ConstObjectDelegate(This, Fn);
        }

        const T* This;
        ConstMemberFunctionType<T> Fn;
    };

    template<typename F>
    struct LambdaDelegate : public IDelegate
    {
        LambdaDelegate(F InInvokable)
            : IDelegate()
            , Invokable(InInvokable)
        {
        }

        virtual TReturn Execute(TArgs... Args) const override
        {
            return Invokable(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() const override
        {
            return new LambdaDelegate(Invokable);
        }

        F Invokable;
    };
};