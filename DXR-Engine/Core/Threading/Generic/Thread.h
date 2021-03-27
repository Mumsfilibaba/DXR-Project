#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"

#include "Time/Timestamp.h"

class Thread : public RefCountedObject
{
public:
    virtual ~Thread() = default;

    virtual void Sleep(Timestamp Time) = 0;

    static Thread* Create();
};