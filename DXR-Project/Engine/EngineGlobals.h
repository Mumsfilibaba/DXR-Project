#pragma once
#include "Containers/TSharedPtr.h"

/*
* EngineGlobals
*/

struct EngineGlobals
{
	// Application
	static TSharedPtr<class GenericApplication> PlatformApplication;
	// Graphics
	static TSharedPtr<class GenericRenderingAPI> RenderingAPI;
	static TSharedPtr<class CommandListExecutor> CmdListExecutor;
};