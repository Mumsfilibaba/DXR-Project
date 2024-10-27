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
    
    void UpdateCursorPosition(const FIntVector2& Position);
    
private:
    FIntVector2 CurrentPosition;
    bool        bIsPositionInitialized;
};
