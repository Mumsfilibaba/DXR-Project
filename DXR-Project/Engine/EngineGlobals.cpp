#include "EngineGlobals.h"

#include "Application/Generic/GenericApplication.h"

#include "RenderingCore/GenericRenderingAPI.h"
#include "RenderingCore/CommandList.h"

/*
* EngineGlobals
*/

// Application
TSharedPtr<GenericApplication> EngineGlobals::PlatformApplication = nullptr;
// Graphics
TSharedPtr<GenericRenderingAPI> EngineGlobals::RenderingAPI		= nullptr;
TSharedPtr<CommandListExecutor> EngineGlobals::CmdListExecutor	= nullptr;