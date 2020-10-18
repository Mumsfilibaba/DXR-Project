#pragma once
#include "Containers/TSharedPtr.h"

/*
* EngineGlobals
*/

struct EngineGlobals
{
	static TSharedPtr<class GenericApplication>		PlatformApplication;
	static TSharedPtr<class RenderingAPI>			RenderingAPI;
	static TSharedPtr<class CommandListExecutor>	CmdListExecutor;
};