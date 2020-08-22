#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EEventType
*/
enum class EEventType : Uint8
{ 
	UNKNOWN_EVENT = 0,

	KEY_PRESSED_EVENT	= 1,
	KEY_RELEASED_EVENT	= 2,
	KEY_TYPED_EVENT		= 3,

	MOUSE_MOVED_EVENT		= 4,
	MOUSE_PRESSED_EVENT		= 5,
	MOUSE_RELEASED_EVENT	= 6,
	MOUSE_SCROLLED_EVENT	= 7,

	WINDOW_RESIZED_EVENT	= 8,
};

/*
* EEventCategory
*/
enum EEventCategory : Uint8
{
	EVENT_CATEGORY_UNKNOWN	= 0,
	
	EVENT_CATEGORY_INPUT	= BIT(1),
	EVENT_CATEGORY_MOUSE	= BIT(2),
	EVENT_CATEGORY_KEYBOARD	= BIT(3),
	EVENT_CATEGORY_WINDOW	= BIT(4),

	EVENT_CATEGORY_ALL		= 0xff
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
	virtual const char* GetName() const \
	{ \
		return #EventType; \
	} \

#define DECLARE_EVENT_CATEGORY(Category) \
	virtual Uint8 GetEventCategory() const override \
	{ \
		return (Category); \
	} \

/*
* Base Event
*/
class Event
{
public:
	Event() = default;
	~Event() = default;

	virtual Uint8		GetEventCategory()	const = 0;
	virtual EEventType	GetEventType()		const = 0;

	virtual const char* GetName() const = 0;

	virtual std::string ToString() const
	{
		return Move(std::string(GetName()));
	}

	static EEventType GetStaticEventType()
	{
		return EEventType::UNKNOWN_EVENT;
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