#pragma once
#include <Containers/Types.h>

/*
* Application
*/

extern class GenericApplication* GlobalPlatformApplication;

/*
* Rendering
*/

extern class Renderer*				GlobalRenderer;
extern class GenericRenderingAPI*	GlobalRenderingAPI;
extern class IShaderCompiler*		GlobalShaderCompiler;

extern Bool GlobalPrePassEnabled;
extern Bool GlobalDrawAABBs;
extern Bool GlobalVSyncEnabled;
extern Bool GlobalFrustumCullEnabled;
extern Bool GlobalFXAAEnabled;
extern Bool GlobalRayTracingEnabled;
extern Bool GlobalSSAOEnabled;