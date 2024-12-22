#pragma once
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericCursor.h"

class COREAPPLICATION_API FMacCursor final : public FGenericCursor
{
public:
    FMacCursor();
    ~FMacCursor();

public:

    // ICursor Interface
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(int32 x, int32 y) override final;

    virtual FIntVector2 GetPosition() const override final;

    virtual void SetVisibility(bool bVisible) override final;

public:

    /**
     * @brief Updates the cursor's position based on processed mouse events.
     * 
     * The cursor position is not read directly from the platform because operations are likely
     * performed on a separate thread from the main UI thread where events are dispatched.
     * 
     * To handle this, the cursor position is updated when mouse events are processed. Until
     * then, the last known cursor position is used whenever the application requests the current
     * cursor position.
     * 
     * @param Position The new position to update the cursor with, represented as an FIntVector2.
     */
    void UpdateCursorPosition(const FIntVector2& Position);

private:
    FIntVector2 CurrentPosition;
    bool        bIsPositionInitialized;
};

