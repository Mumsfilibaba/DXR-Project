#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "RenderLayer/CommandList.h"


EngineLoop GlobalEngineLoop;

class GenericWindow*       GlobalMainWindow          = nullptr;
class GenericApplication*  GlobalPlatformApplication = nullptr;
class EventDispatcher*     GlobalEventDispatcher     = nullptr;
class GenericOutputDevice* GlobalConsoleOutput       = nullptr;

class Game* GlobalGame = nullptr;


class Renderer*           GlobalRenderer       = nullptr;
class GenericRenderLayer* GlobalRenderLayer    = nullptr;
class IShaderCompiler*    GlobalShaderCompiler = nullptr;

CommandListExecutor GlobalCmdListExecutor;

Bool GlobalPrePassEnabled     = true;
Bool GlobalDrawAABBs          = false;
Bool GlobalVSyncEnabled       = false;
Bool GlobalFrustumCullEnabled = true;
Bool GlobalRayTracingEnabled  = false;

Profiler GlobalProfiler;
Console  GlobalConsole;
