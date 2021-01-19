#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"

/*
* Engine
*/

EngineLoop GlobalEngineLoop;

/*
* Application
*/

class GenericWindow*		GlobalMainWindow			= nullptr;
class GenericApplication*	GlobalPlatformApplication	= nullptr;
class EventDispatcher*		GlobalEventDispatcher		= nullptr;
class GenericOutputDevice*	GlobalConsoleOutput			= nullptr;

/*
* Game
*/

class Game* GlobalGame = nullptr;

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

/*
* Debug
*/

Profiler GlobalProfiler;

Bool GlobalProfilerEnabled		= true;
Bool GlobalDrawProfiler			= true;
Bool GlobalDrawRendererInfo		= true;
Bool GlobalDrawTextureDebugger	= false;
