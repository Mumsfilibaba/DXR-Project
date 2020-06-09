#pragma once
#include "Types.h"

#pragma warning(push)
#pragma warning(disable : 4100)

class WindowsWindow;

class GenericEventHandler
{
public:
	virtual ~GenericEventHandler() = default;

	virtual void OnWindowResize(WindowsWindow* Window, Uint16 Width, Uint16 Height)
	{
	}

protected:
	GenericEventHandler() = default;
};

#pragma warning(pop)