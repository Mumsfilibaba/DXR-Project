#pragma once
#include "Containers/TSharedPtr.h"

/*
* EngineGlobals
*/

struct EngineGlobals
{
	// Application
	static TSharedPtr<class GenericApplication> PlatformApplication;
};