#include "VulkanRHI/RayTracing/VulkanRayTracingPipeline.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanStats.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

#if VK_KHR_ray_tracing_pipeline
static VkShaderStageFlagBits RayTracingShaderStageToVk(EShaderStage Stage)
{
    switch (Stage)
    {
        case EShaderStage::RayGen:          return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case EShaderStage::RayMiss:         return VK_SHADER_STAGE_MISS_BIT_KHR;
        case EShaderStage::RayClosestHit:   return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case EShaderStage::RayAnyHit:       return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case EShaderStage::RayIntersection: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case EShaderStage::RayCallable:     return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        default:                            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    }
}
#endif

FVulkanRayTracingPipelineStateRHI::FVulkanRayTracingPipelineStateRHI(FVulkanDevice* InDevice)
    : FRHIRayTracingPipelineState()
    , FVulkanPipeline(InDevice)
    , HandleSize(0)
    , HandleAlignment(0)
    , BaseAlignment(0)
    , MaxStride(0)
{
}

FVulkanRayTracingPipelineStateRHI::~FVulkanRayTracingPipelineStateRHI()
{
}

bool FVulkanRayTracingPipelineStateRHI::Initialize(const FRHIRayTracingPipelineStateDesc& InDesc)
{
#if VK_KHR_ray_tracing_pipeline
    if (!vkCreateRayTracingPipelinesKHR || !vkGetRayTracingShaderGroupHandlesKHR)
    {
        VULKAN_ERROR_CRITICAL("Ray tracing pipeline extension functions are not available");
        return false;
    }

    RayTracingPipelineFlags = InDesc.Flags;

    TArray<FVulkanRayTracingShader*>             StageShaders;
    TArray<VkPipelineShaderStageCreateInfo>      ShaderStages;
    TArray<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups;

    const auto AddStage = [&](FRHIRayTracingShader* RHIShader) -> uint32
    {
        FVulkanRayTracingShader* VulkanShader = GetVulkanRayTracingShader(RHIShader);
        if (!VulkanShader)
        {
            return VK_SHADER_UNUSED_KHR;
        }

        const uint32 StageIndex = static_cast<uint32>(StageShaders.Size());
        StageShaders.Add(VulkanShader);

        VkPipelineShaderStageCreateInfo StageInfo = {};
        StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        StageInfo.stage = RayTracingShaderStageToVk(RHIShader->GetShaderStage());
        StageInfo.pName = *VulkanShader->GetEntryPointName();

        ShaderStages.Add(StageInfo);
        return StageIndex;
    };

    FVulkanPipelineLayoutInfo LayoutInfo;

    const auto AddLayoutForShader = [&](FRHIRayTracingShader* RHIShader)
    {
        FVulkanRayTracingShader* VulkanShader = GetVulkanRayTracingShader(RHIShader);
        if (VulkanShader)
        {
            const VkShaderStageFlagBits StageFlag = RayTracingShaderStageToVk(RHIShader->GetShaderStage());
            LayoutInfo.MergeSetForStage(StageFlag, VulkanShader->GetShaderInfo());
            LayoutInfo.UpdateConstantsForStage(StageFlag, VulkanShader->GetShaderInfo());
        }
    };

    for (FRHIRayGenShader* RayGen : InDesc.RayGenShaders)
    {
        AddLayoutForShader(RayGen);
    }

    for (FRHIRayMissShader* Miss : InDesc.MissShaders)
    {
        AddLayoutForShader(Miss);
    }

    for (FRHIRayCallableShader* Callable : InDesc.CallableShaders)
    {
        AddLayoutForShader(Callable);
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : InDesc.HitGroups)
    {
        for (FRHIRayTracingShader* HitShader : HitGroup.Shaders)
        {
            AddLayoutForShader(HitShader);
        }
    }

#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    LayoutInfo.PromoteUniformBuffersToDynamic();
#endif
    LayoutInfo.GenerateHash();

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.FindOrCreateLayout(LayoutInfo);

    if (!PipelineLayout)
    {
        VULKAN_ERROR_CRITICAL("Failed to create ray tracing pipeline layout");
        return false;
    }

    // Ray-generation groups (general).
    for (FRHIRayGenShader* RayGen : InDesc.RayGenShaders)
    {
        const uint32 StageIndex = AddStage(RayGen);

        VkRayTracingShaderGroupCreateInfoKHR Group = {};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        Group.generalShader      = StageIndex;
        Group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        Group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.Add(Group);

        FVulkanRayTracingShader* VulkanRayGen = GetVulkanRayTracingShader(RayGen);
        
        const String RayGenName = VulkanRayGen ? VulkanRayGen->GetIdentifier() : String();
        GroupNames.Add(RayGenName);

        GetExportNameArray(ERayTracingShaderRecordKind::RayGeneration).Emplace(RayGenName);
    }

    // Miss groups (general).
    for (FRHIRayMissShader* Miss : InDesc.MissShaders)
    {
        const uint32 StageIndex = AddStage(Miss);

        VkRayTracingShaderGroupCreateInfoKHR Group = {};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        Group.generalShader      = StageIndex;
        Group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        Group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.Add(Group);

        FVulkanRayTracingShader* VulkanMiss = GetVulkanRayTracingShader(Miss);
        
        const String MissName = VulkanMiss ? VulkanMiss->GetIdentifier() : String();
        GroupNames.Add(MissName);

        GetExportNameArray(ERayTracingShaderRecordKind::Miss).Emplace(MissName);
    }

    // Hit groups (triangles / procedural).
    for (const FRHIRayTracingHitGroupInfo& HitGroup : InDesc.HitGroups)
    {
        VkRayTracingShaderGroupTypeKHR HitGroupType = (HitGroup.Type == ERayTracingHitGroupType::Procedural)
            ? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
            : VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

        VkRayTracingShaderGroupCreateInfoKHR Group = {};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = HitGroupType;
        Group.generalShader      = VK_SHADER_UNUSED_KHR;
        Group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        Group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;

        for (FRHIRayTracingShader* HitShader : HitGroup.Shaders)
        {
            if (!HitShader)
            {
                continue;
            }

            const uint32 StageIndex = AddStage(HitShader);
            switch (HitShader->GetShaderStage())
            {
                case EShaderStage::RayClosestHit:
                    Group.closestHitShader = StageIndex;
                    break;

                case EShaderStage::RayAnyHit:
                    Group.anyHitShader = StageIndex;
                    break;

                case EShaderStage::RayIntersection: 
                    Group.intersectionShader = StageIndex;
                    break;

                default: 
                    break;
            }
        }

        ShaderGroups.Add(Group);
        GroupNames.Add(HitGroup.Name);

        GetExportNameArray(ERayTracingShaderRecordKind::HitGroup).Emplace(HitGroup.Name);
    }

    // Callable groups (general).
    for (FRHIRayCallableShader* Callable : InDesc.CallableShaders)
    {
        const uint32 StageIndex = AddStage(Callable);

        VkRayTracingShaderGroupCreateInfoKHR Group = {};
        Group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        Group.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        Group.generalShader      = StageIndex;
        Group.closestHitShader   = VK_SHADER_UNUSED_KHR;
        Group.anyHitShader       = VK_SHADER_UNUSED_KHR;
        Group.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.Add(Group);

        FVulkanRayTracingShader* VulkanCallable = GetVulkanRayTracingShader(Callable);

        const String CallableName = VulkanCallable ? VulkanCallable->GetIdentifier() : String();
        GroupNames.Add(CallableName);

        GetExportNameArray(ERayTracingShaderRecordKind::Callable).Emplace(CallableName);
    }

    // Resolve shader modules now that the layout exists (binding patching needs it).
    for (int32 Index = 0; Index < StageShaders.Size(); ++Index)
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = StageShaders[Index]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStages[Index].module = ShaderModule->GetVkShaderModule();
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ray tracing shader module");
            return false;
        }
    }

    VkRayTracingPipelineCreateInfoKHR PipelineCreateInfo = {};
    PipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    PipelineCreateInfo.stageCount                   = static_cast<uint32>(ShaderStages.Size());
    PipelineCreateInfo.pStages                      = ShaderStages.Data();
    PipelineCreateInfo.groupCount                   = static_cast<uint32>(ShaderGroups.Size());
    PipelineCreateInfo.pGroups                      = ShaderGroups.Data();
    PipelineCreateInfo.maxPipelineRayRecursionDepth = Math::Max<uint32>(InDesc.MaxRecursionDepth, 1);
    PipelineCreateInfo.layout                       = PipelineLayout->GetVkPipelineLayout();

    VkResult Result = vkCreateRayTracingPipelinesKHR(
        GetDevice()->GetVkDevice(), 
        VK_NULL_HANDLE, 
        GetDevice()->GetPipelineStateManager().GetVkPipelineCache(), 
        1,
         &PipelineCreateInfo, 
         nullptr, 
         &Pipeline);

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create ray tracing pipeline");
        return false;
    }

    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& RTProperties = GetDevice()->GetPhysicalDevice()->GetRayTracingPipelineProperties();
    HandleSize      = RTProperties.shaderGroupHandleSize;
    HandleAlignment = RTProperties.shaderGroupHandleAlignment;
    BaseAlignment   = RTProperties.shaderGroupBaseAlignment;
    MaxStride       = RTProperties.maxShaderGroupStride;

    const uint32 GroupCount = static_cast<uint32>(ShaderGroups.Size());
    if (HandleSize == 0 || GroupCount == 0)
    {
        VULKAN_ERROR_CRITICAL("Invalid ray tracing shader-group handle configuration");
        return false;
    }

    GroupHandleStorage.Resize(int32(HandleSize * GroupCount));

    Result = vkGetRayTracingShaderGroupHandlesKHR(GetDevice()->GetVkDevice(), Pipeline, 0, GroupCount, GroupHandleStorage.SizeInBytes(), GroupHandleStorage.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to query ray tracing shader-group handles");
        return false;
    }

    STAT_ADD(STAT_Vulkan_NumRayTracingPipelineStates, 1);
    return true;
#else
    UNREFERENCED_VARIABLE(InDesc);
    VULKAN_ERROR_CRITICAL("Ray tracing pipeline support is not compiled in");
    return false;
#endif
}

const uint8* FVulkanRayTracingPipelineStateRHI::GetShaderGroupHandle(const String& ExportName) const
{
    for (int32 Index = 0; Index < GroupNames.Size(); ++Index)
    {
        if (GroupNames[Index] == ExportName)
        {
            const int32 ByteOffset = Index * int32(HandleSize);
            if (ByteOffset + int32(HandleSize) <= GroupHandleStorage.Size())
            {
                return GroupHandleStorage.Data() + ByteOffset;
            }
        }
    }

    return nullptr;
}

void FVulkanRayTracingPipelineStateRHI::GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const
{
    const TArray<String>& Names = GetExportNameArray(Kind);
    if (Names.IsEmpty())
    {
        OutExportName = String();
        return;
    }

    if (RecordIndex < uint32(Names.Size()))
    {
        OutExportName = Names[int32(RecordIndex)];
        return;
    }

    OutExportName = (Kind == ERayTracingShaderRecordKind::HitGroup) ? Names[0] : String();
}

uint32 FVulkanRayTracingPipelineStateRHI::GetNumExportNames(ERayTracingShaderRecordKind Kind) const
{
    return uint32(GetExportNameArray(Kind).Size());
}

void* FVulkanRayTracingPipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetVkPipeline());
}

void FVulkanRayTracingPipelineStateRHI::SetDebugName(const String& InName)
{
    FVulkanPipeline::SetDebugName(InName);
#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#endif
}

void FVulkanRayTracingPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}
