#include "Core/Math/Math.h"
#include "Core/Templates/CString.h"
#include "ShaderCore/MSLShaderBindings.h"
#include "ShaderCompiler/DXC/MSLShaderConverter.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"

#include <spirv_cross_c.h>

struct FMSLReflectedResource
{
    spvc_variable_id         Id;
    spvc_type_id             BaseTypeId;
    EMSLBindingType          BindingType;
    uint8                    RegisterIndex;
    EShaderResourceType      Type;
    EShaderResourceDimension Dimension;
    EShaderBindingSpace      Space;
};

static SpvExecutionModel GetMSLExecutionModel(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Vertex:          return SpvExecutionModelVertex;
        case EShaderStage::Hull:            return SpvExecutionModelTessellationControl;
        case EShaderStage::Domain:          return SpvExecutionModelTessellationEvaluation;
        case EShaderStage::Geometry:        return SpvExecutionModelGeometry;
        case EShaderStage::Mesh:            return SpvExecutionModelMeshEXT;
        case EShaderStage::Amplification:   return SpvExecutionModelTaskEXT;
        case EShaderStage::Pixel:           return SpvExecutionModelFragment;
        case EShaderStage::Compute:         return SpvExecutionModelGLCompute;
        case EShaderStage::RayGen:          return SpvExecutionModelRayGenerationKHR;
        case EShaderStage::RayAnyHit:       return SpvExecutionModelAnyHitKHR;
        case EShaderStage::RayClosestHit:   return SpvExecutionModelClosestHitKHR;
        case EShaderStage::RayMiss:         return SpvExecutionModelMissKHR;
        case EShaderStage::RayIntersection: return SpvExecutionModelIntersectionKHR;
        case EShaderStage::RayCallable:     return SpvExecutionModelCallableKHR;
        default:                            return SpvExecutionModelGLCompute;
    }
}

static bool IsStorageBufferReadOnly(spvc_compiler Compiler, spvc_variable_id Id, bool& bOutIsReadOnly)
{
    size_t               NumDecorations = 0;
    const SpvDecoration* Decorations    = nullptr;
    if (spvc_compiler_get_buffer_block_decorations(Compiler, Id, &Decorations, &NumDecorations) != SPVC_SUCCESS)
    {
        return false;
    }

    bOutIsReadOnly = false;
    for (size_t Index = 0; Index < NumDecorations; Index++)
    {
        if (Decorations[Index] == SpvDecorationNonWritable)
        {
            bOutIsReadOnly = true;
            break;
        }
    }

    return true;
}

static bool UsesSpirvCapability(spvc_compiler Compiler, SpvCapability Capability)
{
    size_t               NumCapabilities = 0;
    const SpvCapability* Capabilities    = nullptr;
    if (spvc_compiler_get_declared_capabilities(Compiler, &Capabilities, &NumCapabilities) != SPVC_SUCCESS)
    {
        return false;
    }

    for (size_t Index = 0; Index < NumCapabilities; Index++)
    {
        if (Capabilities[Index] == Capability)
        {
            return true;
        }
    }

    return false;
}

static EShaderResourceDimension GetImageDimension(spvc_compiler Compiler, spvc_type_id TypeId)
{
    const spvc_type Type          = spvc_compiler_get_type_handle(Compiler, TypeId);
    const bool      bArrayed      = spvc_type_get_image_arrayed(Type) != SPVC_FALSE;
    const bool      bMultisampled = spvc_type_get_image_multisampled(Type) != SPVC_FALSE;

    switch (spvc_type_get_image_dimension(Type))
    {
        case SpvDim1D:     return bArrayed ? EShaderResourceDimension::Texture1DArray : EShaderResourceDimension::Texture1D;
        case SpvDim3D:     return EShaderResourceDimension::Texture3D;
        case SpvDimCube:   return bArrayed ? EShaderResourceDimension::TextureCubeArray : EShaderResourceDimension::TextureCube;
        case SpvDimBuffer: return EShaderResourceDimension::Buffer;
        default:
        {
            if (bMultisampled)
            {
                return bArrayed ? EShaderResourceDimension::Texture2DMSArray : EShaderResourceDimension::Texture2DMS;
            }

            return bArrayed ? EShaderResourceDimension::Texture2DArray : EShaderResourceDimension::Texture2D;
        }
    }
}

static bool GatherMSLResources(spvc_compiler Compiler, spvc_resources Resources, spvc_resource_type ResourceType, TArray<FMSLReflectedResource>& OutResources, String& OutErrors)
{
    size_t                         NumReflected = 0;
    const spvc_reflected_resource* Reflected    = nullptr;
    if (spvc_resources_get_resource_list_for_type(Resources, ResourceType, &Reflected, &NumReflected) != SPVC_SUCCESS)
    {
        OutErrors += String::Printf("Failed to reflect resource type %d\n", static_cast<int32>(ResourceType));
        return false;
    }

    for (size_t Index = 0; Index < NumReflected; Index++)
    {
        const spvc_variable_id Id       = Reflected[Index].id;
        const uint32           SetIndex = spvc_compiler_get_decoration(Compiler, Id, SpvDecorationDescriptorSet);
        const uint32           Register = spvc_compiler_get_decoration(Compiler, Id, SpvDecorationBinding);

        if (SetIndex == MSL_BINDLESS_HEAP_MARKER_SET)
        {
            EMSLBindingType     HeapType     = EMSLBindingType::Unknown;
            EShaderResourceType HeapResource = EShaderResourceType::StructuredBuffer;
            if (Register == MSL_BINDLESS_RESOURCE_BINDING)
            {
                HeapType = EMSLBindingType::BindlessResourceHeap;
            }
            else if (Register == MSL_BINDLESS_SAMPLER_BINDING)
            {
                HeapType     = EMSLBindingType::BindlessSamplerHeap;
                HeapResource = EShaderResourceType::Sampler;
            }
            else
            {
                continue;
            }

            bool bAlreadyGathered = false;
            for (const FMSLReflectedResource& Existing : OutResources)
            {
                if (Existing.BindingType == HeapType)
                {
                    bAlreadyGathered = true;
                    break;
                }
            }

            if (!bAlreadyGathered)
            {
                OutResources.Emplace(FMSLReflectedResource{ Id, Reflected[Index].base_type_id, HeapType, 0, HeapResource, EShaderResourceDimension::Unknown, EShaderBindingSpace::BindlessHeap });
            }

            continue;
        }

        EMSLBindingType          BindingType = EMSLBindingType::Unknown;
        EShaderResourceType      Type        = EShaderResourceType::ConstantBuffer;
        EShaderResourceDimension Dimension   = EShaderResourceDimension::Unknown;
        EShaderBindingSpace      Space       = SetIndex == ShaderBindings::RayTracingLocalSpace ? EShaderBindingSpace::RayTracingLocal : EShaderBindingSpace::Global;
        switch (ResourceType)
        {
            case SPVC_RESOURCE_TYPE_SEPARATE_IMAGE:
            {
                BindingType = EMSLBindingType::ShaderResourceTexture;
                Dimension   = GetImageDimension(Compiler, Reflected[Index].base_type_id);
                Type        = Dimension == EShaderResourceDimension::Buffer ? EShaderResourceType::TypedBuffer : EShaderResourceType::Texture;
                break;
            }

            case SPVC_RESOURCE_TYPE_STORAGE_IMAGE:
            {
                BindingType = EMSLBindingType::UnorderedAccessTexture;
                Dimension   = GetImageDimension(Compiler, Reflected[Index].base_type_id);
                Type        = Dimension == EShaderResourceDimension::Buffer ? EShaderResourceType::RWTypedBuffer : EShaderResourceType::RWTexture;
                break;
            }

            case SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS:
            {
                BindingType = EMSLBindingType::Sampler;
                Type        = EShaderResourceType::Sampler;
                break;
            }

            case SPVC_RESOURCE_TYPE_PUSH_CONSTANT:
            {
                BindingType = EMSLBindingType::ShaderConstants;
                Space       = EShaderBindingSpace::ShaderConstants;
                break;
            }

            case SPVC_RESOURCE_TYPE_UNIFORM_BUFFER:
            {
                if (SetIndex == ShaderBindings::ShaderConstantsSpace)
                {
                    BindingType = EMSLBindingType::ShaderConstants;
                    Space       = EShaderBindingSpace::ShaderConstants;
                }
                else
                {
                    BindingType = EMSLBindingType::ConstantBuffer;
                }

                break;
            }

            case SPVC_RESOURCE_TYPE_STORAGE_BUFFER:
            {
                bool bIsReadOnly = false;
                if (!IsStorageBufferReadOnly(Compiler, Id, bIsReadOnly))
                {
                    OutErrors += String::Printf("Failed to read buffer block decorations for storage buffer at register %u\n", Register);
                    return false;
                }

                BindingType = bIsReadOnly ? EMSLBindingType::ShaderResourceBuffer : EMSLBindingType::UnorderedAccessBuffer;
                Type        = bIsReadOnly ? EShaderResourceType::StructuredBuffer : EShaderResourceType::RWStructuredBuffer;
                break;
            }

            case SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE:
            {
                BindingType = EMSLBindingType::AccelerationStructure;
                Type        = EShaderResourceType::AccelerationStructure;
                break;
            }

            default:
            {
                OutErrors += String::Printf("Unhandled resource type %d\n", static_cast<int32>(ResourceType));
                return false;
            }
        }

        if (Register > 0xFF)
        {
            OutErrors += String::Printf("%s register %u is out of range for an MSL binding table\n", ToString(BindingType), Register);
            return false;
        }

        const uint8 RegisterIndex = BindingType == EMSLBindingType::ShaderConstants ? 0 : static_cast<uint8>(Register);
        OutResources.Emplace(FMSLReflectedResource{ Id, Reflected[Index].base_type_id, BindingType, RegisterIndex, Type, Dimension, Space });
    }

    return true;
}

static uint8 GetMSLNullTextureType(spvc_compiler Compiler, const FMSLReflectedResource& Resource)
{
    if (Resource.BindingType != EMSLBindingType::ShaderResourceTexture && Resource.BindingType != EMSLBindingType::UnorderedAccessTexture)
    {
        return 0;
    }

    const spvc_type Type = spvc_compiler_get_type_handle(Compiler, Resource.BaseTypeId);
    if (spvc_type_get_basetype(Type) != SPVC_BASETYPE_IMAGE)
    {
        return 0;
    }

    const bool bArrayed      = spvc_type_get_image_arrayed(Type);
    const bool bMultisampled = spvc_type_get_image_multisampled(Type);

    EMSLTextureDimension Dimension = EMSLTextureDimension::Texture2D;
    switch (spvc_type_get_image_dimension(Type))
    {
        case SpvDim1D:
            Dimension = bArrayed ? EMSLTextureDimension::Texture1DArray : EMSLTextureDimension::Texture1D;
            break;
        case SpvDim2D:
            Dimension = bMultisampled ? EMSLTextureDimension::Texture2DMS : (bArrayed ? EMSLTextureDimension::Texture2DArray : EMSLTextureDimension::Texture2D);
            break;
        case SpvDimCube:
            Dimension = bArrayed ? EMSLTextureDimension::TextureCubeArray : EMSLTextureDimension::TextureCube;
            break;
        case SpvDim3D:
            Dimension = EMSLTextureDimension::Texture3D;
            break;
        case SpvDimBuffer:
            Dimension = EMSLTextureDimension::TextureBuffer;
            break;
        default:
            break;
    }

    EMSLTextureComponent Component = EMSLTextureComponent::Float;
    if (Resource.BindingType == EMSLBindingType::ShaderResourceTexture && spvc_compiler_variable_is_depth_or_compare(Compiler, Resource.Id))
    {
        Component = EMSLTextureComponent::Depth;
    }
    else
    {
        const spvc_type SampledType = spvc_compiler_get_type_handle(Compiler, spvc_type_get_image_sampled_type(Type));
        switch (spvc_type_get_basetype(SampledType))
        {
            case SPVC_BASETYPE_INT8:
            case SPVC_BASETYPE_INT16:
            case SPVC_BASETYPE_INT32:
            case SPVC_BASETYPE_INT64:
                Component = EMSLTextureComponent::Int;
                break;
            case SPVC_BASETYPE_UINT8:
            case SPVC_BASETYPE_UINT16:
            case SPVC_BASETYPE_UINT32:
            case SPVC_BASETYPE_UINT64:
                Component = EMSLTextureComponent::Uint;
                break;
            default:
                break;
        }
    }

    return MakeMSLNullTextureType(Dimension, Component);
}

static const CHAR* GetMSLTableName(EMSLBindingType BindingType)
{
    switch (GetMSLBindingTable(BindingType))
    {
        case EMSLBindingTable::Texture: return "texture";
        case EMSLBindingTable::Sampler: return "sampler";
        default:                        return "buffer";
    }
}

bool FMSLShaderConverter::Convert(const TArray<uint32>& Spirv, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, TArray<uint8>& OutMSLSource, String& OutErrors)
{
    if (Spirv.IsEmpty() || CompileInfo.EntryPoint.IsEmpty())
    {
        OutErrors += "The MSL conversion needs SPIR-V and an entry point\n";
        return false;
    }

    spvc_context Context = nullptr;
    if (spvc_context_create(&Context) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to create the SPIRV-Cross context\n";
        return false;
    }

    struct FContextGuard
    {
        ~FContextGuard() { spvc_context_destroy(Context); }
        spvc_context Context;
    } ContextGuard{ Context };

    spvc_parsed_ir ParsedCode = nullptr;
    if (spvc_context_parse_spirv(Context, reinterpret_cast<const SpvId*>(Spirv.Data()), Spirv.Size(), &ParsedCode) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to parse the SPIR-V\n";
        return false;
    }

    spvc_compiler CompilerMSL = nullptr;
    if (spvc_context_create_compiler(Context, SPVC_BACKEND_MSL, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &CompilerMSL) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to create the MSL compiler\n";
        return false;
    }

    const SpvExecutionModel ExecutionModel = GetMSLExecutionModel(CompileInfo.ShaderStage);

    // Ray-tracing libraries can hold several entry points, so the one being compiled is selected by name
    const spvc_entry_point* EntryPoints    = nullptr;
    size_t                  NumEntryPoints = 0;
    if (spvc_compiler_get_entry_points(CompilerMSL, &EntryPoints, &NumEntryPoints) == SPVC_SUCCESS)
    {
        for (size_t Index = 0; Index < NumEntryPoints; ++Index)
        {
            if (CompileInfo.EntryPoint == EntryPoints[Index].name)
            {
                spvc_compiler_set_entry_point(CompilerMSL, EntryPoints[Index].name, EntryPoints[Index].execution_model);
                break;
            }
        }
    }

    spvc_compiler_options CompilerOptions = nullptr;
    if (spvc_compiler_create_compiler_options(CompilerMSL, &CompilerOptions) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to create the MSL compiler options\n";
        return false;
    }

    uint32 MSLVersion = SPVC_MAKE_MSL_VERSION(2, 3, 0);
    switch (CompileInfo.ShaderStage)
    {
        case EShaderStage::Mesh:
        case EShaderStage::Amplification:
        case EShaderStage::RayGen:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayMiss:
        case EShaderStage::RayIntersection:
        case EShaderStage::RayCallable:
            MSLVersion = SPVC_MAKE_MSL_VERSION(3, 0, 0);
            break;

        default:
            break;
    }

    if (spvc_compiler_options_set_bool(CompilerOptions, SPVC_COMPILER_OPTION_MSL_TEXTURE_BUFFER_NATIVE, SPVC_TRUE) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to enable native MSL texel buffers\n";
        return false;
    }

    spvc_resources Resources = nullptr;
    if (spvc_compiler_create_shader_resources(CompilerMSL, &Resources) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to reflect the shader resources\n";
        return false;
    }

    constexpr spvc_resource_type ReflectedTypes[] =
    {
        SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
        SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
        SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
        SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
        SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
        SPVC_RESOURCE_TYPE_PUSH_CONSTANT,
        SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE,
    };

    TArray<FMSLReflectedResource> ReflectedResources;
    for (const spvc_resource_type ResourceType : ReflectedTypes)
    {
        if (!GatherMSLResources(CompilerMSL, Resources, ResourceType, ReflectedResources, OutErrors))
        {
            return false;
        }
    }

    bool bUsesBindlessHeaps = false;
    for (const FMSLReflectedResource& ReflectedResource : ReflectedResources)
    {
        if (ReflectedResource.BindingType == EMSLBindingType::BindlessResourceHeap || ReflectedResource.BindingType == EMSLBindingType::BindlessSamplerHeap)
        {
            bUsesBindlessHeaps = true;
            break;
        }
    }

    // intersection_query needs MSL 2.4; 3.0 keeps one version for every ray tracing shader
    if (bUsesBindlessHeaps || UsesSpirvCapability(CompilerMSL, SpvCapabilityRayQueryKHR))
    {
        MSLVersion = SPVC_MAKE_MSL_VERSION(3, 0, 0);
    }

    if (spvc_compiler_options_set_uint(CompilerOptions, SPVC_COMPILER_OPTION_MSL_VERSION, MSLVersion) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to set the MSL version\n";
        return false;
    }

    if (bUsesBindlessHeaps)
    {
        // SPIRV-Cross refuses runtime-array heaps unless this option is Tier2. Argument buffers stay off;
        // the heaps still emit as `device const void* [[buffer(n)]]` rather than nested argument buffers.
        if (spvc_compiler_options_set_uint(CompilerOptions, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS_TIER, 1) != SPVC_SUCCESS)
        {
            OutErrors += "Failed to set the MSL argument-buffer tier for bindless heaps\n";
            return false;
        }
    }

    if (spvc_compiler_install_compiler_options(CompilerMSL, CompilerOptions) != SPVC_SUCCESS)
    {
        OutErrors += "Failed to install the MSL compiler options\n";
        return false;
    }

    // DXC numbers a Vulkan binding after the HLSL register, so b0 and u0 both land on set 0 binding 0.
    // SPIRV-Cross answers that collision by emitting one `constant void*` argument that every alias
    // casts out of, which Metal rejects the moment one of them needs the device address space. Handing
    // every resource its own binding first is what keeps the aliasing path from triggering at all.
    // Heap resources stay on set 31 so the runtime-array table ABI is preserved.
    uint32 DiscreteBinding = 0;
    for (const FMSLReflectedResource& ReflectedResource : ReflectedResources)
    {
        if (ReflectedResource.BindingType == EMSLBindingType::BindlessResourceHeap || ReflectedResource.BindingType == EMSLBindingType::BindlessSamplerHeap)
        {
            continue;
        }

        spvc_compiler_set_decoration(CompilerMSL, ReflectedResource.Id, SpvDecorationBinding, DiscreteBinding);
        spvc_compiler_set_decoration(CompilerMSL, ReflectedResource.Id, SpvDecorationDescriptorSet, 0);
        DiscreteBinding++;
    }

    if (bUsesBindlessHeaps)
    {
        auto AddHeapBinding = [&](unsigned Binding, unsigned BufferIndex) -> bool
        {
            spvc_msl_resource_binding ResourceBinding;
            spvc_msl_resource_binding_init(&ResourceBinding);

            ResourceBinding.stage    = ExecutionModel;
            ResourceBinding.desc_set = MSL_BINDLESS_HEAP_MARKER_SET;
            ResourceBinding.binding  = Binding;

            // Runtime-array heaps still emit as [[buffer(n)]], but SPIRV-Cross picks
            // msl_texture / msl_sampler when the SPIR-V type is Image or Sampler.
            ResourceBinding.msl_buffer  = BufferIndex;
            ResourceBinding.msl_texture = BufferIndex;
            ResourceBinding.msl_sampler = BufferIndex;

            return spvc_compiler_msl_add_resource_binding(CompilerMSL, &ResourceBinding) == SPVC_SUCCESS;
        };

        if (!AddHeapBinding(MSL_BINDLESS_RESOURCE_BINDING, MSL_BINDLESS_RESOURCE_HEAP_BUFFER_INDEX) ||
            !AddHeapBinding(MSL_BINDLESS_SAMPLER_BINDING, MSL_BINDLESS_SAMPLER_HEAP_BUFFER_INDEX))
        {
            OutErrors += "Failed to pin the MSL bindless heap buffer slots\n";
            return false;
        }
    }

    const CHAR* MSLSource = nullptr;
    if (spvc_compiler_compile(CompilerMSL, &MSLSource) != SPVC_SUCCESS || !MSLSource)
    {
        OutErrors += "Failed to create the MSL\n";
        return false;
    }

    // The MSL argument tables are only settled once the source exists, so the slots are read back here
    // and shipped with the source. Nothing in MSL records the HLSL register, so without this table the
    // RHI has no way to tell which argument a given b#, t#, u# or s# ended up as.
    TArray<EMSLBindingType> CollectedTypes;
    for (const FMSLReflectedResource& ReflectedResource : ReflectedResources)
    {
        const unsigned Slot = spvc_compiler_msl_get_automatic_resource_binding(CompilerMSL, ReflectedResource.Id);

        // An unused resource is stripped from the MSL and owns no slot. It stays out of the table and
        // the RHI simply never binds that register.
        if (Slot == ~0u)
        {
            continue;
        }

        const uint8 TableLimit = GetMSLMaxSlotCount(ReflectedResource.BindingType);
        if (Slot > 0xFF || Slot >= TableLimit)
        {
            OutErrors += String::Printf("%s register %u resolved to MSL slot %u, which exceeds the %u-slot %s table\n",
                ToString(ReflectedResource.BindingType), ReflectedResource.RegisterIndex, Slot, TableLimit, GetMSLTableName(ReflectedResource.BindingType));
            return false;
        }

        // Two resources sharing a register take separate MSL slots, and the binding table the RHI
        // builds from this array holds one slot per register. DXC does not diagnose the overlap when
        // it lowers to SPIR-V, so it has to be caught here while the shader can still be named.
        if (ReflectedResource.BindingType != EMSLBindingType::ShaderConstants &&
            ReflectedResource.BindingType != EMSLBindingType::BindlessResourceHeap &&
            ReflectedResource.BindingType != EMSLBindingType::BindlessSamplerHeap)
        {
            for (int32 Existing = 0; Existing < OutReflection.Bindings.Size(); ++Existing)
            {
                if (CollectedTypes[Existing] == ReflectedResource.BindingType && OutReflection.Bindings[Existing].Register == ReflectedResource.RegisterIndex)
                {
                    OutErrors += String::Printf("'%s' declares two %s resources on register %u\n", *CompileInfo.EntryPoint, ToString(ReflectedResource.BindingType), ReflectedResource.RegisterIndex);
                    return false;
                }
            }
        }

        const CHAR* Name = spvc_compiler_get_name(CompilerMSL, ReflectedResource.Id);
        if (!FShaderReflectionUtils::AddBinding(Name, ReflectedResource.Type, ReflectedResource.Dimension, ReflectedResource.Space, ReflectedResource.RegisterIndex, 1, CompileInfo.bDebugInfo, OutReflection, OutErrors))
        {
            return false;
        }

        FMSLBindingSlot& MSLSlot = OutReflection.MSLSlots.Emplace();
        MSLSlot.Slot            = static_cast<uint8>(Slot);
        MSLSlot.NullTextureType = GetMSLNullTextureType(CompilerMSL, ReflectedResource);

        CollectedTypes.Add(ReflectedResource.BindingType);
    }

    size_t ShaderConstantsSize = 0;
    for (const FMSLReflectedResource& ReflectedResource : ReflectedResources)
    {
        if (ReflectedResource.BindingType != EMSLBindingType::ShaderConstants)
        {
            continue;
        }

        const spvc_type Type = spvc_compiler_get_type_handle(CompilerMSL, ReflectedResource.BaseTypeId);
        size_t StructSize = 0;
        if (spvc_compiler_get_declared_struct_size(CompilerMSL, Type, &StructSize) != SPVC_SUCCESS)
        {
            OutErrors += "Failed to reflect the shader-constant block size\n";
            return false;
        }

        ShaderConstantsSize = Math::Max(ShaderConstantsSize, StructSize);
    }

    if (ShaderConstantsSize > 0 && !FShaderReflectionUtils::SetShaderConstantsSize("ShaderConstants", static_cast<uint32>(ShaderConstantsSize), OutReflection, OutErrors))
    {
        return false;
    }

    if (CompileInfo.ShaderStage == EShaderStage::Compute || CompileInfo.ShaderStage == EShaderStage::Mesh || CompileInfo.ShaderStage == EShaderStage::Amplification)
    {
        for (uint32 Axis = 0; Axis < 3; ++Axis)
        {
            OutReflection.MSLInfo.ThreadGroupSize[Axis] = static_cast<uint16>(spvc_compiler_get_execution_mode_argument_by_index(CompilerMSL, SpvExecutionModeLocalSize, Axis));
        }
    }

    // The entry point lives in OpEntryPoint, so stripping OpName never changes the name Metal looks up
    const CHAR* CleansedEntryPoint = spvc_compiler_get_cleansed_entry_point_name(CompilerMSL, *CompileInfo.EntryPoint, ExecutionModel);
    OutReflection.EntryPoint = CleansedEntryPoint && CleansedEntryPoint[0] != '\0' ? String(CleansedEntryPoint) : CompileInfo.EntryPoint;

    const int32 SourceLength = static_cast<int32>(CString::Strlen(MSLSource));
    OutMSLSource = TArray<uint8>(reinterpret_cast<const uint8*>(MSLSource), SourceLength);
    return true;
}
