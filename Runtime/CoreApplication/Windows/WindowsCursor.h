#pragma once

#if PLATFORM_WINDOWS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Interface/PlatformCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsCursor - Windows-specific interface for cursor

class COREAPPLICATION_API CWindowsCursor final : public CPlatformCursor
{
public:

    /**
     * Create Windows cursor interface
     * 
     * @return: Returns the newly created cursor-interface
     */
    static TSharedPtr<CWindowsCursor> CreateCursor();

    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const override final;
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bIsVisible) override final;

private:

    friend struct TDefaultDelete<CWindowsCursor>;

    CWindowsCursor()  = default;
    ~CWindowsCursor() = default;
};

#endif