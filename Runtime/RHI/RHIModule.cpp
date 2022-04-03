#include "RHIModule.h"
#include "RHIInstance.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CDefaultEngineModule, RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Globals

RHI_API CRHIInstance*       GRHIInstance    = nullptr;
RHI_API IRHIShaderCompiler* GShaderCompiler = nullptr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

static CRHIModuleInterface* LoadNullRHI()
{
    return CModuleManager::Get().LoadEngineModule<CRHIModuleInterface>("NullRHI");
}

bool RHIInitialize(ERHIType InRenderApi)
{
    // Load Selected RHI
    CRHIModuleInterface* RHIModule = nullptr;
    if (InRenderApi == ERHIType::D3D12)
    {
        RHIModule = CModuleManager::Get().LoadEngineModule<CRHIModuleInterface>("D3D12RHI");
    }
    else if (InRenderApi == ERHIType::Vulkan)
    {
        RHIModule = CModuleManager::Get().LoadEngineModule<CRHIModuleInterface>("VulkanRHI");
    }
    else if (InRenderApi == ERHIType::Null)
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

    CRHIInstance* RHIInterface = RHIModule->CreateInterface();
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
    IRHICommandContext* CmdContext = GRHIInstance->GetDefaultCommandContext();
    CRHICommandExecutionManager::Get().SetContext(CmdContext);

    return true;
}

void RHIRelease()
{
    CRHICommandExecutionManager::Get().SetContext(nullptr);

    if (GRHIInstance)
    {
        GRHIInstance->Destroy();
        GRHIInstance = nullptr;
    }

    SafeDelete(GShaderCompiler);
}
