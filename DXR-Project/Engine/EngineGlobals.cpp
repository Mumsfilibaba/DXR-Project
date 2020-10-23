#include "EngineGlobals.h"

#include "Application/Generic/GenericApplication.h"

#include "RenderingCore/RenderingAPI.h"
#include "RenderingCore/CommandList.h"

/*
* EngineGlobals
*/

TSharedPtr<GenericApplication>	EngineGlobals::PlatformApplication	= nullptr;
TSharedPtr<RenderingAPI>		EngineGlobals::RenderingAPI			= nullptr;
TSharedPtr<CommandListExecutor>	EngineGlobals::CmdListExecutor		= nullptr;