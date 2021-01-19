#pragma once
#include <Containers/Types.h>

/*
* Engine
*/

extern class EngineLoop GlobalEngineLoop;

/*
* Application
*/

extern class GenericWindow*			GlobalMainWindow;
extern class GenericApplication*	GlobalPlatformApplication;
extern class EventDispatcher*		GlobalEventDispatcher;
extern class GenericOutputDevice*	GlobalConsoleOutput;

/*
* Game
*/

extern class Game* GlobalGame;

/*
* Rendering
*/

extern class Renderer*				GlobalRenderer;
extern class GenericRenderLayer*	GlobalRenderLayer;
extern class IShaderCompiler*		GlobalShaderCompiler;
extern class CommandListExecutor	GlobalCmdListExecutor;

extern Bool GlobalPrePassEnabled;
extern Bool GlobalDrawAABBs;
extern Bool GlobalVSyncEnabled;
extern Bool GlobalFrustumCullEnabled;
extern Bool GlobalFXAAEnabled;
extern Bool GlobalRayTracingEnabled;
extern Bool GlobalSSAOEnabled;

/*
* Debug
*/

extern class Profiler GlobalProfiler;

extern Bool GlobalProfilerEnabled;
extern Bool GlobalDrawProfiler;
extern Bool GlobalDrawRendererInfo;
extern Bool GlobalDrawTextureDebugger;