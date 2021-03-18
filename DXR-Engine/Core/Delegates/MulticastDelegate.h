#pragma once
#include "Delegate.h"

#include "Core/Containers/Array.h"

template<typename TInvokable>
class TMulticastDelegate;

template<typename TReturn, typename... TArgs>
class TMulticastDelegate<TReturn(TArgs...)> : TDelegateBase<TReturn(TArgs...)>
{
private:
    TArray<IDelegate*> Delegates;
};