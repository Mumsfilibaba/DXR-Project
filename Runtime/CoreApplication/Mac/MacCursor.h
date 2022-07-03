#pragma once
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Generic/GenericCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacCursor

class FMacCursor final : public FGenericCursor
{
private:

    FMacCursor()
        : FGenericCursor()
    { }

	~FMacCursor() = default;

public:

	static FMacCursor* CreateMacCursor();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericCursor Interface
	
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetPosition(FGenericWindow* RelativeWindow, int32 x, int32 y) const override final;

    virtual void GetPosition(FGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

    virtual void SetVisibility(bool bVisible) override final;
};