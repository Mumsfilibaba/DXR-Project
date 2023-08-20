#include "VulkanShader.h"

FVulkanShader::FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InVisibility)
    : FVulkanDeviceObject(InDevice)
    , ShaderModule(VK_NULL_HANDLE)
    , Visibility(InVisibility)
{
}

FVulkanShader::~FVulkanShader()
{
    ShaderModule = VK_NULL_HANDLE;
}

bool FVulkanShader::Initialize(const TArray<uint8>& InCode)
{
    return true;
}
