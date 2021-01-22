#pragma once
#include "Events.h"

/*
* EventHandlerFunc
*/

typedef Bool(*EventHandlerFunc)(const Event& Event);

/*
* IEventHandler
*/

class IEventHandler
{
public:
	virtual Bool OnEvent(const Event& Event) = 0;
};