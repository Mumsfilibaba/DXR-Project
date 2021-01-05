#pragma once
#include "Event.h"

#include "Application/Generic/GenericApplication.h"

/*
* Base KeyEvent
*/

struct KeyEvent : public Event
{
public:
	KeyEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: Event()
		, Key(InKey)
		, Modifiers(InModifiers)
	{
	}

	DECLARE_EVENT_CATEGORY(EEventCategory::EventCategory_Input | EEventCategory::EventCategory_Keyboard);

	FORCEINLINE EKey GetKey() const
	{
		return Key;
	}

	FORCEINLINE const ModifierKeyState& GetModifierState() const
	{
		return Modifiers;
	}

private:
	EKey Key;
	ModifierKeyState Modifiers;
};

/*
* KeyPressedEvent
*/

struct KeyPressedEvent : public KeyEvent
{
public:
	KeyPressedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: KeyEvent(InKey, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(EventType_KeyPressed);

	virtual std::string ToString() const override
	{
		return std::string("KeyPressedEvent=") + KeyToString(GetKey());
	}
};

/*
* KeyReleasedEvent
*/

struct KeyReleasedEvent : public KeyEvent
{
public:
	KeyReleasedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: KeyEvent(InKey, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(EventType_KeyReleased);

	virtual std::string ToString() const override
	{
		return std::string("KeyReleasedEvent=") + KeyToString(GetKey());
	}
};

/*
* KeyTypedEvent
*/

struct KeyTypedEvent : public Event
{
public:
	KeyTypedEvent(UInt32 InCharacter)
		: Event()
		, Character(InCharacter)
	{
	}

	DECLARE_EVENT_CLASS(EventType_KeyTyped);

	DECLARE_EVENT_CATEGORY(EEventCategory::EventCategory_Input | EEventCategory::EventCategory_Keyboard);

	virtual std::string ToString() const override
	{
		return std::string("KeyTypedEvent=") + GetPrintableCharacter();
	}

	FORCEINLINE UInt32 GetCharacter() const
	{
		return Character;
	}

	FORCEINLINE const Char GetPrintableCharacter() const
	{
		return static_cast<Char>(Character);
	}

private:
	UInt32 Character;
};