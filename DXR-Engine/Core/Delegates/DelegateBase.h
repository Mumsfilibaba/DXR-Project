#pragma once
#include "Core/Containers/Utilities.h"

template<typename TInvokable>
class TDelegateBase;

template<typename TReturn, typename... TArgs>
class TDelegateBase<TReturn(TArgs...)>
{
protected:
    struct IDelegate
    {
        virtual ~IDelegate() = default;

        virtual TReturn Invoke(TArgs... Args) = 0;
        virtual IDelegate* Clone() = 0;
    };

    struct FunctionDelegate : public IDelegate
    {
        typedef TReturn(*Function)(TArgs...);

        FunctionDelegate(Function InFn)
            : IDelegate()
            , Fn(InFn)
        {
        }

        virtual TReturn Invoke(TArgs... Args) override
        {
            return Fn(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() override
        {
            return new FunctionDelegate(Fn);
        }

        Function Fn;
    };

    template<typename T>
    struct TObjectDelegate : public IDelegate
    {
        typedef TReturn(T::* Function)(TArgs...);

        TObjectDelegate(T* InThis, Function InFn)
            : IDelegate()
            , This(InThis)
            , Fn(InFn)
        {
        }

        virtual TReturn Invoke(TArgs... Args) override
        {
            return ((*This).*Fn)(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() override
        {
            return new TObjectDelegate(This, Fn);
        }

        T* This;
        Function Fn;
    };

    template<typename F>
    struct TLambdaDelegate : public IDelegate
    {
        TLambdaDelegate(F InInvokable)
            : IDelegate()
            , Invokable(InInvokable)
        {
        }

        virtual TReturn Invoke(TArgs... Args) override
        {
            return Invokable(Forward<TArgs>(Args)...);
        }

        virtual IDelegate* Clone() override
        {
            return new TLambdaDelegate(Invokable);
        }

        F Invokable;
    };
};