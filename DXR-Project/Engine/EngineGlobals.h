#pragma once
#include "Containers/TSharedPtr.h"

class GenericApplication;

/*
* EngineGlobals
*/

struct EngineGlobals
{
	static TSharedPtr<GenericApplication> PlatformApplication;
};