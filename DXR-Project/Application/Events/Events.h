#pragma once
#include "Core.h"

#include "Application/Generic/GenericApplication.h"

#include <Containers/TSharedPtr.h>

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

inline const Char* ToString(EEventType EventType)
{
	switch (EventType)
	{
	case EEventType::EventType_KeyPressed:		return "KeyPressed";
	case EEventType::EventType_KeyReleased:		return "KeyReleased";
	case EEventType::EventType_KeyTyped:		return "KeyTyped";
	case EEventType::EventType_MouseMoved:		return "MouseMoved";
	case EEventType::EventType_MousePressed:	return "MousePressed";
	case EEventType::EventType_MouseReleased:	return "MouseReleased";
	case EEventType::EventType_MouseScrolled:	return "MouseScrolled";
	case EEventType::EventType_WindowResized:	return "WindowResized";
	}

	return "Unknown";
}

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
#define DECLARE_EVENT(Type, Category) \
	static EEventType GetStaticType() \
	{ \
		return EEventType::Type; \
	} \
	virtual EEventType GetType() const override final\
	{ \
		return GetStaticType(); \
	} \
	virtual const Char* GetTypeAsString() const \
	{ \
		return #Type; \
	} \
	virtual UInt8 GetCategoryFlags() const override \
	{ \
		return Category; \
	} \

/*
* Event - Base for events
*/

struct Event
{
	virtual ~Event() = default;

	virtual std::string ToString() const = 0;

	virtual EEventType GetType() const			= 0;
	virtual const Char* GetTypeAsString() const	= 0;
	virtual UInt8 GetCategoryFlags() const		= 0;

	inline static EEventType GetStaticType()
	{
		return EEventType::EventType_Unknown;
	}
};

/*
* Get EventType
*/

template<typename T>
inline TEnableIf<std::is_base_of_v<Event, T>, Bool> IsOfEventType(const Event& InEvent)
{
	return (InEvent.GetType() == T::GetStaticType());
}

template<typename T>
inline TEnableIf<std::is_base_of_v<Event, T>, T&> CastEvent(Event& InEvent)
{
	return static_cast<T&>(InEvent);
}

template<typename T>
inline TEnableIf<std::is_base_of_v<Event, T>, const T&> CastEvent(const Event& InEvent)
{
	return static_cast<const T&>(InEvent);
}

/*
* KeyPressedEvent
*/

struct KeyPressedEvent : public Event
{
	DECLARE_EVENT(EventType_KeyPressed, EventCategory_Input | EventCategory_Keyboard);

	KeyPressedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: Event()
		, Key(InKey)
		, Modifiers(InModifiers)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = " + ::ToString(Key);
	}

	EKey				Key;
	ModifierKeyState	Modifiers;
};

/*
* KeyReleasedEvent
*/

struct KeyReleasedEvent : public Event
{
	DECLARE_EVENT(EventType_KeyReleased, EventCategory_Input | EventCategory_Keyboard);

	KeyReleasedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: Event()
		, Key(InKey)
		, Modifiers(InModifiers)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = " + ::ToString(Key);
	}

	EKey				Key;
	ModifierKeyState	Modifiers;
};

/*
* KeyTypedEvent
*/

struct KeyTypedEvent : public Event
{
	DECLARE_EVENT(EventType_KeyTyped, EventCategory_Input | EventCategory_Keyboard);

	KeyTypedEvent(UInt32 InCharacter)
		: Event()
		, Character(InCharacter)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = " + GetPrintableCharacter();
	}

	FORCEINLINE const Char GetPrintableCharacter() const
	{
		return static_cast<Char>(Character);
	}

	UInt32 Character;
};

/*
* MouseMovedEvent
*/

struct MouseMovedEvent : public Event
{
	DECLARE_EVENT(EventType_MouseMoved, EventCategory_Input | EventCategory_Mouse);

	MouseMovedEvent(Int32 InX, Int32 InY)
		: Event()
		, x(InX)
		, y(InY)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = (" + std::to_string(x) + ", " + std::to_string(y) + ")";
	}

	Int32 x;
	Int32 y;
};

/*
* MousePressedEvent
*/

struct MousePressedEvent : public Event
{
	DECLARE_EVENT(EventType_MousePressed, EventCategory_Input | EventCategory_Mouse);

	MousePressedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: Event()
		, Button(InButton)
		, Modifiers(InModifiers)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = " + ::ToString(Button);
	}

	EMouseButton		Button;
	ModifierKeyState	Modifiers;
};

/*
* MouseReleasedEvent
*/

struct MouseReleasedEvent : public Event
{
	DECLARE_EVENT(EventType_MouseReleased, EventCategory_Input | EventCategory_Mouse);

	MouseReleasedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: Event()
		, Button(InButton)
		, Modifiers(InModifiers)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = " + ::ToString(Button);
	}

	EMouseButton		Button;
	ModifierKeyState	Modifiers;
};

/*
* MouseScrolledEvent
*/

struct MouseScrolledEvent : public Event
{
	DECLARE_EVENT(EventType_MouseScrolled, EventCategory_Input | EventCategory_Mouse);

	MouseScrolledEvent(Float InHorizontalDelta, Float InVerticalDelta)
		: Event()
		, HorizontalDelta(InHorizontalDelta)
		, VerticalDelta(InVerticalDelta)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = (" + std::to_string(VerticalDelta) + ", " + std::to_string(HorizontalDelta) + ")";
	}

	Float HorizontalDelta;
	Float VerticalDelta;
};

/*
* WindowResizeEvent
*/

struct WindowResizeEvent : public Event
{
	DECLARE_EVENT(EventType_WindowResized, EventCategory_Window);

	WindowResizeEvent(const TSharedRef<GenericWindow>& InWindow, UInt16 InWidth, UInt16 InHeight)
		: Event()
		, Window(InWindow)
		, Width(InWidth)
		, Height(InHeight)
	{
	}

	virtual std::string ToString() const override final
	{
		return std::string(GetTypeAsString()) + " = (" + std::to_string(Width) + ", " + std::to_string(Height) + ")";
	}

	TSharedRef<GenericWindow> Window;
	UInt16 Width;
	UInt16 Height;
};