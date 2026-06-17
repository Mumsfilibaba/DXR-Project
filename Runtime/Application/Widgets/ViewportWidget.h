#pragma once
#include "Application/IViewport.h"
#include "Application/Widgets/Widget.h"

enum class EViewportPositionSpace
{
    Parent,
    Screen,
};

class APPLICATION_API FViewportWidget : public FWidget
{
public:
    struct FInitializer
    {
        TSharedPtr<IViewport> ViewportInterface;
    };

public:
    FViewportWidget();
    virtual ~FViewportWidget();

    void Initialize(const FInitializer& Initializer);

    // FWidget Interface
    virtual void Tick(const FRectangle& AssignedBounds) override final;

    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override final;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnFocusLost() override final;
    virtual FEventResponse OnFocusGained() override final;

    void SetViewportInterface(const TSharedPtr<IViewport>& InViewportInterface)
    {
        ViewportInterface = InViewportInterface;
    }

    TSharedPtr<IViewport> GetViewportInterface()
    {
        return ViewportInterface;
    }

    TSharedPtr<const IViewport> GetViewportInterface() const
    {
        return ViewportInterface;
    }

     /**
     * @brief Sets the relative viewport size. This size will be clamped to the parent widget's size during Tick.
     * 
     * @param InSize The new size.
     */
    void SetSize(const IntVector2& InSize) { Size = InSize; }
    
    /**
     * @brief Sets the relative viewport position. This size will be clamped to the parent widget's size during Tick.
     * 
     * @param InPosition The new position.
     */
    void SetPosition(const IntVector2& InPosition, EViewportPositionSpace InSpace = EViewportPositionSpace::Parent);

    /**
     * @brief Gets the current viewport size.
     * 
     * @return The size of the viewport.
     */
    IntVector2 GetSize() const { return Size; }
    
    /**
     * @brief Gets the current viewport position.
     * 
     * @return The position of the viewport.
     */
    IntVector2 GetPosition() const { return Position; }

private:
    TSharedPtr<IViewport> ViewportInterface;
    IntVector2            Position;
    IntVector2            Size;
};
