#pragma once
#include "Core.h"

#include "Application/Generic/GenericApplication.h"

enum class EEventType : uint8
{ 
    Unknown = 0,
    // Keys
    KeyPressed  = 1,
    KeyReleased = 2,
    KeyTyped    = 3,
    // Mouse
    MouseMoved    = 4,
    MousePressed  = 5,
    MouseReleased = 6,
    MouseScrolled = 7,
    // Window
    WindowResized      = 8,
    WindowMoved        = 9,
    WindowMouseLeft    = 10,
    WindowMouseEntered = 11,
    WindowFocusChanged = 12,
    WindowClosed       = 13,
};

inline const char* ToString(EEventType EventType)
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

enum EEventCategory : uint8
{
    EventCategory_Unknown  = 0,
    EventCategory_Input    = BIT(1),
    EventCategory_Mouse    = BIT(2),
    EventCategory_Keyboard = BIT(3),
    EventCategory_Window   = BIT(4),
    EventCategory_All      = 0xff
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
    virtual const char* GetTypeAsString() const \
    { \
        return #Type; \
    } \
    virtual uint8 GetCategoryFlags() const override \
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
    virtual const char* GetTypeAsString() const = 0;
    virtual uint8 GetCategoryFlags() const = 0;

    bool IsHandled() const { return HasBeenHandled; }

    static EEventType GetStaticType()
    {
        return EEventType::Unknown;
    }

private:
    mutable bool HasBeenHandled;
};

template<typename T>
TEnableIf<std::is_base_of_v<Event, T>, bool> IsEventOfType(const Event& InEvent)
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
    DECLARE_EVENT(KeyPressed, EventCategory_Input | EventCategory_Keyboard);

    KeyPressedEvent(EKey InKey, bool InIsRepeat, const ModifierKeyState& InModifiers)
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
    bool IsRepeat;
    ModifierKeyState Modifiers;
};

struct KeyReleasedEvent : public Event
{
    DECLARE_EVENT(KeyReleased, EventCategory_Input | EventCategory_Keyboard);

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
    DECLARE_EVENT(KeyTyped, EventCategory_Input | EventCategory_Keyboard);

    KeyTypedEvent(uint32 InCharacter)
        : Event()
        , Character(InCharacter)
    {
    }

    virtual std::string ToString() const override final
    {
        return std::string(GetTypeAsString()) + " = " + GetPrintableCharacter();
    }

    FORCEINLINE const char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

    uint32 Character;
};

struct MouseMovedEvent : public Event
{
    DECLARE_EVENT(MouseMoved, EventCategory_Input | EventCategory_Mouse);

    MouseMovedEvent(int32 InX, int32 InY)
        : Event()
        , x(InX)
        , y(InY)
    {
    }

    virtual std::string ToString() const override final
    {
        return std::string(GetTypeAsString()) + " = (" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    int32 x;
    int32 y;
};

struct MousePressedEvent : public Event
{
    DECLARE_EVENT(MousePressed, EventCategory_Input | EventCategory_Mouse);

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
    DECLARE_EVENT(MouseReleased, EventCategory_Input | EventCategory_Mouse);

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
    DECLARE_EVENT(MouseScrolled, EventCategory_Input | EventCategory_Mouse);

    MouseScrolledEvent(float InHorizontalDelta, float InVerticalDelta)
        : Event()
        , HorizontalDelta(InHorizontalDelta)
        , VerticalDelta(InVerticalDelta)
    {
    }

    virtual std::string ToString() const override final
    {
        return std::string(GetTypeAsString()) + " = (" + std::to_string(VerticalDelta) + ", " + std::to_string(HorizontalDelta) + ")";
    }

    float HorizontalDelta;
    float VerticalDelta;
};

struct WindowResizeEvent : public Event
{
    DECLARE_EVENT(WindowResized, EventCategory_Window);

    WindowResizeEvent(const TRef<GenericWindow>& InWindow, uint16 InWidth, uint16 InHeight)
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

    TRef<GenericWindow> Window;
    uint16 Width;
    uint16 Height;
};

struct WindowFocusChangedEvent : public Event
{
    DECLARE_EVENT(WindowFocusChanged, EventCategory_Window);

    WindowFocusChangedEvent(const TRef<GenericWindow>& InWindow, bool hasFocus)
        : Event()
        , Window(InWindow)
        , HasFocus(hasFocus)
    {
    }

    virtual std::string ToString() const override
    {
        return std::string("WindowFocusChangedEvent=") + std::to_string(HasFocus);
    }

    TRef<GenericWindow> Window;
    bool HasFocus;
};

struct WindowMovedEvent : public Event
{
    DECLARE_EVENT(WindowMoved, EventCategory_Window);
    
    WindowMovedEvent(const TRef<GenericWindow>& InWindow, int16 x, int16 y)
        : Event()
        , Window(InWindow)
        , Position({ x, y })
    {
    }

    virtual std::string ToString() const override
    {
        return std::string("WindowMovedEvent=[x, ") + std::to_string(Position.x) + ", y=" + std::to_string(Position.y) + "]";
    }

    TRef<GenericWindow> Window;
    struct
    {
        int16 x;
        int16 y;
    } Position;
};

struct WindowMouseLeftEvent : public Event
{
    DECLARE_EVENT(WindowMouseLeft, EventCategory_Window);
    
    WindowMouseLeftEvent(const TRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowMouseLeftEvent";
    }

    TRef<GenericWindow> Window;
};

struct WindowMouseEnteredEvent : public Event
{
    DECLARE_EVENT(WindowMouseEntered, EventCategory_Window);
    
    WindowMouseEnteredEvent(const TRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowMouseEnteredEvent";
    }

    TRef<GenericWindow> Window;
};

struct WindowClosedEvent : public Event
{
    DECLARE_EVENT(WindowClosed, EventCategory_Window);

    WindowClosedEvent(const TRef<GenericWindow>& InWindow)
        : Event()
        , Window(InWindow)
    {
    }

    virtual std::string ToString() const override
    {
        return "WindowClosedEvent";
    }

    TRef<GenericWindow> Window;
};