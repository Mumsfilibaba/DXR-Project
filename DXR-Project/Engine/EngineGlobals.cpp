#include "EngineGlobals.h"

/*
* EngineGlobals
*/

TSharedPtr<GenericApplication>	EngineGlobals::PlatformApplication	= nullptr;
TSharedPtr<RenderingAPI>		EngineGlobals::RenderingAPI			= nullptr;
TSharedPtr<CommandListExecutor>	EngineGlobals::CmdListExecutor		= nullptr;