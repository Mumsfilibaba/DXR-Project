#include "Core/Modules/ModuleManager.h"
#include "RendererCore/Interfaces/IRendererModule.h"

IMPLEMENT_ENGINE_MODULE(IModule, RendererCore);

RENDERERCORE_API IRendererModule* IRendererModule::RendererModule = nullptr;