#include "VulkanRHI.h"
#include "VulkanInstance.h"

IMPLEMENT_ENGINE_MODULE(CVulkanRHI, VulkanRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRHI

CRHIInstance* CVulkanRHI::CreateInterface()
{
    return CVulkanInstance::CreateInstance();
}

IRHIShaderCompiler* CVulkanRHI::CreateCompiler()
{
    return nullptr;
}
