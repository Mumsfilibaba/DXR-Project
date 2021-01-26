#pragma once
#include "Events.h"

typedef Bool(*EventHandlerFunc)(const Event& Event);

class IEventHandler
{
public:
    virtual Bool OnEvent(const Event& Event) = 0;
};