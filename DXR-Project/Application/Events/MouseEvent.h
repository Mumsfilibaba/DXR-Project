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
		: X(InX)
		, Y(InY)
	{
	}

	DECLARE_EVENT_CLASS(MOUSE_MOVED_EVENT);

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_INPUT | EEventCategory::EVENT_CATEGORY_MOUSE);

	virtual std::string ToString() const override
	{
		return std::string("MouseMovedEvent=[") + std::to_string(X) + ", " + std::to_string(Y) + "]";
	}

	FORCEINLINE Int32 GetX() const
	{
		return X;
	}

	FORCEINLINE Int32 GetY() const
	{
		return Y;
	}

private:
	Int32 X;
	Int32 Y;
};

/*
* Base MouseButtonEvent
*/

struct MouseButtonEvent : public Event
{
public:
	MouseButtonEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
		: Button(InButton)
		, Modifiers(InModifiers)
	{
	}

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_INPUT | EEventCategory::EVENT_CATEGORY_MOUSE);

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

	DECLARE_EVENT_CLASS(MOUSE_PRESSED_EVENT);

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

	DECLARE_EVENT_CLASS(MOUSE_RELEASED_EVENT);

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
		: HorizontalDelta(InHorizontalDelta)
		, VerticalDelta(InVerticalDelta)
	{
	}

	DECLARE_EVENT_CLASS(MOUSE_SCROLLED_EVENT);

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_INPUT | EEventCategory::EVENT_CATEGORY_MOUSE);

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