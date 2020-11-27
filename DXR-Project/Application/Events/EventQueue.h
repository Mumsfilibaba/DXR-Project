#pragma once
#include "Event.h"
#include "EventHandler.h"

class EventQueue
{
public:
	static void RegisterEventHandler(EventHandlerFunc Func, UInt8 EventCategoryMask = EEventCategory::EVENT_CATEGORY_ALL);
	static void RegisterEventHandler(IEventHandler* EventHandler, UInt8 EventCategoryMask = EEventCategory::EVENT_CATEGORY_ALL);
	static void UnregisterEventHandler(IEventHandler* EventHandler);

	static bool SendEvent(const Event& Event);
};