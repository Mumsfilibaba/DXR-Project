#pragma once
#include "Core.h"

/*
* EEventType
*/

enum class EEventType : UInt8
{ 
	EventType_Unknown	= 0,

	EventType_KeyPressed	= 1,
	EventType_KeyReleased	= 2,
	EventType_KeyTyped		= 3,

	EventType_MouseMoved	= 4,
	EventType_MousePressed	= 5,
	EventType_MouseReleased	= 6,
	EventType_MouseScrolled	= 7,

	EventType_WindowResized = 8,
};

/*
* EEventCategory
*/

enum EEventCategory : UInt8
{
	EventCategory_Unknown	= 0,
	
	EventCategory_Input		= BIT(1),
	EventCategory_Mouse		= BIT(2),
	EventCategory_Keyboard	= BIT(3),
	EventCategory_Window	= BIT(4),

	EventCategory_All = 0xff
};

/*
* Helper Macro
*/

#define DECLARE_EVENT_CLASS(EventType) \
	static EEventType GetStaticEventType() \
	{ \
		return EEventType::EventType; \
	} \
	virtual EEventType GetEventType() const override \
	{ \
		return GetStaticEventType(); \
	} \
	virtual const Char* GetName() const \
	{ \
		return #EventType; \
	} \

#define DECLARE_EVENT_CATEGORY(Category) \
	virtual UInt8 GetEventCategory() const override \
	{ \
		return (Category); \
	} \

/*
* Base Event
*/

struct Event
{
public:
	virtual ~Event() = default;

	virtual EEventType GetEventType() const	= 0;
	virtual UInt8 GetEventCategory() const	= 0;

	virtual const Char* GetName() const = 0;

	virtual std::string ToString() const
	{
		return Move(std::string(GetName()));
	}

	static EEventType GetStaticEventType()
	{
		return EEventType::EventType_Unknown;
	}
};

/*
* Get EventType
*/

template<typename T>
inline bool IsOfEventType(const Event& InEvent)
{
	static_assert(std::is_base_of<Event, T>(), "IsOfEventType can only be used on types derived from Event");
	return (InEvent.GetEventType() == T::GetStaticEventType());
}

template<typename T>
inline T& EventCast(Event& InEvent)
{
	static_assert(std::is_base_of<Event, T>(), "EventCast can only be used on types derived from Event");
	return static_cast<T&>(InEvent);
}

template<typename T>
inline const T& EventCast(const Event& InEvent)
{
	static_assert(std::is_base_of<Event, T>(), "EventCast can only be used on types derived from Event");
	return static_cast<const T&>(InEvent);
}