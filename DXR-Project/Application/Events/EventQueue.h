#pragma once
#include "Event.h"
#include "EventHandler.h"

/*
* EventQueue
*/

class EventQueue
{
public:
	static void RegisterEventHandler(EventHandlerFunc Func, UInt8 EventCategoryMask = EEventCategory::EventCategory_All);
	static void RegisterEventHandler(IEventHandler* EventHandler, UInt8 EventCategoryMask = EEventCategory::EventCategory_All);
	static void UnregisterEventHandler(IEventHandler* EventHandler);

	static bool SendEvent(const Event& Event);
};