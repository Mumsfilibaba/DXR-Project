#pragma once
#include "Application/Generic/GenericCursor.h"

#include "Windows.h"

class WindowsApplication;

/*
* WindowsCursor
*/
class WindowsCursor : public GenericCursor
{
public:
	WindowsCursor(WindowsApplication* InApplication);
	~WindowsCursor();

	virtual bool Initialize(const CursorInitializer& InInitializer) override final;

	virtual VoidPtr GetNativeHandle() const override final
	{
		return reinterpret_cast<VoidPtr>(hCursor);
	}

	FORCEINLINE HCURSOR GetCursor() const
	{
		return hCursor;
	}

private:
	WindowsApplication* Application;
	HCURSOR hCursor;
	LPCSTR CursorName;
};