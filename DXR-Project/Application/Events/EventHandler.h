#pragma once
#include "Event.h"

typedef bool(*EventHandlerFunc)(const Event& Event);

class IEventHandler
{
public:
	virtual bool OnEvent(const Event& Event) = 0;
};