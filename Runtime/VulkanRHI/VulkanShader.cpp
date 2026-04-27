#include "VulkanRHI/VulkanShader.h"
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
    return (FCString::Strcmp(Name, "SPV_GOOGLE_decorate_string") == 0) || (FCString::Strcmp(Name, "SPV_GOOGLE_hlsl_functionality1") == 0) || 
        (FCString::Strcmp(Name, "SPV_GOOGLE_user_type") == 0);
}

static bool IsGoogleDecorateStringDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslSemanticGOOGLE || DecorationId == SpirvOps::DecorationUserTypeGOOGLE;
}

static bool IsGoogleDecorateIdDecoration(uint32 DecorationId)
{
    return DecorationId == SpirvOps::DecorationHlslCounterBufferGOOGLE;
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

FVulkanRayTracingShader::FVulkanRayTracingShader(FVulkanDevice* InDevice)
    : FVulkanShader(InDevice, ShaderVisibility_Compute)
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

    FString GoogleValidationError;
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

		if (TSharedRef<FVulkanShaderModule>* Existing = ShaderModules.Find(DescriptorSetIndex))
		{
		    // Another thread won the race; destroy the newly created VkShaderModule and reuse the existing shared ref.
			vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
		    return *Existing;
		}

        TSharedRef<FVulkanShaderModule> NewShaderModule = new FVulkanShaderModule(GetDevice(), ShaderModule);
        ShaderModules.Add(DescriptorSetIndex, NewShaderModule);
        return NewShaderModule;
    }
}

bool FVulkanShader::PatchShaderBindings(FSpirvArray& OutSpirv, uint32 DescriptorSetIndex)
{
    if (SpirvCode.IsEmpty())
    {
        VULKAN_ERROR_CRITICAL("No SPIR-V code supplied");
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
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = VulkanBindingType_SampledImage;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding));
            
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
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = VulkanBindingType_Sampler;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, Samplers[Index].id, SpvDecorationBinding));
            
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
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = VulkanBindingType_StorageImage;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding));
            
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
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingType          = VulkanBindingType_UniformBuffer;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, UniformBuffers[Index].id, SpvDecorationBinding));
            
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
            FVulkanShaderInfo::FResourceBinding Binding;
            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, StorageBuffers[Index].id, SpvDecorationBinding));
            
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

            const FString BaseTypeName = spvc_compiler_get_name(Compiler, StorageBuffers[Index].base_type_id);

            const bool bIsUAV = BaseTypeName.Contains("RWStructuredBuffer");
            if (bIsUAV)
            {
                Binding.BindingType = VulkanBindingType_StorageBufferReadWrite;
            }
            else
            {
                Binding.BindingType = VulkanBindingType_StorageBufferRead;
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
        constexpr size_t MaxBytes  = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
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

bool FVulkanShader::ValidateNoGoogleSpirvRequirements(const FSpirvArray& Words, FString* OutErrorMessage)
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
                        *OutErrorMessage = FString::CreateFormatted("Found Google extension: %s", ExtName);
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
    , FVulkanShader(InDevice, ShaderVisibility_Vertex)
{
}

FVulkanVertexShaderRHI::~FVulkanVertexShaderRHI() = default;

FVulkanHullShaderRHI::FVulkanHullShaderRHI(FVulkanDevice* InDevice)
    : FRHIHullShader()
    , FVulkanShader(InDevice, ShaderVisibility_Hull)
{
}

FVulkanHullShaderRHI::~FVulkanHullShaderRHI() = default;

FVulkanDomainShaderRHI::FVulkanDomainShaderRHI(FVulkanDevice* InDevice)
    : FRHIDomainShader()
    , FVulkanShader(InDevice, ShaderVisibility_Domain)
{
}

FVulkanDomainShaderRHI::~FVulkanDomainShaderRHI() = default;

FVulkanGeometryShaderRHI::FVulkanGeometryShaderRHI(FVulkanDevice* InDevice)
    : FRHIGeometryShader()
    , FVulkanShader(InDevice, ShaderVisibility_Geometry)
{
}

FVulkanGeometryShaderRHI::~FVulkanGeometryShaderRHI() = default;

FVulkanPixelShaderRHI::FVulkanPixelShaderRHI(FVulkanDevice* InDevice)
    : FRHIPixelShader()
    , FVulkanShader(InDevice, ShaderVisibility_Pixel)
{
}

FVulkanPixelShaderRHI::~FVulkanPixelShaderRHI() = default;

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
    , FVulkanShader(InDevice, ShaderVisibility_Compute)
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

