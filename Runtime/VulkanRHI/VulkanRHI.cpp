#include "VulkanRHI.h"
#include "RHIInstanceVulkan.h"
#include "VulkanShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CVulkanRHI, VulkanRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRHI

CRHIInstance* CVulkanRHI::CreateInterface()
{
    return CRHIInstanceVulkan::CreateInstance();
}

IRHIShaderCompiler* CVulkanRHI::CreateCompiler()
{
    return dbg_new CVulkanShaderCompiler();
}
