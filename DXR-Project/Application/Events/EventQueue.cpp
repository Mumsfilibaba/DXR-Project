#include "EventQueue.h"

#include "Containers/TArray.h"

/*
* EventHandlerPair
*/
struct EventHandlerPair
{
	EventHandlerPair(IEventHandler* InHandler, uint8 InCategoryMask)
		: CategoryMask(InCategoryMask)
		, IsHandlerFunc(false)
	{
		Handler = InHandler;
	}

	EventHandlerPair(EventHandlerFunc InFunc, uint8 InCategoryMask)
		: CategoryMask(InCategoryMask)
		, IsHandlerFunc(true)
	{
		Func = InFunc;
	}

	union 
	{
		IEventHandler*		Handler;
		EventHandlerFunc	Func;
	};

	uint8	CategoryMask	= 0;
	bool	IsHandlerFunc	= false;
};

static TArray<EventHandlerPair>	GlobalEventHandlers;

/*
* EventQueue
*/
void EventQueue::RegisterEventHandler(EventHandlerFunc Func, uint8 EventCategoryMask)
{
	VALIDATE(Func != nullptr);

	// Make sure this function is not already registered
	for (EventHandlerPair& Pair : GlobalEventHandlers)
	{
		if (Pair.IsHandlerFunc)
		{
			if (Pair.Func == Func)
			{
				LOG_WARNING("Function is already registered as a EventHandler, will not be registered again");
				return;
			}
		}
	}

	// Add new func
	GlobalEventHandlers.EmplaceBack(Func, EventCategoryMask);
}

void EventQueue::RegisterEventHandler(IEventHandler* EventHandler, uint8 EventCategoryMask)
{	
	VALIDATE(EventHandler != nullptr);

	// Make sure this handler is not already registered
	for (EventHandlerPair& Pair : GlobalEventHandlers)
	{
		if (!Pair.IsHandlerFunc)
		{
			if (Pair.Handler == EventHandler)
			{
				LOG_WARNING("Handler is already registered as a EventHandler, will not be registered again");
				return;
			}
		}
	}

	// Add new handler
	GlobalEventHandlers.EmplaceBack(EventHandler, EventCategoryMask);
}

void EventQueue::UnregisterEventHandler(IEventHandler* EventHandler)
{
	VALIDATE(EventHandler != nullptr);

	// Make sure this handler is not already registered
	for (auto It = GlobalEventHandlers.Begin(); It != GlobalEventHandlers.End(); It++)
	{
		if (!It->IsHandlerFunc)
		{
			if (It->Handler != EventHandler)
			{
				It = GlobalEventHandlers.Erase(It);
				return;
			}
		}
	}

	LOG_WARNING("Handler is NOT registered as a EventHandler");
}

bool EventQueue::SendEvent(const Event& Event)
{
	// Make sure this handler is not already registered
	for (EventHandlerPair& Pair : GlobalEventHandlers)
	{
		if (Pair.CategoryMask & Event.GetEventCategory())
		{
			if (Pair.IsHandlerFunc)
			{
				if (Pair.Func(Event))
				{
					// This handler signaled that the event should not be sent to other handlers
					return true;
				}
			}
			else
			{
				if (Pair.Handler->OnEvent(Event))
				{
					// This handler signaled that the event should not be sent to other handlers
					return true;
				}
			}
		}
	}

	return false;
}
