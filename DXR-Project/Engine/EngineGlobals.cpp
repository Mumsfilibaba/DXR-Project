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

Bool GlobalPrePassEnabled		= true;
Bool GlobalDrawAABBs			= false;
Bool GlobalVSyncEnabled			= false;
Bool GlobalFrustumCullEnabled	= true;
Bool GlobalFXAAEnabled			= true;
Bool GlobalRayTracingEnabled	= false;
Bool GlobalSSAOEnabled			= true;