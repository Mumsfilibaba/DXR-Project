#include "VulkanRHI.h"
#include "VulkanInstance.h"
#include "VulkanShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CVulkanRHI, VulkanRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRHI

CRHIInstance* CVulkanRHI::CreateInterface()
{
    return CVulkanInstance::CreateInstance();
}

IRHIShaderCompiler* CVulkanRHI::CreateCompiler()
{
    return dbg_new CVulkanShaderCompiler();
}
