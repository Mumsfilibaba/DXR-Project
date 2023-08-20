#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"

FVulkanShader::FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InVisibility)
    : FVulkanDeviceObject(InDevice)
    , ShaderModule(VK_NULL_HANDLE)
    , Visibility(InVisibility)
{
}

FVulkanShader::~FVulkanShader()
{
    if (VULKAN_CHECK_HANDLE(ShaderModule))
    {
        vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
        ShaderModule = VK_NULL_HANDLE;
    }
}

bool FVulkanShader::Initialize(const TArray<uint8>& InCode)
{
    VkShaderModuleCreateInfo ShaderModuleCreateInfo;
    FMemory::Memzero(&ShaderModuleCreateInfo);

    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode    = reinterpret_cast<const uint32_t*>(InCode.Data());;
    ShaderModuleCreateInfo.codeSize = InCode.Size() / 4;

    VkResult Result = vkCreateShaderModule(GetDevice()->GetVkDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule);
    VULKAN_CHECK_RESULT(Result, "Failed to create ShaderModule");
    return true;
}
