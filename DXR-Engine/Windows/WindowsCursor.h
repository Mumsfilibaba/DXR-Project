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

	virtual Bool Init(const CursorCreateInfo& InCreateInfo) override final;

	virtual Void* GetNativeHandle() const override final
	{
		return reinterpret_cast<Void*>(hCursor);
	}

	FORCEINLINE HCURSOR GetCursor() const
	{
		return hCursor;
	}

private:
	WindowsApplication* Application;
	HCURSOR hCursor;
	LPCSTR	CursorName;
};