#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Interface/PlatformCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacCursor - Mac specific implementation for cursor interface

class CMacCursor final : public CPlatformCursor
{
public:

    static TSharedPtr<CMacCursor> CreateCursor();
    
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const override final;
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bVisible) override final;

private:

    friend struct TDefaultDelete<CMacCursor>;
    
    CMacCursor()
        : CPlatformCursor()
    { }
    
    ~CMacCursor() = default;
};

#endif
