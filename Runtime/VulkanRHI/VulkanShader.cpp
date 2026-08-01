#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanConstants.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "Core/Misc/CRC.h"

#include <spirv_cross_c.h>

namespace SpirvOps
{
    constexpr uint16 OpExtension              = 10;
    constexpr uint16 OpDecorate               = 71;
    constexpr uint16 OpMemberDecorate         = 72;
    constexpr uint16 OpDecorateString         = 5632;
    constexpr uint16 OpMemberDecorateString   = 5633;
    constexpr uint16 OpDecorateId             = 332;

    constexpr uint32 DecorationHlslCounterBufferGOOGLE = 5634;
    constexpr uint32 DecorationHlslSemanticGOOGLE      = 5635;
    constexpr uint32 DecorationUserTypeGOOGLE          = 5636;
}

static bool SpvReadLiteralString(const uint32* Inst, uint16 InstWords, uint16 StartWord, CHAR* OutBuf, uint32 BufSize)
{
    if (StartWord >= InstWords || BufSize == 0)
    {
        return false;
    }

    const CHAR*  Src      = reinterpret_cast<const CHAR*>(&Inst[StartWord]);
    const uint32 MaxBytes = (InstWords - StartWord) * sizeof(uint32);
    
    uint32 i = 0;
    for (; i < MaxBytes && i < (BufSize - 1); ++i)
    {
        OutBuf[i] = Src[i];
        if (Src[i] == '\0')
        {
            return true;
        }
    }

    OutBuf[i < BufSize ? i : BufSize - 1] = '\0';
    return false;
}

static bool IsOneOfGoogleExtensions(const CHAR* Name)
{
    return (CString::Strcmp(Name, "SPV_GOOGLE_decorate_string") == 0) || (CString::Strcmp(Name, "SPV_GOOGLE_hlsl_functionality1") == 0) || 
        (CString::Strcmp(Name, "SPV_GOOGLE_user_type") == 0);
}

static bool IsGoogleDecorateStringDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslSemanticGOOGLE || DecorationId == SpirvOps::DecorationUserTypeGOOGLE;
}

static bool IsGoogleDecorateIdDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslCounterBufferGOOGLE;
}

static uint16 ComputeEffectiveRegister(uint32 OriginalSet, uint32 RawBinding)
{
    if (OriginalSet == VULKAN_RAY_TRACING_LOCAL_SET)
    {
        const uint32 EffectiveRegister = VULKAN_RAY_TRACING_LOCAL_REGISTER_BASE + RawBinding;
        VULKAN_ERROR("RT-local resource (register %u, space%u) force-mapped into the global set at register %u",
            RawBinding, VULKAN_RAY_TRACING_LOCAL_SET, EffectiveRegister);
        CHECK(EffectiveRegister < VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS);
        return static_cast<uint16>(EffectiveRegister);
    }
    else if (OriginalSet == VULKAN_SHADER_CONSTANTS_SET)
    {
        VULKAN_ERROR("Descriptor resource declared in space%u (reserved for 32-bit constants), register %u; "
            "this is unexpected - treating it as a global-space resource", VULKAN_SHADER_CONSTANTS_SET, RawBinding);
    }

    return static_cast<uint16>(RawBinding);
}

static EVulkanNullImageViewType GetNullImageViewType(spvc_compiler Compiler, spvc_type_id TypeId)
{
    const spvc_type Type      = spvc_compiler_get_type_handle(Compiler, TypeId);
    const SpvDim    Dimension = spvc_type_get_image_dimension(Type);
    const bool      bArrayed  = spvc_type_get_image_arrayed(Type) != SPVC_FALSE;

    if (Dimension == SpvDimCube)
    {
        return bArrayed ? EVulkanNullImageViewType::TextureCubeArray : EVulkanNullImageViewType::TextureCube;
    }

    return bArrayed ? EVulkanNullImageViewType::Texture2DArray : EVulkanNullImageViewType::Texture2D;
}

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

FVulkanShader::FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility::Type InShaderVisibility)
    : FVulkanDeviceChild(InDevice)
    , ShaderVisibility(InShaderVisibility)
{
}

FVulkanShader::~FVulkanShader()
{
    TScopedLock Lock(ShaderModulesCS);
    ShaderModules.Clear();
}

FVulkanRayTracingShader::FVulkanRayTracingShader(FVulkanDevice* InDevice)
    : FVulkanShader(InDevice, EShaderVisibility::RayTracing)
{
}

FVulkanRayTracingShader::~FVulkanRayTracingShader() = default;

bool FVulkanShader::Initialize(const TArray<uint8>& InCode)
{
    if (InCode.Size() % sizeof(uint32) != 0)
    {
        VULKAN_ERROR_CRITICAL("SPIR-V code is not aligned properly, ensure that the code is valid SPIR-V");
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

    // Cache per (set index + resolved final bindings). This is stable for non-RT shaders (each owns its
    // set, so the bindings never change), but distinguishes an RT shader reused across pipelines that
    // assign it different merged bindings - those must not alias to the same VkShaderModule.

    uint64 ModuleKey = DescriptorSetIndex;
    for (const FVulkanShaderInfo::FResourceBinding& Binding : ShaderInfo.ResourceBindings)
    {
        uint32 RemappedBinding = 0;
        const bool bFound = Layout->GetRemappedBinding(ShaderVisibility, Binding.BindingType, Binding.OriginalBindingIndex, RemappedBinding);
        HashCombine(ModuleKey, bFound ? RemappedBinding : static_cast<uint32>(Binding.BindingIndex));
    }
    
    {
        TScopedLock Lock(ShaderModulesCS);

        // Find the ShaderModule with the matching resolved bindings
        if (TSharedRef<FVulkanShaderModule>* ShaderModule = ShaderModules.Find(ModuleKey))
        {
            return *ShaderModule;
        }
    }
    
    // Patch the SPIR-V code with the correct DescriptorSetIndex + merged binding numbers
    FSpirvArray PatchedCode;
    if (!PatchShaderBindings(PatchedCode, Layout, DescriptorSetIndex))
    {
        VULKAN_ERROR_CRITICAL("Failed to get resource bindings");
        return nullptr;
    }
    
    if (PatchedCode.IsEmpty())
    {
        VULKAN_ERROR_CRITICAL("Patched code is empty");
        return nullptr;
    }

    FSpirvArray StrippedCode;
    if (!StripGoogleSpirvRequirements(PatchedCode, StrippedCode))
    {
        VULKAN_ERROR_CRITICAL("Failed to strip Google SPIR-V requirements");
        return nullptr;
    }

    String GoogleValidationError;
    if (!ValidateNoGoogleSpirvRequirements(StrippedCode, &GoogleValidationError))
    {
        VULKAN_ERROR_CRITICAL("Google SPIR-V requirements remain after stripping: %s", *GoogleValidationError);
        return nullptr;
    }

    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode    = StrippedCode.Data();
    ShaderModuleCreateInfo.codeSize = StrippedCode.SizeInBytes();

    VkShaderModule ShaderModule = VK_NULL_HANDLE;

    VkResult Result = vkCreateShaderModule(GetDevice()->GetVkDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
        return nullptr;
    }
    else
    {
        TScopedLock Lock(ShaderModulesCS);

		if (TSharedRef<FVulkanShaderModule>* Existing = ShaderModules.Find(ModuleKey))
		{
		    // Another thread won the race; destroy the newly created VkShaderModule and reuse the existing shared ref.
			vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
		    return *Existing;
		}

        TSharedRef<FVulkanShaderModule> NewShaderModule = new FVulkanShaderModule(GetDevice(), ShaderModule);
        ShaderModules.Add(ModuleKey, NewShaderModule);
        return NewShaderModule;
    }
}

bool FVulkanShader::PatchShaderBindings(FSpirvArray& OutSpirv, FVulkanPipelineLayout* Layout, uint32 DescriptorSetIndex)
{
    if (SpirvCode.IsEmpty())
    {
        VULKAN_ERROR_CRITICAL("No SPIR-V code supplied");
        return false;
    }

    CHECK(Layout != nullptr);
    CHECK(ShaderInfo.BindingOffsets.Size() == ShaderInfo.ResourceBindings.Size());

    FSpirvArray PatchedCode = SpirvCode;

    // BindingOffsets is index-aligned with ResourceBindings. Resolve the final binding number from the
    // (merged) layout and write both decorations. For non-RT shaders this resolves to the same dense
    // BindingIndex as before. For ray tracing it resolves to the shared merged binding.

    for (int32 Index = 0; Index < ShaderInfo.BindingOffsets.Size(); Index++)
    {
        const FVulkanShaderInfo::FBindingOffsets& Offsets  = ShaderInfo.BindingOffsets[Index];
        const FVulkanShaderInfo::FResourceBinding& Binding = ShaderInfo.ResourceBindings[Index];

        CHECK(Offsets.BindingOffset       != UINT32_MAX);
        CHECK(Offsets.DescriptorSetOffset != UINT32_MAX);

        uint32 RemappedBinding = 0;
        const bool bFound = Layout->GetRemappedBinding(ShaderVisibility, Binding.BindingType, Binding.OriginalBindingIndex, RemappedBinding);
        CHECK(ShaderVisibility == EShaderVisibility::RayTracing || (bFound && RemappedBinding == Binding.BindingIndex));

        const uint32 FinalBinding = bFound ? RemappedBinding : Binding.BindingIndex;

        PatchedCode[Offsets.BindingOffset]       = FinalBinding;
        PatchedCode[Offsets.DescriptorSetOffset] = DescriptorSetIndex;
    }

    for (const FVulkanShaderInfo::FBindingOffsets& Offsets : ShaderInfo.HeapBindingOffsets)
    {
        CHECK(Offsets.DescriptorSetOffset != UINT32_MAX);
        PatchedCode[Offsets.DescriptorSetOffset] = VULKAN_BINDLESS_RUNTIME_SET_INDEX;
    }

    OutSpirv = Move(PatchedCode);
    return true;
}

bool FVulkanShader::InitializeShaderLayout()
{
    if (SpirvCode.IsEmpty())
    {
        VULKAN_ERROR_CRITICAL("No SPIR-V code supplied");
        return false;
    }
 
    spvc_context Context = nullptr;
    spvc_result Result = spvc_context_create(&Context);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR_CRITICAL("Failed to create SpvcContext");
        return false;
    }

    spvc_context_set_error_callback(Context, [](void*, const CHAR* Error)
    {
        UNREFERENCED_VARIABLE(Error);
        VULKAN_ERROR("[SPIRV-Cross Error] %s", Error);
    }, nullptr);

    // The code size needs to be aligned to the elementsize
    spvc_parsed_ir ParsedCode = nullptr;
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(SpirvCode.Data()), SpirvCode.Size(), &ParsedCode);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR_CRITICAL("Failed to parse Spirv");
        spvc_context_destroy(Context);
        return false;
    }

    spvc_compiler Compiler = nullptr;
    Result = spvc_context_create_compiler(Context, SPVC_BACKEND_GLSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &Compiler);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR_CRITICAL("Failed to create SPIR-V compiler");
        spvc_context_destroy(Context);
        return false;
    }

    const spvc_entry_point* EntryPoints = nullptr;
    size_t NumEntryPoints = 0;
    Result = spvc_compiler_get_entry_points(Compiler, &EntryPoints, &NumEntryPoints);
    if (Result == SPVC_SUCCESS && NumEntryPoints > 0)
    {
        EntryPointName = EntryPoints[0].name;
    }
    else
    {
        EntryPointName = "main";
    }

    spvc_resources ShaderResources;
    Result = spvc_compiler_create_shader_resources(Compiler, &ShaderResources);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR_CRITICAL("Failed to create shader resources");
        spvc_context_destroy(Context);
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
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, SampledImages[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_RESOURCE_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_RESOURCE_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = EVulkanBindingType::SampledImage;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.NullViewType         = GetNullImageViewType(Compiler, SampledImages[Index].base_type_id);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding));

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = spvc_compiler_get_name(Compiler, SampledImages[Index].base_type_id);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }

    // Samplers
    size_t NumSamplers = 0;
    const spvc_reflected_resource* Samplers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS, &Samplers, &NumSamplers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumSamplers; Index++)
        {
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, Samplers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_SAMPLER_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Sampler at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_SAMPLER_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = EVulkanBindingType::Sampler;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding));

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = spvc_compiler_get_name(Compiler, Samplers[Index].base_type_id);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }

    // UAV Textures
    size_t NumStorageImages = 0;
    const spvc_reflected_resource* StorageImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &StorageImages, &NumStorageImages) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumStorageImages; Index++)
        {
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageImages[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_RESOURCE_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_RESOURCE_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = EVulkanBindingType::StorageImage;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.NullViewType         = GetNullImageViewType(Compiler, StorageImages[Index].base_type_id);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding));

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = spvc_compiler_get_name(Compiler, StorageImages[Index].base_type_id);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }

    // ConstantBuffers
    size_t NumUniformBuffers = 0;
    const spvc_reflected_resource* UniformBuffers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &UniformBuffers, &NumUniformBuffers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumUniformBuffers; Index++)
        {
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_RESOURCE_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_RESOURCE_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = EVulkanBindingType::UniformBuffer;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding));

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = spvc_compiler_get_name(Compiler, UniformBuffers[Index].base_type_id);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }

    // SRV + UAV Buffers
    size_t NumStorageBuffers = 0;
    const spvc_reflected_resource* StorageBuffers = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &StorageBuffers, &NumStorageBuffers) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumStorageBuffers; Index++)
        {
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_RESOURCE_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_RESOURCE_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding));

            const String BaseTypeName = spvc_compiler_get_name(Compiler, StorageBuffers[Index].base_type_id);

            const bool bIsUAV = BaseTypeName.Contains("RWStructuredBuffer");
            if (bIsUAV)
            {
                Binding.BindingType = EVulkanBindingType::StorageBufferReadWrite;
            }
            else
            {
                Binding.BindingType = EVulkanBindingType::StorageBufferRead;
            }

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = Move(BaseTypeName);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }

    // Acceleration Structures
    size_t NumAccelerationStructures = 0;
    const spvc_reflected_resource* AccelerationStructures = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE, &AccelerationStructures, &NumAccelerationStructures) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumAccelerationStructures; Index++)
        {
            const uint32 OriginalSet = spvc_compiler_get_decoration(Compiler, AccelerationStructures[Index].id, SpvDecorationDescriptorSet);

            uint32 BindingOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, AccelerationStructures[Index].id, SpvDecorationBinding, &BindingOffset))
            {
                BindingOffset = UINT32_MAX;
            }

            uint32 DescriptorSetOffset = UINT32_MAX;
            if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, AccelerationStructures[Index].id, SpvDecorationDescriptorSet, &DescriptorSetOffset))
            {
                DescriptorSetOffset = UINT32_MAX;
            }

            if (OriginalSet == VULKAN_BINDLESS_HEAP_MARKER_SET)
            {
                const uint32 OriginalBinding = spvc_compiler_get_decoration(Compiler, AccelerationStructures[Index].id, SpvDecorationBinding);
                if (OriginalBinding != VULKAN_BINDLESS_RESOURCE_BINDING)
                {
                    VULKAN_ERROR_CRITICAL("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.",
                        VULKAN_BINDLESS_HEAP_MARKER_SET, VULKAN_BINDLESS_RESOURCE_BINDING, OriginalBinding, VULKAN_BINDLESS_HEAP_MARKER_SET);
                    spvc_context_destroy(Context);
                    return false;
                }

                ShaderInfo.HeapBindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
                continue;
            }

            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = EVulkanBindingType::AccelerationStructure;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = ComputeEffectiveRegister(OriginalSet, spvc_compiler_get_decoration(Compiler, AccelerationStructures[Index].id, SpvDecorationBinding));

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            Binding.DebugName = spvc_compiler_get_name(Compiler, AccelerationStructures[Index].id);
        #endif

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }
    else
    {
        spvc_context_destroy(Context);
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
                NumPushBytes = Math::Max(NumPushBytes, StructSize);
            }
            else
            {
                DEBUG_BREAK();
                spvc_context_destroy(Context);
                return false;
            }
        }

        // TODO: We try and align all constants to a vec4/float4 since we do this in D3D12, check if this is necessary
        MAYBE_UNUSED constexpr size_t MaxBytes = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
        constexpr size_t Alignment = sizeof(float) * 4;
                
        //size_t NumPushBytes = RangeOffset + Range;
        CHECK(NumPushBytes <= MaxBytes);
        NumPushBytes = Math::AlignUp(NumPushBytes, Alignment);
        CHECK(NumPushBytes <= MaxBytes);

        // After we have aligned the bytes we convert into NumShaderConstants, i.e number of uint32's
        ShaderInfo.NumPushConstants = Math::AlignUp<uint32>(static_cast<uint32>(NumPushBytes), sizeof(uint32)) / sizeof(uint32);
        CHECK(ShaderInfo.NumPushConstants <= VULKAN_MAX_NUM_PUSH_CONSTANTS);
    }
    else
    {
        spvc_context_destroy(Context);
        return false;
    }
    
    // Do NOT bake binding/set numbers here. At reflection time the shader is seen in isolation, so the final numbers are unknown.
    for (int32 Index = 0; Index < ShaderInfo.BindingOffsets.Size(); Index++)
    {
        CHECK(ShaderInfo.BindingOffsets[Index].BindingOffset       != UINT32_MAX);
        CHECK(ShaderInfo.BindingOffsets[Index].DescriptorSetOffset != UINT32_MAX);
    }
    
    spvc_context_destroy(Context);
    return true;
}

bool FVulkanShader::StripGoogleSpirvRequirements(const FSpirvArray& InWords, FSpirvArray& OutWords)
{
    OutWords.Clear();

    if (InWords.Size() < 5)
    {
        return false;
    }

    OutWords.Reserve(InWords.Size());

    for (uint32 i = 0; i < 5; ++i)
    {
        OutWords.Add(InWords[i]);
    }

    const uint32* Words     = InWords.Data();
    const uint32  WordCount = static_cast<uint32>(InWords.Size());

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint32 FirstWord = Words[Read];
        const uint16 OpCode    = static_cast<uint16>(FirstWord & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(FirstWord >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            return false;
        }

        const uint32* Inst = &Words[Read];

        bool bSkip = false;
        if (OpCode == SpirvOps::OpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    bSkip = true;
                }
            }
        }

        if (!bSkip && (OpCode == SpirvOps::OpDecorateString || OpCode == SpirvOps::OpMemberDecorateString))
        {
            if (OpCode == SpirvOps::OpDecorateString && InstWords >= 3 && IsGoogleDecorateStringDecoration(Inst[2]))
            {
                bSkip = true;
            }
            else if (OpCode == SpirvOps::OpMemberDecorateString && InstWords >= 4 && IsGoogleDecorateStringDecoration(Inst[3]))
            {
                bSkip = true;
            }
        }

        if (!bSkip && (OpCode == SpirvOps::OpDecorate || OpCode == SpirvOps::OpDecorateId))
        {
            if (InstWords >= 3 && IsGoogleDecorateIdDecoration(Inst[2]))
            {
                bSkip = true;
            }
        }

        if (!bSkip && OpCode == SpirvOps::OpMemberDecorate)
        {
            if (InstWords >= 4 && IsGoogleDecorateIdDecoration(Inst[3]))
            {
                bSkip = true;
            }
        }

        if (!bSkip)
        {
            for (uint16 w = 0; w < InstWords; ++w)
            {
                OutWords.Add(Inst[w]);
            }
        }

        Read += InstWords;
    }

    return true;
}

bool FVulkanShader::ValidateNoGoogleSpirvRequirements(const FSpirvArray& Words, String* OutErrorMessage)
{
    if (Words.Size() < 5)
    {
        return true;
    }

    const uint32* Data      = Words.Data();
    const uint32  WordCount = static_cast<uint32>(Words.Size());

    uint32 Read = 5;
    while (Read < WordCount)
    {
        const uint32 FirstWord = Data[Read];
        const uint16 OpCode    = static_cast<uint16>(FirstWord & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(FirstWord >> 16);

        if (InstWords == 0 || (Read + InstWords) > WordCount)
        {
            break;
        }

        const uint32* Inst = &Data[Read];
        if (OpCode == SpirvOps::OpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    if (OutErrorMessage)
                    {
                        *OutErrorMessage = String::CreateFormatted("Found Google extension: %s", ExtName);
                    }

                    return false;
                }
            }
        }

        Read += InstWords;
    }

    return true;
}

FVulkanVertexShaderRHI::FVulkanVertexShaderRHI(FVulkanDevice* InDevice)
    : FRHIVertexShader()
    , FVulkanShader(InDevice, EShaderVisibility::Vertex)
{
}

FVulkanVertexShaderRHI::~FVulkanVertexShaderRHI() = default;

FVulkanHullShaderRHI::FVulkanHullShaderRHI(FVulkanDevice* InDevice)
    : FRHIHullShader()
    , FVulkanShader(InDevice, EShaderVisibility::Hull)
{
}

FVulkanHullShaderRHI::~FVulkanHullShaderRHI() = default;

FVulkanDomainShaderRHI::FVulkanDomainShaderRHI(FVulkanDevice* InDevice)
    : FRHIDomainShader()
    , FVulkanShader(InDevice, EShaderVisibility::Domain)
{
}

FVulkanDomainShaderRHI::~FVulkanDomainShaderRHI() = default;

FVulkanGeometryShaderRHI::FVulkanGeometryShaderRHI(FVulkanDevice* InDevice)
    : FRHIGeometryShader()
    , FVulkanShader(InDevice, EShaderVisibility::Geometry)
{
}

FVulkanGeometryShaderRHI::~FVulkanGeometryShaderRHI() = default;

FVulkanPixelShaderRHI::FVulkanPixelShaderRHI(FVulkanDevice* InDevice)
    : FRHIPixelShader()
    , FVulkanShader(InDevice, EShaderVisibility::Pixel)
{
}

FVulkanPixelShaderRHI::~FVulkanPixelShaderRHI() = default;

FVulkanMeshShaderRHI::FVulkanMeshShaderRHI(FVulkanDevice* InDevice)
    : FRHIMeshShader()
    , FVulkanShader(InDevice, EShaderVisibility::Mesh)
{
}

FVulkanMeshShaderRHI::~FVulkanMeshShaderRHI() = default;

FVulkanAmplificationShaderRHI::FVulkanAmplificationShaderRHI(FVulkanDevice* InDevice)
    : FRHIAmplificationShader()
    , FVulkanShader(InDevice, EShaderVisibility::Task)
{
}

FVulkanAmplificationShaderRHI::~FVulkanAmplificationShaderRHI() = default;

FVulkanRayGenShaderRHI::FVulkanRayGenShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayGenShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayGenShaderRHI::~FVulkanRayGenShaderRHI() = default;

FVulkanRayAnyHitShaderRHI::FVulkanRayAnyHitShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayAnyHitShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayAnyHitShaderRHI::~FVulkanRayAnyHitShaderRHI() = default;

FVulkanRayClosestHitShaderRHI::FVulkanRayClosestHitShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayClosestHitShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayClosestHitShaderRHI::~FVulkanRayClosestHitShaderRHI() = default;

FVulkanRayMissShaderRHI::FVulkanRayMissShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayMissShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayMissShaderRHI::~FVulkanRayMissShaderRHI() = default;

FVulkanComputeShaderRHI::FVulkanComputeShaderRHI(FVulkanDevice* InDevice)
    : FRHIComputeShader()
    , FVulkanShader(InDevice, EShaderVisibility::Compute)
{
}

FVulkanComputeShaderRHI::~FVulkanComputeShaderRHI() = default;

void* FVulkanVertexShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanHullShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanDomainShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanGeometryShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanPixelShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanMeshShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanAmplificationShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayGenShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayAnyHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayClosestHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayMissShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanComputeShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanVertexShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanHullShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanDomainShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanGeometryShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanPixelShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanMeshShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanAmplificationShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

void* FVulkanRayGenShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

void* FVulkanRayAnyHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

void* FVulkanRayClosestHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

void* FVulkanRayMissShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

void* FVulkanComputeShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

