#include "Core/Misc/CRC.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanPipelineLayout.h"

#include <spirv_cross_c.h>
#include <spirv-headers/spirv.h>

static bool SpvReadLiteralString(const uint32* InWords, uint32 InWordCount, uint32 InFirstStringWord, CHAR* OutStr, SIZE_T OutStrSize)
{
    if (!InWords || !OutStr || OutStrSize == 0)
    {
        return false;
    }

    // Literal strings are NULL-terminated and padded to 32-bit words. Make sure we never read past this instruction.
    const uint32 MaxBytes = (InWordCount > InFirstStringWord) ? (InWordCount - InFirstStringWord) * 4u : 0u;
    if (MaxBytes == 0)
    {
        return false;
    }

    const CHAR* Src = reinterpret_cast<const CHAR*>(InWords + InFirstStringWord);

    // Find NULL within the instruction bounds.
    uint32 Len = 0;
    while (Len < MaxBytes && Src[Len] != '\0')
    {
        ++Len;
    }

    if (Len >= MaxBytes)
    {
        return false; // No terminator within instruction -> malformed.
    }

    const SIZE_T CopyLen = (static_cast<SIZE_T>(Len) < (OutStrSize - 1)) ? static_cast<SIZE_T>(Len) : (OutStrSize - 1);
    FMemory::Memcpy(OutStr, Src, CopyLen);
    OutStr[CopyLen] = '\0';
    return true;
}

static bool IsOneOfGoogleExtensions(const CHAR* Ext)
{
    return (FCString::Strcmp(Ext, "SPV_GOOGLE_hlsl_functionality1") == 0) ||
           (FCString::Strcmp(Ext, "SPV_GOOGLE_user_type") == 0) ||
           (FCString::Strcmp(Ext, "SPV_GOOGLE_decorate_string") == 0);
}

static bool IsGoogleDecorateStringDecoration(uint32 Decoration)
{
    // These are string-based decorations which are part of SPV_GOOGLE_decorate_string / SPV_GOOGLE_user_type.
    return (Decoration == SpvDecorationUserTypeGOOGLE) || (Decoration == SpvDecorationHlslSemanticGOOGLE);
}

static bool IsGoogleDecorateIdDecoration(uint32 Decoration)
{
    // Optional: strip HLSL counter buffer association decoration as well.
    return (Decoration == SpvDecorationHlslCounterBufferGOOGLE);
}

static SpvExecutionModel GetSpvExecutionModelForVisibility(EShaderVisibility Visibility)
{
    switch (Visibility)
    {
        case ShaderVisibility_Vertex:   return SpvExecutionModelVertex;
        case ShaderVisibility_Hull:     return SpvExecutionModelTessellationControl;
        case ShaderVisibility_Domain:   return SpvExecutionModelTessellationEvaluation;
        case ShaderVisibility_Geometry: return SpvExecutionModelGeometry;
        case ShaderVisibility_Pixel:    return SpvExecutionModelFragment;
        case ShaderVisibility_Compute:  return SpvExecutionModelGLCompute;
        default:                        return SpvExecutionModelMax;
    }
}

FVulkanShaderModule::FVulkanShaderModule(FVulkanDevice* InDevice, VkShaderModule InShaderModule)
    : FVulkanDeviceChild(InDevice)
    , ShaderModule(InShaderModule)
{
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

FVulkanShaderModuleRef FVulkanShader::GetOrCreateShaderModule(FVulkanPipelineLayout* Layout)
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
        if (FVulkanShaderModuleRef* ShaderModule = ShaderModules.Find(DescriptorSetIndex))
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
        VULKAN_ERROR_CRITICAL("Failed to strip SPV_GOOGLE requirements from SPIR-V");
        return nullptr;
    }

    FString GoogleValidationError;
    if (!ValidateNoGoogleSpirvRequirements(StrippedCode, &GoogleValidationError))
    {
        VULKAN_ERROR_CRITICAL("SPIR-V still contains GOOGLE requirements after stripping: %s", *GoogleValidationError);
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

    TScopedLock Lock(ShaderModulesCS);

    if (FVulkanShaderModuleRef* Existing = ShaderModules.Find(DescriptorSetIndex))
    {
        vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
        return *Existing;
    }

    FVulkanShaderModuleRef NewShaderModule = new FVulkanShaderModule(GetDevice(), ShaderModule);
    ShaderModules.Add(DescriptorSetIndex, NewShaderModule);
    return NewShaderModule;
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

bool FVulkanShader::StripGoogleSpirvRequirements(const FSpirvArray& InWords, FSpirvArray& OutWords)
{
    // --------------------------------------------------------------------------------------------------------------------
    // Strips SPV_GOOGLE_* extension declarations and the non-semantic reflection decorations which force
    // enabling VK_GOOGLE_* device extensions, while preserving source debug (NonSemantic.*).
    //
    // IMPORTANT:
    // - Run this AFTER you do any patching based on binary offsets (binding remap), because stripping changes offsets.
    // --------------------------------------------------------------------------------------------------------------------

    OutWords.Clear();

    if (InWords.Size() < 5)
    {
        return false;
    }

    OutWords.Reserve(InWords.Size());

    // Copy header (5 words).
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

        // 1) Remove OpExtension "SPV_GOOGLE_*"
        if (OpCode == SpvOpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, /* First string word */ 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    bSkip = true;
                }
            }
        }

        // 2) Remove OpDecorateString* / OpMemberDecorateString* for GOOGLE decorations.
        if (!bSkip && (OpCode == SpvOpDecorateString || OpCode == SpvOpDecorateStringGOOGLE))
        {
            // OpDecorateString: <target-id> <decoration> <literal-string...>
            if (InstWords >= 3)
            {
                const uint32 Decoration = Inst[2];
                if (IsGoogleDecorateStringDecoration(Decoration))
                {
                    bSkip = true;
                }
            }
        }

        if (!bSkip && (OpCode == SpvOpMemberDecorateString || OpCode == SpvOpMemberDecorateStringGOOGLE))
        {
            // OpMemberDecorateString: <struct-type-id> <member> <decoration> <literal-string...>
            if (InstWords >= 4)
            {
                const uint32 Decoration = Inst[3];
                if (IsGoogleDecorateStringDecoration(Decoration))
                {
                    bSkip = true;
                }
            }
        }

        // 3) Remove GOOGLE decorate-id style decorations too (e.g. HlslCounterBufferGOOGLE).
        // OpDecorateId opcode is 332 in SPIR-V, but may not exist in older headers; compare by opcode number to be safe.
        if (!bSkip && (OpCode == SpvOpDecorate || OpCode == 332u))
        {
            // OpDecorate:   <target-id> <decoration> [extra operands...]
            // OpDecorateId: <target-id> <decoration> <id>

            if (InstWords >= 3)
            {
                const uint32 Decoration = Inst[2];
                if (IsGoogleDecorateIdDecoration(Decoration))
                {
                    bSkip = true;
                }
            }
        }

        if (!bSkip && (OpCode == SpvOpMemberDecorate))
        {
            // OpMemberDecorate: <struct-type-id> <member> <decoration> [extra...]
            if (InstWords >= 4)
            {
                const uint32 Decoration = Inst[3];
                if (IsGoogleDecorateIdDecoration(Decoration))
                {
                    bSkip = true;
                }
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
    if (OutErrorMessage)
    {
        *OutErrorMessage = "";
    }

    if (Words.Size() < 5)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = "SPIR-V is too small (missing header).";
        }

        return false;
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
            if (OutErrorMessage)
            {
                *OutErrorMessage = "Malformed SPIR-V instruction (word count out of bounds).";
            }

            return false;
        }

        const uint32* Inst = &Data[Read];

        // OpExtension "SPV_GOOGLE_*"
        if (OpCode == SpvOpExtension)
        {
            CHAR ExtName[256] = {};
            if (SpvReadLiteralString(Inst, InstWords, 1, ExtName, sizeof(ExtName)))
            {
                if (IsOneOfGoogleExtensions(ExtName))
                {
                    if (OutErrorMessage)
                    {
                        *OutErrorMessage = "Found GOOGLE OpExtension: ";
                        *OutErrorMessage += ExtName;
                    }

                    return false;
                }
            }
        }

        // OpDecorateString / OpMemberDecorateString with GOOGLE decorations.
        if (OpCode == SpvOpDecorateString || OpCode == SpvOpDecorateStringGOOGLE)
        {
            if (InstWords >= 3 && IsGoogleDecorateStringDecoration(Inst[2]))
            {
                if (OutErrorMessage)
                {
                    *OutErrorMessage = "Found GOOGLE OpDecorateString decoration.";
                }

                return false;
            }
        }

        if (OpCode == SpvOpMemberDecorateString || OpCode == SpvOpMemberDecorateStringGOOGLE)
        {
            if (InstWords >= 4 && IsGoogleDecorateStringDecoration(Inst[3]))
            {
                if (OutErrorMessage)
                {
                    *OutErrorMessage = "Found GOOGLE OpMemberDecorateString decoration.";
                }

                return false;
            }
        }

        // decorate-id based GOOGLE decoration (OpDecorateId opcode is 332).
        if (OpCode == SpvOpDecorate || OpCode == 332u)
        {
            if (InstWords >= 3 && IsGoogleDecorateIdDecoration(Inst[2]))
            {
                if (OutErrorMessage)
                {
                    *OutErrorMessage = "Found GOOGLE OpDecorate/OpDecorateId decoration.";
                }

                return false;
            }
        }

        if (OpCode == SpvOpMemberDecorate)
        {
            if (InstWords >= 4 && IsGoogleDecorateIdDecoration(Inst[3]))
            {
                if (OutErrorMessage)
                {
                    *OutErrorMessage = "Found GOOGLE OpMemberDecorate decoration.";
                }

                return false;
            }
        }

        Read += InstWords;
    }

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

    // Select an entry point that matches the shader stage.
    // Vulkan picks entry points via VkPipelineShaderStageCreateInfo::pName, but SPIRV-Cross reflection must also
    // be configured to the same entry point if there are multiple in the module.

    {
        SIZE_T NumEntryPoints = 0;
        const spvc_entry_point* EntryPoints = nullptr;

        Result = spvc_compiler_get_entry_points(Compiler, &EntryPoints, &NumEntryPoints);
        if (Result != SPVC_SUCCESS || EntryPoints == nullptr || NumEntryPoints == 0)
        {
            VULKAN_ERROR_CRITICAL("SPIR-V contains no entry points (OpEntryPoint)");
            spvc_context_destroy(Context);
            return false;
        }

        const SpvExecutionModel WantedModel = GetSpvExecutionModelForVisibility(ShaderVisibility);
        if (WantedModel == SpvExecutionModelMax)
        {
            VULKAN_ERROR_CRITICAL("Invalid shader visibility when selecting SPIR-V entry point");
            spvc_context_destroy(Context);
            return false;
        }

        if (NumEntryPoints > 1)
        {
            VULKAN_ERROR_CRITICAL("Only a single entry point per SPIR-V module is supported (found %zu). Compile with one entry point.", NumEntryPoints);
            spvc_context_destroy(Context);
            return false;
        }

        const spvc_entry_point& Single = EntryPoints[0];
        if (Single.execution_model != WantedModel)
        {
            VULKAN_ERROR_CRITICAL("Entry point execution model does not match shader stage (visibility=%u)", static_cast<uint32>(ShaderVisibility));
            spvc_context_destroy(Context);
            return false;
        }

        const spvc_entry_point* BestEntryPoint = &Single;
        CHECK(BestEntryPoint->name != nullptr);
        EntryPoint = BestEntryPoint->name;

        Result = spvc_compiler_set_entry_point(Compiler, BestEntryPoint->name, BestEntryPoint->execution_model);
        if (Result != SPVC_SUCCESS)
        {
            VULKAN_ERROR_CRITICAL("Failed to set SPIRV-Cross entry point: %s", BestEntryPoint->name);
            spvc_context_destroy(Context);
            return false;
        }
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

    const auto HasDuplicateBinding = [&](EVulkanBindingType BindingType, uint8 OriginalBindingIndex) -> bool
    {
        for (const FVulkanShaderInfo::FResourceBinding& ExistingBinding : ShaderInfo.ResourceBindings)
        {
            if (ExistingBinding.BindingType == BindingType && ExistingBinding.OriginalBindingIndex == OriginalBindingIndex)
            {
                return true;
            }
        }

        return false;
    };

    const auto GetBaseTypeId = [&](spvc_type_id QualifiedTypeId) -> spvc_type_id
    {
        spvc_type TypeHandle = spvc_compiler_get_type_handle(Compiler, QualifiedTypeId);
        return spvc_type_get_base_type_id(TypeHandle);
    };

    const auto HasNonWritableOnAnyMember = [&](spvc_type_id StructTypeId) -> bool
    {
        spvc_type StructType = spvc_compiler_get_type_handle(Compiler, StructTypeId);

        const uint32 NumMembers = spvc_type_get_num_member_types(StructType);
        for (uint32 MemberIndex = 0; MemberIndex < NumMembers; ++MemberIndex)
        {
            if (spvc_compiler_has_member_decoration(Compiler, StructTypeId, MemberIndex, SpvDecorationNonWritable))
            {
                return true;
            }
        }

        return false;
    };

    const auto TryClassifyStorageBufferFromUserType = [&](spvc_variable_id VarId, bool& OutReadOnly) -> bool
    {
        const char* UserTypeStr = spvc_compiler_get_decoration_string(Compiler, VarId, SpvDecorationUserTypeGOOGLE);
        if (UserTypeStr == nullptr || UserTypeStr[0] == '\0')
        {
            return false;
        }

        const FString UserType(UserTypeStr);
        if (UserType.StartsWith("structuredbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("byteaddressbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("tbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("texturebuffer:", EStringCaseType::NoCase))
        {
            OutReadOnly = true;
            return true;
        }

        if (UserType.StartsWith("rwstructuredbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("appendstructuredbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("consumestructuredbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("rasterizerorderedstructuredbuffer:", EStringCaseType::NoCase) ||
            UserType.StartsWith("rwbyteaddressbuffer:", EStringCaseType::NoCase))
        {
            OutReadOnly = false;
            return true;
        }

        return false;
    };

    const auto IsReadOnlyStorageBufferFallback = [&](const spvc_reflected_resource& Res) -> bool
    {
        const spvc_type_id BlockStructTypeId = GetBaseTypeId(Res.type_id);

        bool bNonWritableOnBlock = false;
        
        SIZE_T NumDecorations = 0;
        const SpvDecoration* Decorations = nullptr;
        if (spvc_compiler_get_buffer_block_decorations(Compiler, Res.id, &Decorations, &NumDecorations) == SPVC_SUCCESS)
        {
            for (SIZE_T DecorationIndex = 0; DecorationIndex < NumDecorations; ++DecorationIndex)
            {
                if (Decorations[DecorationIndex] == SpvDecorationNonWritable)
                {
                    bNonWritableOnBlock = true;
                    break;
                }
            }
        }

        const bool bNonWritableOnVar      = spvc_compiler_has_decoration(Compiler, Res.id, SpvDecorationNonWritable) != 0;
        const bool bNonWritableOnType     = spvc_compiler_has_decoration(Compiler, Res.type_id, SpvDecorationNonWritable) != 0;
        const bool bNonWritableOnBaseType = spvc_compiler_has_decoration(Compiler, BlockStructTypeId, SpvDecorationNonWritable) != 0;
        const bool bNonWritableOnMembers  = HasNonWritableOnAnyMember(BlockStructTypeId);
        return bNonWritableOnBlock || bNonWritableOnVar || bNonWritableOnType || bNonWritableOnBaseType || bNonWritableOnMembers;
    };

    // Query storage images first.
    SIZE_T NumStorageImages = 0;

    const spvc_reflected_resource* StorageImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &StorageImages, &NumStorageImages) != SPVC_SUCCESS)
    {
        spvc_context_destroy(Context);
        return false;
    }

    TArray<uint32> StorageImageResourceIDs;
    StorageImageResourceIDs.Reserve(static_cast<int32>(NumStorageImages));
    
    for (uint32 Index = 0; Index < NumStorageImages; Index++)
    {
        StorageImageResourceIDs.AddUnique(static_cast<uint32>(StorageImages[Index].id));
    }

    // SRV Textures
    SIZE_T NumSampledImages = 0;

    const spvc_reflected_resource* SampledImages = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, &SampledImages, &NumSampledImages) == SPVC_SUCCESS)
    {
        for (uint32 Index = 0; Index < NumSampledImages; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;
            const uint8 OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, SampledImages[Index].id, SpvDecorationBinding));

            // If this exact reflected resource is also present in STORAGE_IMAGE, treat it as UAV.
            // Do not key this on binding index alone, since t0/u0 are valid and common in HLSL.

            const uint32 ResourceID = static_cast<uint32>(SampledImages[Index].id);
            Binding.BindingType = StorageImageResourceIDs.Contains(ResourceID) ? VulkanBindingType_StorageImage : VulkanBindingType_SampledImage;

            if (HasDuplicateBinding(Binding.BindingType, OriginalBindingIndex))
            {
                continue;
            }

            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = OriginalBindingIndex;
            
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

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, SampledImages[Index].base_type_id);

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
    SIZE_T NumSamplers = 0;

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

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, Samplers[Index].base_type_id);

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
    {
        for (uint32 Index = 0; Index < NumStorageImages; Index++)
        {
            FVulkanShaderInfo::FResourceBinding Binding;

            const uint8 OriginalBindingIndex = static_cast<uint8>(spvc_compiler_get_decoration(Compiler, StorageImages[Index].id, SpvDecorationBinding));
            Binding.BindingType = VulkanBindingType_StorageImage;

            if (HasDuplicateBinding(Binding.BindingType, OriginalBindingIndex))
            {
                continue;
            }

            Binding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
            Binding.OriginalBindingIndex = OriginalBindingIndex;
            
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
            
            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, StorageImages[Index].base_type_id);

            ShaderInfo.BindingOffsets.Add({ DescriptorSetOffset, BindingOffset });
            ShaderInfo.ResourceBindings.Add(Move(Binding));
        }
    }

    if (StorageImages == nullptr && NumStorageImages > 0)
    {
        spvc_context_destroy(Context);
        return false;
    }

    // ConstantBuffers
    SIZE_T NumUniformBuffers = 0;

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

            // Set debug-name
            Binding.DebugName = spvc_compiler_get_name(Compiler, UniformBuffers[Index].base_type_id);

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
    SIZE_T NumStorageBuffers = 0;

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

            bool bReadOnlyBuffer = false;
            if (!TryClassifyStorageBufferFromUserType(StorageBuffers[Index].id, bReadOnlyBuffer))
            {
                bReadOnlyBuffer = IsReadOnlyStorageBufferFallback(StorageBuffers[Index]);
            }

            if (!bReadOnlyBuffer)
            {
                Binding.BindingType = VulkanBindingType_StorageBufferReadWrite;
            }
            else
            {
                Binding.BindingType = VulkanBindingType_StorageBufferRead;
            }

            // Set debug-name
            Binding.DebugName = Move(BaseTypeName);

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
    SIZE_T NumPushConstants = 0;

    const spvc_reflected_resource* PushConstants = nullptr;
    if (spvc_resources_get_resource_list_for_type(ShaderResources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT, &PushConstants, &NumPushConstants) == SPVC_SUCCESS)
    {
        SIZE_T NumPushBytes = 0;
        for (uint32 Index = 0; Index < NumPushConstants; Index++)
        {
            SIZE_T StructSize = 0;

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

        constexpr SIZE_T MaxBytes  = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
        constexpr SIZE_T Alignment = sizeof(float) * 4;
                
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
