#include "EngineGlobals.h"

/*
* Application
*/

class GenericApplication* GlobalPlatformApplication = nullptr;

/*
* Rendering
*/

class GenericRenderingAPI*	GlobalRenderingAPI		= nullptr;
class IShaderCompiler*		GlobalShaderCompiler	= nullptr;
class ICommandContext*		GlobalCommandContext	= nullptr;