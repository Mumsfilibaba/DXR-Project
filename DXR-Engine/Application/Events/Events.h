#pragma once
#include "Core.h"

#include "Application/Generic/GenericApplication.h"

#include <Containers/TSharedPtr.h>

enum class EEventType : UInt8
{ 
    Unknown = 0,

    KeyPressed  = 1,
    KeyReleased = 2,
    KeyTyped    = 3,

    MouseMoved    = 4,
    MousePressed  = 5,
    MouseReleased = 6,
    MouseScrolled = 7,

    WindowResized      = 8,
    WindowMoved        = 9,
    WindowMouseLeft    = 10,
    WindowMouseEntered = 11,
    WindowFocusChanged = 12,
    WindowClosed       = 13,
};

inline const Char* ToString(EEventType EventType)
{
    switch (EventType)
    {
    case EEventType::KeyPressed:         return "KeyPressed";
    case EEventType::KeyReleased:        return "KeyReleased";
    case EEventType::KeyTyped:           return "KeyTyped";
    case EEventType::MouseMoved:         return "MouseMoved";
    case EEventType::MousePressed:       return "MousePressed";
    case EEventType::MouseReleased:      return "MouseReleased";
    case EEventType::MouseScrolled:      return "MouseScrolled";
    case EEventType::WindowResized:      return "WindowResized";
    case EEventType::WindowMoved:        return "WindowMoved";
    case EEventType::WindowMouseLeft:    return "WindowMouseLeft";
    case EEventType::WindowMouseEntered: return "WindowMouseEntered";
    case EEventType::WindowFocusChanged: return "WindowFocusChanged";
    case EEventType::WindowClosed:       return "WindowClosed";
    }

    return "Unknown";
}

enum EEventCategory : UInt8
{
    Unknown = 0,
    
    Input    = BIT(1),
    Mouse    = BIT(2),
    Keyboard = BIT(3),
    Window   = BIT(4),

    All = 0xff
};

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

struct Event
{
    friend class EventDispatcher;

public:
    Event()
        : HasBeenHandled(false)
    {
    }

    virtual ~Event() = default;

    virtual std::string ToString() const = 0;

    virtual EEventType GetType() const  = 0;
    virtual const Char* GetTypeAsString() const = 0;
    virtual UInt8 GetCategoryFlags() const = 0;

    Bool IsHandled() const { return HasBeenHandled; }

    static EEventType GetStaticType()
    {
        return EEventType::EventType_Unknown;
    }

private:
    mutable Bool HasBeenHandled;
};

template<typename T>
TEnableIf<std::is_base_of_v<Event, T>, Bool> IsEventOfType(const Event& InEvent)
{
    return (InEvent.GetType() == T::GetStaticType());
}

template<typename T>
TEnableIf<std::is_base_of_v<Event, T>, T&> CastEvent(Event& InEvent)
{
    return static_cast<T&>(InEvent);
}

template<typename T>
TEnableIf<std::is_base_of_v<Event, T>, const T&> CastEvent(const Event& InEvent)
{
    return static_cast<const T&>(InEvent);
}

struct KeyPressedEvent : public Event
{
    DECLARE_EVENT(EventType_KeyPressed, EventCategory_Input | EventCategory_Keyboard);

    KeyPressedEvent(EKey InKey, Bool InIsRepeat, const ModifierKeyState& InModifiers)
        : Event()
        , Key(InKey)
        , IsRepeat(InIsRepeat)
        , Modifiers(InModifiers)
    {
    }

    virtual std::string ToString() const override final
    {
        return std::string(GetTypeAsString()) + " = " + ::ToString(Key) + ", IsRepeat = " + (IsRepeat ? "True" : "False");
    }

    EKey Key;
    Bool IsRepeat;
    ModifierKeyState Modifiers;
};

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

    EKey             Key;
    ModifierKeyState Modifiers;
};

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

    EMouseButton     Button;
    ModifierKeyState Modifiers;
};

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

    EMouseButton     Button;
    ModifierKeyState Modifiers;
};


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

struct WindowFocusChangedEvent : public Event
{
    DECLARE_EVENT(EventType_WindowFocusChanged, EventCategory_Window);

    WindowFocusChangedEvent(const TSharedRef<GenericWindow>& InWindow, Bool hasFocus)
        : Event()
        , Window(InWindow)
        , HasFocus(hasFocus)
    {
    }

    virtual std::string ToString() const override
    {
        return std::string("WindowFocusChangedEvent=") + std::to_string(HasFocus);
    }

    TSharedRef<GenericWindow> Window;
    Bool HasFocus;
};

struct WindowMovedEvent : public Event
{
    DECLARE_EVENT(EventType_WindowMoved, EventCategory_Window);
    
    WindowMovedEvent(const TSharedRef<GenericWindow>& InWindow, Int16 x, Int16 y)
        : Event()
        , Window(InWindow)
        , Position({ x, y })
    {
    }

    virtual std::string ToString() const override
    {
        return std::string("WindowMovedEvent=[x, ") + std::to_string(Position.x) + ", y=" + std::to_string(Position.y) + "]";
    }

    TSharedRef<GenericWindow> Window;
    struct
    {
        Int16 x;
        Int16 y;
    } Position;
};

struct WindowMouseLeftEvent : public Event
{
    DECLARE_EVENT(EventType_WindowMouseLeft, EventCategory_Window);
    
    WindowMouseLeftEvent(const TSharedRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowMouseLeftEvent";
    }

    TSharedRef<GenericWindow> Window;
};

struct WindowMouseEnteredEvent : public Event
{
    DECLARE_EVENT(EventType_WindowMouseEntered, EventCategory_Window);
    
    WindowMouseEnteredEvent(const TSharedRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowMouseEnteredEvent";
    }

    TSharedRef<GenericWindow> Window;
};

struct WindowClosedEvent : public Event
{
    DECLARE_EVENT(EventType_WindowClosed, EventCategory_Window);

    WindowClosedEvent(const TSharedRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowClosedEvent";
    }

    TSharedRef<GenericWindow> Window;
};