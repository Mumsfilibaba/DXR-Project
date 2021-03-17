#pragma once
#include "Events.h"
#include "EventHandler.h"

#include "Application/Generic/GenericApplicationEventHandler.h"

#include "Core/Containers/Array.h"

class EventDispatcher : public GenericApplicationEventHandler
{
    struct EventHandlerPair
    {
        EventHandlerPair(IEventHandler* InHandler, uint8 InCategoryMask)
            : CategoryMask(InCategoryMask)
            , IsHandlerFunc(false)
        {
            Handler = InHandler;
        }

        EventHandlerPair(EventHandlerFunc InFunc, uint8 InCategoryMask)
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

        uint8 CategoryMask  = 0;
        bool  IsHandlerFunc = false;
    };

public:
    EventDispatcher(class GenericApplication* InApplication);
    ~EventDispatcher() = default;

    void RegisterEventHandler(EventHandlerFunc Func, uint8 EventCategoryMask = EEventCategory::EventCategory_All);
    void RegisterEventHandler(IEventHandler* EventHandler, uint8 EventCategoryMask = EEventCategory::EventCategory_All);
    
    void UnregisterEventHandler(EventHandlerFunc Func);
    void UnregisterEventHandler(IEventHandler* EventHandler);

    bool SendEvent(const Event& Event);

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
    virtual void OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseMove(int32 x, int32 y) override final;
    virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) override final;
    virtual void OnCharacterInput(uint32 Character) override final;
    virtual void OnWindowResized(const TRef<GenericWindow>& InWindow, uint16 Width, uint16 Height) override final;
    virtual void OnWindowMoved(const TRef<GenericWindow>& Window, int16 x, int16 y) override final;
    virtual void OnWindowFocusChanged(const TRef<GenericWindow>& Window, bool HasFocus) override final;
    virtual void OnWindowMouseLeft(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowMouseEntered(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowClosed(const TRef<GenericWindow>& Window) override final;

private:
    class GenericApplication* Application = nullptr;
    TArray<EventHandlerPair> EventHandlers;
};