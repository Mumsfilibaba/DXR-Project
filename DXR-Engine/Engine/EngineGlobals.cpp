#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "RenderingCore/CommandList.h"

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
class GenericRenderLayer*	GlobalRenderLayer		= nullptr;
class IShaderCompiler*		GlobalShaderCompiler	= nullptr;

CommandListExecutor	GlobalCmdListExecutor;

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

Profiler	GlobalProfiler;
Console		GlobalConsole;

Bool GlobalProfilerEnabled		= false;
Bool GlobalDrawProfiler			= false;
Bool GlobalDrawRendererInfo		= true;
Bool GlobalDrawTextureDebugger	= false;
