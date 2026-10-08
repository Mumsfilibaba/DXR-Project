#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanConstants.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "Core/Misc/CRC.h"

static EVulkanBindingType::Type GetVulkanBindingType(EShaderResourceType Type)
{
    switch (Type)
    {
        case EShaderResourceType::ConstantBuffer:        return EVulkanBindingType::UniformBuffer;
        case EShaderResourceType::Sampler:               return EVulkanBindingType::Sampler;
        case EShaderResourceType::Texture:               return EVulkanBindingType::SampledImage;
        case EShaderResourceType::TypedBuffer:           return EVulkanBindingType::TexelBufferRead;
        case EShaderResourceType::StructuredBuffer:
        case EShaderResourceType::ByteAddressBuffer:     return EVulkanBindingType::StorageBufferRead;
        case EShaderResourceType::AccelerationStructure: return EVulkanBindingType::AccelerationStructure;
        case EShaderResourceType::RWTexture:             return EVulkanBindingType::StorageImage;
        case EShaderResourceType::RWTypedBuffer:         return EVulkanBindingType::TexelBufferReadWrite;
        case EShaderResourceType::RWStructuredBuffer:
        case EShaderResourceType::RWByteAddressBuffer:   return EVulkanBindingType::StorageBufferReadWrite;
        default:                                         return EVulkanBindingType::Count;
    }
}

static EVulkanNullImageViewType GetNullImageViewType(EShaderResourceDimension Dimension)
{
    switch (Dimension)
    {
        case EShaderResourceDimension::Texture1D:        return EVulkanNullImageViewType::Texture1D;
        case EShaderResourceDimension::Texture1DArray:   return EVulkanNullImageViewType::Texture1DArray;
        case EShaderResourceDimension::Texture2DArray:   return EVulkanNullImageViewType::Texture2DArray;
        case EShaderResourceDimension::Texture2DMS:      return EVulkanNullImageViewType::Texture2DMS;
        case EShaderResourceDimension::Texture2DMSArray: return EVulkanNullImageViewType::Texture2DMSArray;
        case EShaderResourceDimension::Texture3D:        return EVulkanNullImageViewType::Texture3D;
        case EShaderResourceDimension::TextureCube:      return EVulkanNullImageViewType::TextureCube;
        case EShaderResourceDimension::TextureCubeArray: return EVulkanNullImageViewType::TextureCubeArray;
        default:                                         return EVulkanNullImageViewType::Texture2D;
    }
}

static uint16 ComputeEffectiveRegister(EShaderBindingSpace Space, uint8 Register)
{
    if (Space == EShaderBindingSpace::RayTracingLocal)
    {
        const uint32 EffectiveRegister = VULKAN_RAY_TRACING_LOCAL_REGISTER_BASE + Register;
        VULKAN_ERROR("RT-local resource (register %u, space%u) force-mapped into the global set at register %u",
            static_cast<uint32>(Register), VULKAN_RAY_TRACING_LOCAL_SET, EffectiveRegister);
        CHECK(EffectiveRegister < VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS);
        return static_cast<uint16>(EffectiveRegister);
    }

    return static_cast<uint16>(Register);
}

static bool FindDescriptorDecorationOffsets(const uint32* Words, uint32 NumWords, uint32 Set, uint32 Binding, FSpirvBindingOffsets& OutOffsets)
{
    constexpr uint16 OpDecorate              = 71;
    constexpr uint32 DecorationBinding       = 33;
    constexpr uint32 DecorationDescriptorSet = 34;

    struct FDecorations
    {
        uint32 SetValue      = UINT32_MAX;
        uint32 SetOffset     = UINT32_MAX;
        uint32 BindingValue  = UINT32_MAX;
        uint32 BindingOffset = UINT32_MAX;
    };

    TMap<uint32, FDecorations> DecorationsById;
    for (uint32 Read = 5; Read < NumWords;)
    {
        const uint16 OpCode    = static_cast<uint16>(Words[Read] & 0xFFFFu);
        const uint16 InstWords = static_cast<uint16>(Words[Read] >> 16);
        if (InstWords == 0 || Read + InstWords > NumWords)
        {
            return false;
        }

        if (OpCode == OpDecorate && InstWords == 4)
        {
            FDecorations& Decorations = DecorationsById.FindOrAdd(Words[Read + 1]);
            if (Words[Read + 2] == DecorationDescriptorSet)
            {
                Decorations.SetValue  = Words[Read + 3];
                Decorations.SetOffset = Read + 3;
            }
            else if (Words[Read + 2] == DecorationBinding)
            {
                Decorations.BindingValue  = Words[Read + 3];
                Decorations.BindingOffset = Read + 3;
            }
        }

        Read += InstWords;
    }

    uint32 NumMatches = 0;
    DecorationsById.Foreach([&](const uint32&, const FDecorations& Decorations)
    {
        if (Decorations.SetValue == Set && Decorations.BindingValue == Binding && Decorations.SetOffset < UINT16_MAX && Decorations.BindingOffset < UINT16_MAX)
        {
            OutOffsets.SetWordOffset     = static_cast<uint16>(Decorations.SetOffset);
            OutOffsets.BindingWordOffset = static_cast<uint16>(Decorations.BindingOffset);
            NumMatches++;
        }
    });

    return NumMatches == 1;
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

bool FVulkanShader::Initialize(const FShaderCodeView& InCode)
{
    const TArrayView<const uint8> NativeCode = InCode.GetNativeCode();
    if (NativeCode.Size() % sizeof(uint32) != 0)
    {
        VULKAN_ERROR_CRITICAL("SPIR-V code is not aligned properly, ensure that the code is valid SPIR-V");
        return false;
    }

    SpirvCode      = FSpirvArray(reinterpret_cast<const uint32*>(NativeCode.Data()), static_cast<int32>(NativeCode.Size() / sizeof(uint32)));
    EntryPointName = InCode.GetEntryPoint();
    if (EntryPointName.IsEmpty())
    {
        VULKAN_ERROR_CRITICAL("The SPIR-V shader code has no entry point");
        return false;
    }

    return BuildShaderInfo(InCode);
}

bool FVulkanShader::CreateInternalShaderCode(EShaderStage Stage, TArrayView<const uint8> Spirv, FShaderReflection Reflection, TArray<uint8>& OutShaderCode)
{
    const uint32* Words    = reinterpret_cast<const uint32*>(Spirv.Data());
    const uint32  NumWords = static_cast<uint32>(Spirv.Size() / sizeof(uint32));

    Reflection.SpirvOffsets.Clear();
    for (const FShaderResourceBinding& Binding : Reflection.Bindings)
    {
        const uint32 Set = Binding.Space == EShaderBindingSpace::RayTracingLocal ? VULKAN_RAY_TRACING_LOCAL_SET : 0;
        if (!FindDescriptorDecorationOffsets(Words, NumWords, Set, Binding.Register, Reflection.SpirvOffsets.Emplace()))
        {
            VULKAN_ERROR("Internal shader binding (set %u, binding %u) was not found exactly once", Set, static_cast<uint32>(Binding.Register));
            return false;
        }
    }

    String Error;
    if (!FShaderCodeWriter::Write(EShaderOutputLanguage::SPIRV, Stage, EShaderCodeFlags::None, Reflection, Spirv, OutShaderCode, &Error))
    {
        VULKAN_ERROR("Failed to write the internal shader code: %s", *Error);
        return false;
    }

    return true;
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
        if (FVulkanShaderModuleRef* ShaderModule = ShaderModules.Find(ModuleKey))
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

    // ShaderCompiler already stripped the Google extensions and rewrote the storage-image formats
    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode    = PatchedCode.Data();
    ShaderModuleCreateInfo.codeSize = PatchedCode.SizeInBytes();

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

        if (FVulkanShaderModuleRef* Existing = ShaderModules.Find(ModuleKey))
        {
            // Another thread won the race; destroy the newly created VkShaderModule and reuse the existing shared ref.
            vkDestroyShaderModule(GetDevice()->GetVkDevice(), ShaderModule, nullptr);
            return *Existing;
        }

        FVulkanShaderModuleRef NewShaderModule = new FVulkanShaderModule(GetDevice(), ShaderModule);
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
    
    // BindingOffsets is index-aligned with ResourceBindings. Resolve the final binding number from the
    // (merged) layout and write both decorations. For non-RT shaders this resolves to the same dense
    // BindingIndex as before. For ray tracing it resolves to the shared merged binding.
    
    FSpirvArray PatchedCode = SpirvCode;
    for (int32 Index = 0; Index < ShaderInfo.BindingOffsets.Size(); Index++)
    {
        const FVulkanShaderInfo::FBindingOffsets& Offsets  = ShaderInfo.BindingOffsets[Index];
        const FVulkanShaderInfo::FResourceBinding& Binding = ShaderInfo.ResourceBindings[Index];

        CHECK(Offsets.BindingOffset       != UINT16_MAX);
        CHECK(Offsets.DescriptorSetOffset != UINT16_MAX);

        uint32 RemappedBinding = 0;
        const bool bFound = Layout->GetRemappedBinding(ShaderVisibility, Binding.BindingType, Binding.OriginalBindingIndex, RemappedBinding);

        // Ray tracing merges several stages into one layout, so its bindings legitimately move. Every other
        // stage must land on its own slot; anything else means two registers collided in one namespace.
        if (ShaderVisibility != EShaderVisibility::RayTracing && (!bFound || RemappedBinding != Binding.BindingIndex))
        {
            VULKAN_ERROR_CRITICAL("Binding %u (register %u, %s) did not resolve to its own layout slot (found=%s, remapped=%u)",
                Binding.BindingIndex, Binding.OriginalBindingIndex, ToString(Binding.BindingType), bFound ? "yes" : "no", RemappedBinding);
            return false;
        }

        const uint32 FinalBinding = bFound ? RemappedBinding : Binding.BindingIndex;

        PatchedCode[Offsets.BindingOffset]       = FinalBinding;
        PatchedCode[Offsets.DescriptorSetOffset] = DescriptorSetIndex;
    }

#if VULKAN_ENABLE_SPLIT_BINDLESS_HEAP
    FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
    const bool bSplitHeap = (BindlessManager != nullptr) && (BindlessManager->GetMode() == EVulkanBindlessMode::Split);
#endif

    for (const FVulkanShaderInfo::FBindingOffsets& Offsets : ShaderInfo.HeapBindingOffsets)
    {
        CHECK(Offsets.DescriptorSetOffset != UINT16_MAX);
        PatchedCode[Offsets.DescriptorSetOffset] = VULKAN_BINDLESS_RUNTIME_SET_INDEX;

    #if VULKAN_ENABLE_SPLIT_BINDLESS_HEAP
        if (bSplitHeap)
        {
            CHECK(Offsets.BindingOffset   != UINT16_MAX);
            CHECK(Offsets.HeapBindingType != EVulkanBindingType::Count);
            PatchedCode[Offsets.BindingOffset] = GetBindlessBindingForType(GetDescriptorTypeFromBindingType(Offsets.HeapBindingType));
        }
    #endif
    }

    OutSpirv = Move(PatchedCode);
    return true;
}

bool FVulkanShader::BuildShaderInfo(const FShaderCodeView& InCode)
{
    const TArrayView<const FShaderResourceBinding> Bindings     = InCode.GetBindings();
    const TArrayView<const FSpirvBindingOffsets>   SpirvOffsets = InCode.GetSpirvOffsets();
    CHECK(SpirvOffsets.Size() == Bindings.Size());

    uint32 GlobalBinding = 0;
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        const FShaderResourceBinding& Binding = Bindings[Index];
        const FSpirvBindingOffsets&   Offsets = SpirvOffsets[Index];

        const EVulkanBindingType::Type BindingType = GetVulkanBindingType(Binding.Type);
        CHECK(BindingType != EVulkanBindingType::Count);
        CHECK(Binding.Space != EShaderBindingSpace::ShaderConstants);

        if (Binding.Space == EShaderBindingSpace::BindlessHeap)
        {
            ShaderInfo.HeapBindingOffsets.Add({ Offsets.SetWordOffset, Offsets.BindingWordOffset, BindingType });
            continue;
        }

        FVulkanShaderInfo::FResourceBinding NewBinding;
        NewBinding.BindingType          = BindingType;
        NewBinding.BindingIndex         = static_cast<uint8>(GlobalBinding++);
        NewBinding.NullViewType         = GetNullImageViewType(Binding.Dimension);
        NewBinding.OriginalBindingIndex = ComputeEffectiveRegister(Binding.Space, Binding.Register);

    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        NewBinding.DebugName = InCode.GetBindingName(Index);
    #endif

        ShaderInfo.BindingOffsets.Add({ Offsets.SetWordOffset, Offsets.BindingWordOffset });
        ShaderInfo.ResourceBindings.Add(::Move(NewBinding));
    }

    // Whole float4s, the same as D3D12
    constexpr uint32 MaxBytes  = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
    const uint32     PushBytes = Math::AlignUp<uint32>(InCode.GetInfo().ShaderConstantsSize, sizeof(float) * 4);
    if (PushBytes > MaxBytes)
    {
        VULKAN_ERROR_CRITICAL("The shader constants are %u bytes, only %u bytes are supported", PushBytes, MaxBytes);
        return false;
    }

    ShaderInfo.NumPushConstants = PushBytes / sizeof(uint32);
    return true;
}

FVulkanVertexShaderRHI::FVulkanVertexShaderRHI(FVulkanDevice* InDevice)
    : FRHIVertexShader()
    , FVulkanShader(InDevice, EShaderVisibility::Vertex)
{
}

FVulkanVertexShaderRHI::~FVulkanVertexShaderRHI() = default;

void* FVulkanVertexShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanVertexShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanHullShaderRHI::FVulkanHullShaderRHI(FVulkanDevice* InDevice)
    : FRHIHullShader()
    , FVulkanShader(InDevice, EShaderVisibility::Hull)
{
}

FVulkanHullShaderRHI::~FVulkanHullShaderRHI() = default;

void* FVulkanHullShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanHullShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanDomainShaderRHI::FVulkanDomainShaderRHI(FVulkanDevice* InDevice)
    : FRHIDomainShader()
    , FVulkanShader(InDevice, EShaderVisibility::Domain)
{
}

FVulkanDomainShaderRHI::~FVulkanDomainShaderRHI() = default;

void* FVulkanDomainShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanDomainShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanGeometryShaderRHI::FVulkanGeometryShaderRHI(FVulkanDevice* InDevice)
    : FRHIGeometryShader()
    , FVulkanShader(InDevice, EShaderVisibility::Geometry)
{
}

FVulkanGeometryShaderRHI::~FVulkanGeometryShaderRHI() = default;

void* FVulkanGeometryShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanGeometryShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanPixelShaderRHI::FVulkanPixelShaderRHI(FVulkanDevice* InDevice)
    : FRHIPixelShader()
    , FVulkanShader(InDevice, EShaderVisibility::Pixel)
{
}

FVulkanPixelShaderRHI::~FVulkanPixelShaderRHI() = default;

void* FVulkanPixelShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanPixelShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanMeshShaderRHI::FVulkanMeshShaderRHI(FVulkanDevice* InDevice)
    : FRHIMeshShader()
    , FVulkanShader(InDevice, EShaderVisibility::Mesh)
{
}

FVulkanMeshShaderRHI::~FVulkanMeshShaderRHI() = default;

void* FVulkanMeshShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanMeshShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanAmplificationShaderRHI::FVulkanAmplificationShaderRHI(FVulkanDevice* InDevice)
    : FRHIAmplificationShader()
    , FVulkanShader(InDevice, EShaderVisibility::Task)
{
}

FVulkanAmplificationShaderRHI::~FVulkanAmplificationShaderRHI() = default;

void* FVulkanAmplificationShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanAmplificationShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}

FVulkanRayTracingShader::FVulkanRayTracingShader(FVulkanDevice* InDevice)
    : FVulkanShader(InDevice, EShaderVisibility::RayTracing)
{
}

FVulkanRayTracingShader::~FVulkanRayTracingShader() = default;

FVulkanRayGenShaderRHI::FVulkanRayGenShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayGenShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayGenShaderRHI::~FVulkanRayGenShaderRHI() = default;

void* FVulkanRayGenShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayGenShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanRayAnyHitShaderRHI::FVulkanRayAnyHitShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayAnyHitShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayAnyHitShaderRHI::~FVulkanRayAnyHitShaderRHI() = default;

void* FVulkanRayAnyHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayAnyHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanRayClosestHitShaderRHI::FVulkanRayClosestHitShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayClosestHitShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayClosestHitShaderRHI::~FVulkanRayClosestHitShaderRHI() = default;

void* FVulkanRayClosestHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayClosestHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanRayMissShaderRHI::FVulkanRayMissShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayMissShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayMissShaderRHI::~FVulkanRayMissShaderRHI() = default;

void* FVulkanRayMissShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayMissShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanRayIntersectionShaderRHI::FVulkanRayIntersectionShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayIntersectionShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayIntersectionShaderRHI::~FVulkanRayIntersectionShaderRHI() = default;

void* FVulkanRayIntersectionShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayIntersectionShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanRayCallableShaderRHI::FVulkanRayCallableShaderRHI(FVulkanDevice* InDevice)
    : FRHIRayCallableShader()
    , FVulkanRayTracingShader(InDevice)
{
}

FVulkanRayCallableShaderRHI::~FVulkanRayCallableShaderRHI() = default;

void* FVulkanRayCallableShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanRayCallableShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanRayTracingShader*>(this);
}

FVulkanComputeShaderRHI::FVulkanComputeShaderRHI(FVulkanDevice* InDevice)
    : FRHIComputeShader()
    , FVulkanShader(InDevice, EShaderVisibility::Compute)
{
}

FVulkanComputeShaderRHI::~FVulkanComputeShaderRHI() = default;

void* FVulkanComputeShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(&SpirvCode);
}

void* FVulkanComputeShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FVulkanShader*>(this);
}
