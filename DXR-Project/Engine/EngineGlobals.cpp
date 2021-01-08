#include "EngineGlobals.h"

/*
* Application
*/

class GenericApplication* GlobalPlatformApplication = nullptr;

/*
* Rendering
*/

class Renderer*				GlobalRenderer			= nullptr;
class GenericRenderingAPI*	GlobalRenderingAPI		= nullptr;
class IShaderCompiler*		GlobalShaderCompiler	= nullptr;