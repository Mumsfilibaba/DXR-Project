#pragma once
#include "Events.h"
#include "EventHandler.h"

#include "Application/Generic/GenericApplicationEventHandler.h"

#include <Containers/Array.h>

class EventDispatcher : public GenericApplicationEventHandler
{
    struct EventHandlerPair
    {
        EventHandlerPair(IEventHandler* InHandler, UInt8 InCategoryMask)
            : CategoryMask(InCategoryMask)
            , IsHandlerFunc(false)
        {
            Handler = InHandler;
        }

        EventHandlerPair(EventHandlerFunc InFunc, UInt8 InCategoryMask)
            : CategoryMask(InCategoryMask)
            , IsHandlerFunc(true)
        {
            Func = InFunc;
        }

        union
        {
            IEventHandler*   Handler;
            EventHandlerFunc Func;
        };

        UInt8 CategoryMask  = 0;
        Bool  IsHandlerFunc = false;
    };

public:
    EventDispatcher(class GenericApplication* InApplication);
    ~EventDispatcher() = default;

    void RegisterEventHandler(EventHandlerFunc Func, UInt8 EventCategoryMask = EEventCategory::EventCategory_All);
    void RegisterEventHandler(IEventHandler* EventHandler, UInt8 EventCategoryMask = EEventCategory::EventCategory_All);
    
    void UnregisterEventHandler(EventHandlerFunc Func);
    void UnregisterEventHandler(IEventHandler* EventHandler);

    Bool SendEvent(const Event& Event);

    FORCEINLINE void SetApplication(class GenericApplication* InApplication)
    {
        Application = InApplication;
    }

    FORCEINLINE class GenericApplication* GetApplication()
    {
        return Application;
    }

public:
    virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnKeyPressed(EKey KeyCode, Bool IsRepeat, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseMove(Int32 x, Int32 y) override final;
    virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta) override final;
    virtual void OnCharacterInput(UInt32 Character) override final;
    virtual void OnWindowResized(const TRef<GenericWindow>& InWindow, UInt16 Width, UInt16 Height) override final;
    virtual void OnWindowMoved(const TRef<GenericWindow>& Window, Int16 x, Int16 y) override final;
    virtual void OnWindowFocusChanged(const TRef<GenericWindow>& Window, Bool HasFocus) override final;
    virtual void OnWindowMouseLeft(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowMouseEntered(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowClosed(const TRef<GenericWindow>& Window) override final;

private:
    class GenericApplication* Application = nullptr;
    TArray<EventHandlerPair> EventHandlers;
};