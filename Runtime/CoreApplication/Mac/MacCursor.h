#pragma once
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericCursor.h"

class COREAPPLICATION_API FMacCursor final : public FGenericCursor
{
public:
    FMacCursor();
    ~FMacCursor();

    virtual void SetCursor(ECursor Cursor) override final;
    virtual void SetPosition(int32 x, int32 y) override final;
    virtual FIntVector2 GetPosition() const override final;
    virtual void SetVisibility(bool bVisible) override final;
    
    // The cursor is not read from the Platform directly since we are most likely operating in a
    // seperate thread from the main-thread where the events are sent from. In order to get around
    // this, the cursor is updated when we process mouse events, until then we are using the
    // precious cursor position whenever we request the position within the application.
    void UpdateCursorPosition(const FIntVector2& Position);
    
private:
    FIntVector2 CurrentPosition;
    bool        bIsPositionInitialized;
};
