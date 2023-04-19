#pragma once
#include "Core/Containers/SharedPtr.h"

class FWidget;

class APPLICATION_API FNullWidget
{
public:
    static TSharedPtr<FWidget> Get()
    {
        return NullWidget;
    }

private:
    static TSharedPtr<FWidget> NullWidget;
};