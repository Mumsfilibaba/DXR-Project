#pragma once
#include "Core/Containers/SharedPtr.h"

struct FWidget : public TSharedFromThis<FWidget>
{
    virtual void Paint() = 0;
};