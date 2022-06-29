#include "RHIModule.h"
#include "RHICoreInterface.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CDefaultEngineModule, RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Globals

RHI_API CRHICoreInterface*  GRHIInstance    = nullptr;
RHI_API IRHIShaderCompiler* GShaderCompiler = nullptr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

static FRHIModule* LoadNullRHI()
{
    return CModuleManager::Get().LoadEngineModule<FRHIModule>("NullRHI");
}

bool RHIInitialize(ERHIInstanceType InRenderApi)
{
    // Load Selected RHI
    FRHIModule* RHIModule = nullptr;
    if (InRenderApi == ERHIInstanceType::D3D12)
    {
        RHIModule = CModuleManager::Get().LoadEngineModule<FRHIModule>("D3D12RHI");
    }
	else if (InRenderApi == ERHIInstanceType::Metal)
	{
		RHIModule = CModuleManager::Get().LoadEngineModule<FRHIModule>("MetalRHI");
	}
    else if (InRenderApi == ERHIInstanceType::Null)
    {
        RHIModule = LoadNullRHI();
    }

    if (!RHIModule)
    {
        LOG_WARNING("[InitRHI] Failed to load RHI, trying to load NullRHI");

        RHIModule = LoadNullRHI();
        if (!RHIModule)
        {
            LOG_ERROR("[InitRHI] Failed to load RHI and fallback, the application has to terminate");
            return false;
        }
    }

    // TODO: This should be in EngineConfig and/or CCommandLine
    const bool bEnableDebug =
#if ENABLE_API_DEBUGGING
        true;
#else
        false;
#endif

    CRHICoreInterface* RHIInterface = RHIModule->CreateInterface();
    if (!(RHIInterface && RHIInterface->Initialize(bEnableDebug)))
    {
        LOG_ERROR("[InitRHI] Failed to init RHIInterface, the application has to terminate");
        return false;
    }

    GRHIInstance = RHIInterface;

    IRHIShaderCompiler* Compiler = RHIModule->CreateCompiler();
    if (!Compiler)
    {
        LOG_ERROR("[InitRHI] Failed to init RHIShaderCompiler, the application has to terminate");
        return false;
    }

    GShaderCompiler = Compiler;

    // Set the context to the command queue
    IRHICommandContext* CmdContext = RHIGetDefaultCommandContext();
    CRHICommandQueue::Get().SetContext(CmdContext);

    return true;
}

void RHIRelease()
{
    CRHICommandQueue::Get().SetContext(nullptr);

    if (GRHIInstance)
    {
        GRHIInstance->Destroy();
        GRHIInstance = nullptr;
    }

    SafeDelete(GShaderCompiler);
}
