#pragma once
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"

class COREAPPLICATION_API FMacCursor final : public IPlatformCursor
{
public:
    FMacCursor();
    virtual ~FMacCursor();

    // IPlatformCursor Interface
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(int32 x, int32 y) override final;
    virtual IntVector2 GetPosition() const override final;

    virtual void SetVisibility(bool bVisible) override final;

    virtual bool IsVisible() const override final
    {
        return bIsVisible;
    }

    void UpdateCursorPosition(const IntVector2& InPosition);

private:
    IntVector2 CurrentPosition;
    ECursor    CurrentCursor;
    bool       bIsVisible;
    bool       bIsPositionInitialized;
    bool       bIsCursorInitialized;
};

