#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include "VulkanPipelineLayout.h"
#include "Core/Misc/CRC.h"
#include <spirv_cross_c.h>

FVulkanDevice* FVulkanShaderModule::StaticDevice = nullptr;

FVulkanShaderModule::FVulkanShaderModule(FVulkanDevice* InDevice, VkShaderModule InShaderModule)
    : ShaderModule(InShaderModule)
{
    if (!StaticDevice)
    {
        StaticDevice = InDevice;
    }
}

FVulkanShaderModule::~FVulkanShaderModule()
{
    if (VULKAN_CHECK_HANDLE(ShaderModule))
    {
        vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
        ShaderModule = VK_NULL_HANDLE;
    }
}

FVulkanShader::FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility)
    : FVulkanDeviceChild(InDevice)
    , ShaderVisibility(InShaderVisibility)
{
}

FVulkanShader::~FVulkanShader()
{
    TScopedLock Lock(ShaderModulesCS);
    ShaderModules.Clear();
}

bool FVulkanShader::Initialize(const TArray<uint8>& InCode)
{
    if (InCode.Size() % sizeof(uint32) != 0)
    {
        VULKAN_ERROR("SPIR-V code is not aligned properly, ensure that the code is valid SPIR-V");
        return false;
    }

    const int32 CodeSize = InCode.Size() / sizeof(uint32);
    SpirvCode = FSpirvArray(reinterpret_cast<const uint32*>(InCode.Data()), CodeSize);
    if (!InitializeShaderLayout())
    {
        return false;
    }
    else
    {
        return true;
    }
}

TSharedRef<FVulkanShaderModule> FVulkanShader::GetOrCreateShaderModule(FVulkanPipelineLayout* Layout)
{
    CHECK(Layout != nullptr);
    
    // Retrieve the DescriptorSetIndex, if there are no descriptors in this shader, then just use zero
    uint32 DescriptorSetIndex = 0;
    if (!ShaderInfo.ResourceBindings.IsEmpty())
    {
        if (!Layout->GetDescriptorSetIndex(ShaderVisibility, DescriptorSetIndex))
        {
            return nullptr;
        }
    }
    
    {
        TScopedLock Lock(ShaderModulesCS);

        // Find the ShaderModule with the correct DescriptorSetIndex
        if (TSharedRef<FVulkanShaderModule>* ShaderModule = ShaderModules.Find(DescriptorSetIndex))
        {
            return *ShaderModule;
        }
    }
    
    // Patch the SPIR-V code with the correct DescriptorSetIndex
    FSpirvArray PatchedCode;
    if (!PatchShaderBindings(PatchedCode, DescriptorSetIndex))
    {
        VULKAN_ERROR("Failed to get resource bindings");
        return nullptr;
    }
    
    if (PatchedCode.IsEmpty())
    {
        VULKAN_ERROR("Patched code is empty");
        return nullptr;
    }

    VkShaderModuleCreateInfo ShaderModuleCreateInfo;
    FMemory::Memzero(&ShaderModuleCreateInfo);

    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode    = PatchedCode.Data();
    ShaderModuleCreateInfo.codeSize = PatchedCode.SizeInBytes();

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    VkResult Result = vkCreateShaderModule(GetDevice()->GetVkDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create ShaderModule");
        return nullptr;
    }
    else
    {
        TScopedLock Lock(ShaderModulesCS);

        TSharedRef<FVulkanShaderModule> NewShaderModule = new FVulkanShaderModule(GetDevice(), ShaderModule);
        ShaderModules.Add(DescriptorSetIndex, NewShaderModule);
        return NewShaderModule;
    }
}

bool FVulkanShader::PatchShaderBindings(FSpirvArray& OutSpirv, uint32 DescriptorSetIndex)
{
    if (SpirvCode.IsEmpty())
    {
        VULKAN_ERROR("No SPIR-V code supplied");
        return false;
    }
 
    // Remap the necessary bindings
    FSpirvArray PatchedCode = SpirvCode;
    for (FVulkanShaderInfo::FBindingOffsets& Offsets : ShaderInfo.BindingOffsets)
    {
        CHECK(Offsets.DescriptorSetOffset != UINT32_MAX);
        PatchedCode[Offsets.DescriptorSetOffset] = DescriptorSetIndex;
    }
    
    OutSpirv = Move(PatchedCode);
    return true;
}

bool FVulkanShader::InitializeShaderLayout()
{
    if (SpirvCode.IsEmpty())
    {
        VULKAN_ERROR("No SPIR-V code supplied");
        return false;
    }
 
    spvc_context Context = nullptr;
    spvc_result Result = spvc_context_create(&Context);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR("Failed to create SpvcContext");
        return false;
    }

    spvc_context_set_error_callback(Context, [](void*, const CHAR* Error)
    {
        VULKAN_ERROR("[SPIRV-Cross Error] %s", Error);
    }, nullptr);

    // The code size needs to be aligned to the elementsize
    spvc_parsed_ir ParsedCode = nullptr;
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(SpirvCode.Data()), SpirvCode.Size(), &ParsedCode);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR("Failed to parse Spirv");
        return false;
    }

    spvc_compiler Compiler = nullptr;
    Result = spvc_context_create_compiler(Context, SPVC_BACKEND_GLSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &Compiler);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR("Failed to create SPIR-V compiler");
        return false;
    }

    spvc_resources ShaderResources;
    Result = spvc_compiler_create_shader_resources(Compiler, &ShaderResources);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR("Failed to create shader resources");
        return false;
    }

    // Use global binding for Vulkan shaders
    uint32 GlobalBinding = 0;

    // SRV Textures
    size_t NumSampledImages = 0;
    const spvc_reflected_resource* SampledImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, &SampledImages, &NumSampledImages) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumSampledImages; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType  = BindingType_SampledImage;
            Binding.BindingIndex = GlobalBinding++;
            Binding.OriginalBindingIndex = spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding);
            
            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding, &BindingOffset))
                BindingOffset = UINT32_MAX;
            
            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, SampledImages[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
                DescriptorSetOffset = UINT32_MAX;

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, SampledImages[Index].base_type_id);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        return false;
    }

    // Samplers
    size_t NumSamplers = 0;
    const spvc_reflected_resource* Samplers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS, &Samplers, &NumSamplers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumSamplers; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType  = BindingType_Sampler;
            Binding.BindingIndex = GlobalBinding++;
            Binding.OriginalBindingIndex = spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding);
            
            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding, &BindingOffset))
                BindingOffset = UINT32_MAX;
            
            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, Samplers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
                DescriptorSetOffset = UINT32_MAX;

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, Samplers[Index].base_type_id);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        return false;
    }

    // UAV Textures
    size_t NumStorageImages = 0;
    const spvc_reflected_resource* StorageImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &StorageImages, &NumStorageImages) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumStorageImages; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType  = BindingType_StorageImage;
            Binding.BindingIndex = GlobalBinding++;
            Binding.OriginalBindingIndex = spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding);
            
            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding, &BindingOffset))
                BindingOffset = UINT32_MAX;
            
            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageImages[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
                DescriptorSetOffset = UINT32_MAX;
            
            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, StorageImages[Index].base_type_id);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        return false;
    }

    // ConstantBuffers
    size_t NumUniformBuffers = 0;
    const spvc_reflected_resource* UniformBuffers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &UniformBuffers, &NumUniformBuffers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumUniformBuffers; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType  = BindingType_UniformBuffer;
            Binding.BindingIndex = GlobalBinding++;
            Binding.OriginalBindingIndex = spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding);
            
            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding, &BindingOffset))
                BindingOffset = UINT32_MAX;
            
            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
                DescriptorSetOffset = UINT32_MAX;

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, UniformBuffers[Index].base_type_id);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        return false;
    }

    // SRV + UAV Buffers
    size_t NumStorageBuffers = 0;
    const spvc_reflected_resource* StorageBuffers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &StorageBuffers, &NumStorageBuffers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumStorageBuffers; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingIndex = GlobalBinding++;
            Binding.OriginalBindingIndex = spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding);
            
            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding, &BindingOffset))
                BindingOffset = UINT32_MAX;
            
            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
                DescriptorSetOffset = UINT32_MAX;

            FString BaseTypeName = spvc_compiler_get_name(Compiler, StorageBuffers[Index].base_type_id);
            const bool bIsUAV = BaseTypeName.Contains("RWStructuredBuffer");
            if (bIsUAV)
            {
                Binding.BindingType = BindingType_StorageBufferReadWrite;
            }
            else
            {
                Binding.BindingType = BindingType_StorageBufferRead;
            }

            // Set debug-name
            Binding.DebugName = Move(BaseTypeName);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        return false;
    }

    // Push Constants
    size_t NumPushConstants = 0;
    const spvc_reflected_resource* PushConstants = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT, &PushConstants, &NumPushConstants) == SPVC_SUCCESS)
    {
        size_t NumPushBytes = 0;
        for (uint32 Index = 0; Index < NumPushConstants; Index++)
        {
            size_t StructSize  = 0;
            spvc_type type = spvc_compiler_get_type_handle(Compiler, PushConstants[Index].base_type_id);
            if (spvc_compiler_get_declared_struct_size(Compiler, type, &StructSize) == SPVC_SUCCESS)
            {
                NumPushBytes = FMath::Max(NumPushBytes, StructSize);
                VULKAN_INFO("    StructSize=%d", StructSize);
            }
            else
            {
                DEBUG_BREAK();
                return false;
            }
        }

        // TODO: We try and align all constants to a vec4/float4 since we do this in D3D12, check if this is necessary
        constexpr size_t MaxBytes  = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
        constexpr size_t Alignment = sizeof(float) * 4;
                
        //size_t NumPushBytes = RangeOffset + Range;
        CHECK(NumPushBytes <= MaxBytes);
        NumPushBytes = FMath::AlignUp(NumPushBytes, Alignment);
        CHECK(NumPushBytes <= MaxBytes);

        // After we have aligned the bytes we convert into Num32BitConstants, i.e number of uint32's
        ShaderInfo.NumPushConstants = FMath::AlignUp<uint32>(static_cast<uint32>(NumPushBytes), sizeof(uint32)) / sizeof(uint32);
        CHECK(ShaderInfo.NumPushConstants <= VULKAN_MAX_NUM_PUSH_CONSTANTS);
    }
    else
    {
        return false;
    }
    
    // Remap the necessary bindings
    for (int32 Index = 0; Index < ShaderInfo.BindingOffsets.Size(); Index++)
    {
        FVulkanShaderInfo::FBindingOffsets& Offsets = ShaderInfo.BindingOffsets[Index];
        CHECK(Offsets.BindingOffset != UINT32_MAX);
        CHECK(Offsets.DescriptorSetOffset != UINT32_MAX);
        
        // Since all the bindings will be the same no matter what DescriptorSetIndex, only change the BindingIndex
        FVulkanShaderInfo::FResourceBinding& Binding = ShaderInfo.ResourceBindings[Index];
        SpirvCode[Offsets.BindingOffset]       = Binding.BindingIndex;
        SpirvCode[Offsets.DescriptorSetOffset] = 0;
    }
    
    return true;
}
