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
		: Key(InKey)
		, Modifiers(InModifiers)
	{
	}

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_INPUT | EEventCategory::EVENT_CATEGORY_KEYBOARD);

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
class KeyPressedEvent : public KeyEvent
{
public:
	KeyPressedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: KeyEvent(InKey, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(KEY_PRESSED_EVENT);

	virtual std::string ToString() const override
	{
		return std::string("KeyPressedEvent=") + KeyToString(GetKey());
	}
};

/*
* KeyReleasedEvent
*/
class KeyReleasedEvent : public KeyEvent
{
public:
	KeyReleasedEvent(EKey InKey, const ModifierKeyState& InModifiers)
		: KeyEvent(InKey, InModifiers)
	{
	}

	DECLARE_EVENT_CLASS(KEY_RELEASED_EVENT);

	virtual std::string ToString() const override
	{
		return std::string("KeyReleasedEvent=") + KeyToString(GetKey());
	}
};

/*
* KeyTypedEvent
*/
class KeyTypedEvent : public Event
{
public:
	KeyTypedEvent(Uint32 InCharacter)
		: Character(InCharacter)
	{
	}

	DECLARE_EVENT_CLASS(KEY_TYPED_EVENT);

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_INPUT | EEventCategory::EVENT_CATEGORY_KEYBOARD);

	virtual std::string ToString() const override
	{
		return std::string("KeyTypedEvent=") + GetPrintableCharacter();
	}

	FORCEINLINE Uint32 GetCharacter() const
	{
		return Character;
	}

	FORCEINLINE const Char GetPrintableCharacter() const
	{
		return static_cast<Char>(Character);
	}

private:
	Uint32 Character;
};