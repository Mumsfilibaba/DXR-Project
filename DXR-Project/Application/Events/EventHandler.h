#pragma once
#include "Event.h"

/*
* EventHandlerFunc
*/

typedef bool(*EventHandlerFunc)(const Event& Event);

/*
* IEventHandler
*/

class IEventHandler
{
public:
	virtual bool OnEvent(const Event& Event) = 0;
};