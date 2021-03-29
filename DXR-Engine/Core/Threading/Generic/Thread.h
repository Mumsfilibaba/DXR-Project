#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"

#include "Time/Timestamp.h"

typedef void(*ThreadFunction)();

class Thread : public RefCountedObject
{
public:
    virtual ~Thread() = default;

    virtual void Wait() = 0;

    // TODO: Enable memberfunctions and lambdas
    static Thread* Create(ThreadFunction Func);
};