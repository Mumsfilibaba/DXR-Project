#pragma once
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Generic/GenericCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacCursor

class CMacCursor final : public CGenericCursor
{
private:

    CMacCursor()
        : CGenericCursor()
    { }

	~CMacCursor() = default;

public:

	static CMacCursor* CreateMacCursor();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericCursor Interface
	
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(CGenericWindow* RelativeWindow, int32 x, int32 y) const override final;

    virtual void GetPosition(CGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bVisible) override final;
};