#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanSamplerStateRHI::FVulkanSamplerStateRHI(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FVulkanDeviceChild(InDevice)
    , Sampler(VK_NULL_HANDLE)
    , BindlessHandle()
{
}

FVulkanSamplerStateRHI::~FVulkanSamplerStateRHI()
{
    if (BindlessHandle.IsValid())
    {
        if (FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
        {
            BindlessManager->Free(BindlessHandle);
        }
        
        BindlessHandle = FRHIDescriptorHandle();
    }

    Sampler = VK_NULL_HANDLE;
}

void* FVulkanSamplerStateRHI::GetRHINativeSampler() const
{
    return reinterpret_cast<void*>(Sampler);
}

FRHIDescriptorHandle FVulkanSamplerStateRHI::GetBindlessHandle() const
{
    FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return FRHIDescriptorHandle();
    }

    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    if (!VULKAN_CHECK_HANDLE(Sampler))
    {
        return FRHIDescriptorHandle();
    }

    BindlessHandle = BindlessManager->Allocate(EDescriptorType::Sampler);
    if (!BindlessHandle.IsValid())
    {
        return FRHIDescriptorHandle();
    }

    BindlessManager->EnqueueSamplerWrite(BindlessHandle, Sampler);
    return BindlessHandle;
}

bool FVulkanSamplerStateRHI::Initialize()
{
    if (!GetDevice()->FindOrCreateSampler(Desc, Sampler))
    {
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }

    return true;
}
