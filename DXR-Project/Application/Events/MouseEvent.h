#pragma once
#include "Event.h"

#include "Application/Generic/GenericApplication.h"

/*
* MouseMovedEvent
*/

struct MouseMovedEvent : public Event
{
public:
	MouseMovedEvent(Int32 InX, Int32 InY)
		: Event()
		, x(InX)
		, y(InY)
	{
	}

	DECLARE_EVENT_CLASS(EventType_MouseMoved);

	DECLARE_EVENT_CATEGORY(EEventCategory::EventCategory_Input| EEventCategory::EventCategory_Mouse);

	virtual std::string ToString() const override
	{
		return std::string("MouseMovedEvent=[") + std::to_string(x) + ", " + std::to_string(y) + "]";
	}

	FORCEINLINE Int32 GetX() const
	{
		return x;
	}

	FORCEINLINE Int32 GetY() const
	{
		return y;
	}

private:
	Int32 x;
	Int32 y;
};

/*
* Base MouseButtonEvent
*/

struct MouseButtonEvent : public Event
{
public:
	MouseButtonEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: Event()
		, Button(InButton)
		, Modifiers(InModifiers)
	{
	}

	DECLARE_EVENT_CATEGORY(EEventCategory::EventCategory_Input | EEventCategory::EventCategory_Mouse);

	FORCEINLINE EMouseButton GetButton() const
	{
		return Button;
	}

	FORCEINLINE const ModifierKeyState& GetModifiers() const
	{
		return Modifiers;
	}

private:
	EMouseButton Button;
	ModifierKeyState Modifiers;
};

/*
* MousePressedEvent
*/

struct MousePressedEvent : public MouseButtonEvent
{
public:
	MousePressedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: MouseButtonEvent(InButton, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(EventType_MousePressed);

	virtual std::string ToString() const override
	{
		return std::string("MousePressedEvent=") + ButtonToString(GetButton());
	}
};

/*
* MouseReleasedEvent
*/

struct MouseReleasedEvent : public MouseButtonEvent
{
public:
	MouseReleasedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: MouseButtonEvent(InButton, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(EventType_MouseReleased);

	virtual std::string ToString() const override
	{
		return std::string("MouseReleasedEvent=") + ButtonToString(GetButton());
	}
};

/*
* MouseScrolledEvent
*/

struct MouseScrolledEvent : public Event
{
public:
	MouseScrolledEvent(Float InHorizontalDelta, Float InVerticalDelta)
		: Event()
		, HorizontalDelta(InHorizontalDelta)
		, VerticalDelta(InVerticalDelta)
	{
	}

	DECLARE_EVENT_CLASS(EventType_MouseScrolled);

	DECLARE_EVENT_CATEGORY(EEventCategory::EventCategory_Input | EEventCategory::EventCategory_Mouse);

	virtual std::string ToString() const override
	{
		return std::string("MouseScrolledEvent=[") + std::to_string(HorizontalDelta) + ", " + std::to_string(VerticalDelta) + "]";
	}

	FORCEINLINE Float GetHorizontalDelta() const
	{
		return HorizontalDelta;
	}

	FORCEINLINE Float GetVerticalDelta() const
	{
		return VerticalDelta;
	}

private:
	Float HorizontalDelta;
	Float VerticalDelta;
};