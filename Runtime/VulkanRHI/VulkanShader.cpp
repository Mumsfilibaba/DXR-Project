#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include <glslang/Public/resource_limits_c.h> // Required for use of glslang_default_resource
#include <spirv_cross_c.h>

static uint32 GetShaderStageDescriporSetOffset(EShaderVisibility ShaderVisibility)
{
    switch (ShaderVisibility)
    {
        // Graphics
        case ShaderVisibility_Vertex:   return 0;
        case ShaderVisibility_Hull:     return 1;
        case ShaderVisibility_Domain:   return 2;
        case ShaderVisibility_Geometry: return 3;
        case ShaderVisibility_Pixel:    return 4;
        // Compute
        case ShaderVisibility_Compute:  return 0;
        // Other
        default: return 0;
    }
}

static glslang_stage_t GetGlslangStage(EShaderVisibility ShaderVisibility)
{
    switch (ShaderVisibility)
    {
        // Graphics
        case ShaderVisibility_Vertex:   return GLSLANG_STAGE_VERTEX;
        case ShaderVisibility_Hull:     return GLSLANG_STAGE_TESSCONTROL;
        case ShaderVisibility_Domain:   return GLSLANG_STAGE_TESSEVALUATION;
        case ShaderVisibility_Geometry: return GLSLANG_STAGE_GEOMETRY;
        case ShaderVisibility_Pixel:    return GLSLANG_STAGE_FRAGMENT;
        // Compute
        case ShaderVisibility_Compute:  return GLSLANG_STAGE_COMPUTE;
        // Other
        default: return glslang_stage_t(-1);
    }
}


FVulkanShader::FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility)
    : FVulkanDeviceChild(InDevice)
    , ShaderModule(VK_NULL_HANDLE)
    , ShaderVisibility(InShaderVisibility)
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
    if (!PatchShaderBindings(InCode, SpirvCode))
    {
        VULKAN_ERROR("Failed to get resource bindings");
        return false;
    }

    VkShaderModuleCreateInfo ShaderModuleCreateInfo;
    FMemory::Memzero(&ShaderModuleCreateInfo);

    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode    = reinterpret_cast<const uint32_t*>(SpirvCode.Data());
    ShaderModuleCreateInfo.codeSize = SpirvCode.Size();

    VkResult Result = vkCreateShaderModule(GetDevice()->GetVkDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create ShaderModule");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanShader::PatchShaderBindings(const TArray<uint8>& InCode, TArray<uint8>& OutCode)
{
    if (InCode.IsEmpty())
    {
        VULKAN_ERROR("No SPIR-V code supplied");
        return false;
    }

    if (InCode.Size() % sizeof(uint32) != 0)
    {
        VULKAN_ERROR("SPIR-V code must be a multiple of 4, ensure that the code is valid SPIR-V");
        return false;
    }
 
    spvc_context Context = nullptr;
    spvc_result  Result  = spvc_context_create(&Context);
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
    constexpr uint32 ElementSize = sizeof(uint32) / sizeof(uint8);
    const uint32 NumElements = InCode.Size() / ElementSize;

    spvc_parsed_ir ParsedCode = nullptr;
    Result = spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(InCode.Data()), NumElements, &ParsedCode);
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

    // Not setting variables directly in case we fail on the way
    FVulkanShaderLayout LocalShaderLayout;

    // Use global binding for Vulkan shaders
    uint32 GlobalBinding = 0;
    // TODO: Get this from pipeline-creation
    const uint32 DescriptorSetOffset = GetShaderStageDescriporSetOffset(ShaderVisibility);

    // SRV Textures
    size_t NumSampledImages = 0;
    const spvc_reflected_resource* SampledImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, &SampledImages, &NumSampledImages) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumSampledImages; Index++)
        {
            const uint32 NewBinding     = GlobalBinding++;
            const uint32 CurrentBinding = spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding);
            spvc_compiler_set_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding, NewBinding);
            spvc_compiler_set_decoration(Compiler, SampledImages[Index].id, SpvDecorationDescriptorSet, DescriptorSetOffset);

            LocalShaderLayout.SampledImageBindings.Emplace(BindingType_SampledImage, DescriptorSetOffset, NewBinding, CurrentBinding);
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
            const uint32 NewBinding     = GlobalBinding++;
            const uint32 CurrentBinding = spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding);
            spvc_compiler_set_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding, NewBinding);
            spvc_compiler_set_decoration(Compiler, Samplers[Index].id, SpvDecorationDescriptorSet, DescriptorSetOffset);

            LocalShaderLayout.SamplerBindings.Emplace(BindingType_Sampler, DescriptorSetOffset, NewBinding, CurrentBinding);
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
            const uint32 NewBinding     = GlobalBinding++;
            const uint32 CurrentBinding = spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding);
            spvc_compiler_set_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding, NewBinding);
            spvc_compiler_set_decoration(Compiler, StorageImages[Index].id, SpvDecorationDescriptorSet, DescriptorSetOffset);

            LocalShaderLayout.StorageImageBindings.Emplace(BindingType_StorageImage, DescriptorSetOffset, NewBinding, CurrentBinding);
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
            const uint32 NewBinding     = GlobalBinding++;
            const uint32 CurrentBinding = spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding);
            spvc_compiler_set_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding, NewBinding);
            spvc_compiler_set_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationDescriptorSet, DescriptorSetOffset);

            LocalShaderLayout.UniformBufferBindings.Emplace(BindingType_UniformBuffer, DescriptorSetOffset, NewBinding, CurrentBinding);
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
            const uint32 NewBinding     = GlobalBinding++;
            const uint32 CurrentBinding = spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding);
            spvc_compiler_set_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding, NewBinding);
            spvc_compiler_set_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationDescriptorSet, DescriptorSetOffset);

            const FString BaseTypeName = spvc_compiler_get_name(Compiler, StorageBuffers[Index].base_type_id);
            const bool bIsUAV = BaseTypeName.Contains("RWStructuredBuffer");
            if (bIsUAV)
            {
                LocalShaderLayout.UAVStorageBufferBindings.Emplace(BindingType_StorageBuffer, DescriptorSetOffset, NewBinding, CurrentBinding);
            }
            else
            {
                LocalShaderLayout.SRVStorageBufferBindings.Emplace(BindingType_StorageBuffer, DescriptorSetOffset, NewBinding, CurrentBinding);
            }
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
            size_t NumRanges = 0;
            const spvc_buffer_range* Ranges = nullptr;
            if (spvc_compiler_get_active_buffer_ranges(Compiler, PushConstants[Index].id, &Ranges, &NumRanges) == SPVC_SUCCESS)
            {
                for (uint32 RangeIndex = 0; RangeIndex < NumRanges; RangeIndex++)
                {
                    const spvc_buffer_range& Range = Ranges[Index];
                    // NOTE: For now we have assumed that all push constants use a single range that is available to all shader stages
                    // so the num push bytes means that we have a range from 0->end (end = offset + range)
                    NumPushBytes += Range.offset + Range.range;
                }
            }
            else
            {
                return false;
            }
        }

        LocalShaderLayout.NumPushConstants = FMath::AlignUp<uint32>(static_cast<uint32>(NumPushBytes), sizeof(uint32)) / sizeof(uint32);
    }
    else
    {
        return false;
    }

    // Modify options.
    spvc_compiler_options Options = nullptr;
    spvc_compiler_create_compiler_options(Compiler, &Options);
    spvc_compiler_options_set_uint(Options, SPVC_COMPILER_OPTION_GLSL_VERSION, 450);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_FORCE_TEMPORARY, SPVC_TRUE);
    spvc_compiler_options_set_bool(Options, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, SPVC_TRUE);
    spvc_compiler_install_compiler_options(Compiler, Options);

    // Compile the GLSL code
    const CHAR* NewSource = nullptr;
    Result = spvc_compiler_compile(Compiler, &NewSource);
    if (Result != SPVC_SUCCESS)
    {
        VULKAN_ERROR("Failed to compile changes into GLSL");
        return false;
    }

    // Create a new array (1 extra byte of slack for a null-terminator)
    const uint32 SourceLength = FCString::Strlen(NewSource);
    TArray<uint8> NewShader(reinterpret_cast<const uint8*>(NewSource), SourceLength * sizeof(const uint8), 1);
    NewShader[SourceLength] = 0;

    // Now we can destroy the context
    spvc_context_destroy(Context);

    // Init glslang
    static bool bIsGlslangReady = false;
    if (!bIsGlslangReady)
    {
        glslang_initialize_process();
        bIsGlslangReady = true;
    }

    // Convert the shader stage to glslang-enum
    const glslang_stage_t GlslangStage = GetGlslangStage(ShaderVisibility);

    // Compile the GLSL to SPIRV
    glslang_input_t Input;
    Input.language                          = GLSLANG_SOURCE_GLSL;
    Input.stage                             = GlslangStage;
    Input.client                            = GLSLANG_CLIENT_VULKAN;
    Input.client_version                    = GLSLANG_TARGET_VULKAN_1_2;
    Input.target_language                   = GLSLANG_TARGET_SPV;
    Input.target_language_version           = GLSLANG_TARGET_SPV_1_5;
    Input.code                              = reinterpret_cast<int8*>(NewShader.Data());
    Input.default_version                   = 110;
    Input.default_profile                   = GLSLANG_NO_PROFILE;
    Input.force_default_version_and_profile = false;
    Input.forward_compatible                = false;
    Input.messages                          = GLSLANG_MSG_DEFAULT_BIT;
    Input.resource                          = glslang_default_resource();

    glslang_shader_t* Shader = glslang_shader_create(&Input);
    if (!glslang_shader_preprocess(Shader, &Input))
    {
        const CHAR* InfoLog      = glslang_shader_get_info_log(Shader);
        const CHAR* InfoDebugLog = glslang_shader_get_info_debug_log(Shader);
        
        LOG_ERROR("GLSL preprocessing failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        
        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    if (!glslang_shader_parse(Shader, &Input))
    {
        const CHAR* InfoLog      = glslang_shader_get_info_log(Shader);
        const CHAR* InfoDebugLog = glslang_shader_get_info_debug_log(Shader);
        
        LOG_ERROR("GLSL parsing failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        LOG_ERROR("    %s", glslang_shader_get_preprocessed_code(Shader));

        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    glslang_program_t* Program = glslang_program_create();
    glslang_program_add_shader(Program, Shader);

    if (!glslang_program_link(Program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
    {
        const CHAR* InfoLog      = glslang_program_get_info_log(Program);
        const CHAR* InfoDebugLog = glslang_program_get_info_debug_log(Program);
        
        LOG_ERROR("GLSL linking failed");
        LOG_ERROR("    %s", InfoLog);
        LOG_ERROR("    %s", InfoDebugLog);
        
        glslang_program_delete(Program);
        glslang_shader_delete(Shader);
        
        DEBUG_BREAK();
        return false;
    }

    // Retrieve the SPIRV shader code
    glslang_spv_options_t SpvOptions;
    FMemory::Memzero(&SpvOptions);

    // TODO: Patch the SPIR-V code manually so that we do not need this SPIR-V - >GLSL -> SPIR-V conversion
    SpvOptions.validate            = true;
    SpvOptions.generate_debug_info = true;
    SpvOptions.optimize_size       = true;
    
    glslang_program_SPIRV_generate_with_options(Program, GlslangStage, &SpvOptions);

    // Get the size and allocate enough room in the vector
    const uint64 ProgramSize = glslang_program_SPIRV_get_size(Program);
    
    // Transfer the SPIRV code into our format
    OutCode.Resize(static_cast<int32>(ProgramSize * sizeof(uint32)));
    glslang_program_SPIRV_get(Program, reinterpret_cast<uint32*>(OutCode.Data()));

    // Print any messages from linking
    if (const CHAR* SpirvMessages = glslang_program_SPIRV_get_messages(Program))
    {
        LOG_INFO("%s", SpirvMessages);
    }

    // Cleanup
    glslang_program_delete(Program);
    glslang_shader_delete(Shader);

    // Finalize layout
    LocalShaderLayout.NumTotalBindings = 
        LocalShaderLayout.UniformBufferBindings.Size() +
        LocalShaderLayout.SampledImageBindings.Size() +
        LocalShaderLayout.StorageImageBindings.Size() +
        LocalShaderLayout.SRVStorageBufferBindings.Size() +
        LocalShaderLayout.UAVStorageBufferBindings.Size() +
        LocalShaderLayout.SamplerBindings.Size();

    ShaderLayout = Move(LocalShaderLayout);
    return true;
}
