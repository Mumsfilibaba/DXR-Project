#include "Core/Math/Math.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Templates/CString.h"
#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"
#include "ShaderCompiler/Reflection/SpirvShaderReflector.h"

#include <spirv_cross_c.h>

class FScopedSpvcContext
{
public:
    FScopedSpvcContext()
        : Context(nullptr)
    {
        if (spvc_context_create(&Context) != SPVC_SUCCESS)
        {
            Context = nullptr;
        }
    }

    ~FScopedSpvcContext()
    {
        if (Context)
        {
            spvc_context_destroy(Context);
        }
    }

    spvc_context Get() const
    {
        return Context;
    }

private:
    spvc_context Context;
};

static spvc_compiler CreateReflectionCompiler(const FScopedSpvcContext& Context, const TArray<uint32>& Spirv, FString& OutErrors)
{
    if (!Context.Get())
    {
        OutErrors += TEXT("Failed to create the SPIRV-Cross context\n");
        return nullptr;
    }

    // The context lives in the caller, which outlives every spvc_* call made with OutErrors still in scope
    spvc_context_set_error_callback(Context.Get(), [](void* UserData, const ANSICHAR* Error)
    {
        *static_cast<FString*>(UserData) += FString::Printf(TEXT("SPIRV-Cross: %s\n"), StringCast<CHAR_T>(Error).Get());
    }, &OutErrors);

    spvc_parsed_ir ParsedCode = nullptr;
    if (spvc_context_parse_spirv(Context.Get(), reinterpret_cast<const SpvId*>(Spirv.Data()), Spirv.Size(), &ParsedCode) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to parse the SPIR-V\n");
        return nullptr;
    }

    spvc_compiler Compiler = nullptr;
    if (spvc_context_create_compiler(Context.Get(), SPVC_BACKEND_NONE, ParsedCode, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &Compiler) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to create the SPIRV-Cross reflection compiler\n");
        return nullptr;
    }

    return Compiler;
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

static uint32 GetArraySize(spvc_compiler Compiler, spvc_type_id TypeId)
{
    const spvc_type Type = spvc_compiler_get_type_handle(Compiler, TypeId);
    if (spvc_type_get_num_array_dimensions(Type) == 0 || !spvc_type_array_dimension_is_literal(Type, 0))
    {
        return 1;
    }

    const uint32 Size = static_cast<uint32>(spvc_type_get_array_dimension(Type, 0));
    return Size == 0 ? 1 : Size;
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

static bool AddResources(spvc_compiler Compiler, spvc_resources Resources, spvc_resource_type ResourceType, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, FString& OutErrors)
{
    size_t                         NumResources = 0;
    const spvc_reflected_resource* Reflected    = nullptr;
    if (spvc_resources_get_resource_list_for_type(Resources, ResourceType, &Reflected, &NumResources) != SPVC_SUCCESS)
    {
        OutErrors += FString::Printf(TEXT("Failed to reflect the SPIR-V resources of type %d\n"), static_cast<int32>(ResourceType));
        return false;
    }

    for (size_t Index = 0; Index < NumResources; Index++)
    {
        const spvc_reflected_resource& Resource = Reflected[Index];

        const uint32 Set        = spvc_compiler_get_decoration(Compiler, Resource.id, SpvDecorationDescriptorSet);
        const uint32 RawBinding = spvc_compiler_get_decoration(Compiler, Resource.id, SpvDecorationBinding);

        const TStringConversion<CHAR_T, ANSICHAR> Name = StringCast<CHAR_T>(spvc_compiler_get_name(Compiler, Resource.id));

        uint32 BindingOffset = UINT32_MAX;
        uint32 SetOffset     = UINT32_MAX;
        if (!spvc_compiler_get_binary_offset_for_decoration(Compiler, Resource.id, SpvDecorationBinding, &BindingOffset) ||
            !spvc_compiler_get_binary_offset_for_decoration(Compiler, Resource.id, SpvDecorationDescriptorSet, &SetOffset))
        {
            OutErrors += FString::Printf(TEXT("Resource '%s' at binding %u has no DescriptorSet or Binding decoration to patch\n"), Name.Get(), RawBinding);
            return false;
        }

        if (BindingOffset >= 0xFFFF || SetOffset >= 0xFFFF)
        {
            OutErrors += FString::Printf(TEXT("Resource '%s' at binding %u is decorated past word 65535 of the module, which the container cannot record\n"), Name.Get(), RawBinding);
            return false;
        }

        EShaderResourceType      Type      = EShaderResourceType::ConstantBuffer;
        EShaderResourceDimension Dimension = EShaderResourceDimension::Unknown;
        switch (ResourceType)
        {
            case SPVC_RESOURCE_TYPE_SEPARATE_IMAGE:
            {
                Dimension = GetImageDimension(Compiler, Resource.base_type_id);
                Type      = Dimension == EShaderResourceDimension::Buffer ? EShaderResourceType::TypedBuffer : EShaderResourceType::Texture;
                break;
            }

            case SPVC_RESOURCE_TYPE_STORAGE_IMAGE:
            {
                Dimension = GetImageDimension(Compiler, Resource.base_type_id);
                Type      = Dimension == EShaderResourceDimension::Buffer ? EShaderResourceType::RWTypedBuffer : EShaderResourceType::RWTexture;
                break;
            }

            case SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS:
            {
                Type = EShaderResourceType::Sampler;
                break;
            }

            case SPVC_RESOURCE_TYPE_UNIFORM_BUFFER:
            {
                Type = EShaderResourceType::ConstantBuffer;
                break;
            }

            case SPVC_RESOURCE_TYPE_STORAGE_BUFFER:
            {
                bool bIsReadOnly = false;
                if (!IsStorageBufferReadOnly(Compiler, Resource.id, bIsReadOnly))
                {
                    OutErrors += FString::Printf(TEXT("Failed to read buffer block decorations for storage buffer at register %u\n"), RawBinding);
                    return false;
                }

                Type = bIsReadOnly ? EShaderResourceType::StructuredBuffer : EShaderResourceType::RWStructuredBuffer;
                break;
            }

            case SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE:
            {
                Type = EShaderResourceType::AccelerationStructure;
                break;
            }

            default:
            {
                OutErrors += FString::Printf(TEXT("Unhandled SPIR-V resource type %d\n"), static_cast<int32>(ResourceType));
                return false;
            }
        }

        EShaderBindingSpace Space = EShaderBindingSpace::Global;
        if (Set == ShaderBindings::SpirvHeapMarkerSet)
        {
            const uint32 ExpectedBinding = ResourceType == SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS ? ShaderBindings::SpirvHeapSamplerBinding : ShaderBindings::SpirvHeapResourceBinding;
            if (ResourceType == SPVC_RESOURCE_TYPE_STORAGE_BUFFER && RawBinding == ShaderBindings::SpirvHeapCounterBinding)
            {
                OutErrors += TEXT("Shader takes a counter on a heap-indexed RW/Append/Consume buffer. "
                    "The bindless heap has no counter descriptors; use an explicit RWByteAddressBuffer counter at a regular register instead.\n");
                return false;
            }

            if (RawBinding != ExpectedBinding)
            {
                OutErrors += FString::Printf(TEXT("Resource at marker set %u must sit at binding %u (got %u). HLSL must not declare regular resources at space%u.\n"),
                    ShaderBindings::SpirvHeapMarkerSet, ExpectedBinding, RawBinding, ShaderBindings::SpirvHeapMarkerSet);
                return false;
            }

            // Every heap-indexed storage buffer is bound as a storage buffer descriptor, writable or not
            if (Type == EShaderResourceType::RWStructuredBuffer)
            {
                Type = EShaderResourceType::StructuredBuffer;
            }

            Space = EShaderBindingSpace::BindlessHeap;
        }
        else if (Set == ShaderBindings::RayTracingLocalSpace)
        {
            Space = EShaderBindingSpace::RayTracingLocal;
        }
        else if (Set == ShaderBindings::ShaderConstantsSpace)
        {
            LOG_WARNING("[FSpirvShaderReflector]: Descriptor resource '%s' declared in space%u (reserved for 32-bit constants), register %u; treating it as a global-space resource",
                Name.Get(), ShaderBindings::ShaderConstantsSpace, RawBinding);
        }

        const uint32 Count = GetArraySize(Compiler, Resource.type_id);
        if (!FShaderReflectionUtils::AddBinding(Name.Get(), Type, Dimension, Space, RawBinding, Count, CompileInfo.bDebugInfo, OutReflection, OutErrors))
        {
            return false;
        }

        FSpirvBindingOffsets& Offsets = OutReflection.SpirvOffsets.Emplace();
        Offsets.SetWordOffset     = static_cast<uint16>(SetOffset);
        Offsets.BindingWordOffset = static_cast<uint16>(BindingOffset);
    }

    return true;
}

static bool ReflectPushConstants(spvc_compiler Compiler, spvc_resources Resources, FShaderReflection& OutReflection, FString& OutErrors)
{
    size_t                         NumPushConstants = 0;
    const spvc_reflected_resource* PushConstants    = nullptr;
    if (spvc_resources_get_resource_list_for_type(Resources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT, &PushConstants, &NumPushConstants) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to reflect the SPIR-V push constants\n");
        return false;
    }

    size_t NumPushBytes = 0;
    for (size_t Index = 0; Index < NumPushConstants; Index++)
    {
        size_t StructSize = 0;
        const spvc_type Type = spvc_compiler_get_type_handle(Compiler, PushConstants[Index].base_type_id);
        if (spvc_compiler_get_declared_struct_size(Compiler, Type, &StructSize) != SPVC_SUCCESS)
        {
            OutErrors += TEXT("Failed to retrieve the size of the push constants\n");
            return false;
        }

        NumPushBytes = Math::Max(NumPushBytes, StructSize);
    }

    if (NumPushBytes == 0)
    {
        return true;
    }

    return FShaderReflectionUtils::SetShaderConstantsSize(TEXT("PushConstants"), static_cast<uint32>(NumPushBytes), OutReflection, OutErrors);
}

bool FSpirvShaderReflector::Reflect(const TArray<uint32>& Spirv, const FShaderCompileInfo& CompileInfo, FShaderReflection& OutReflection, FString& OutErrors)
{
    FScopedSpvcContext Context;
    spvc_compiler Compiler = CreateReflectionCompiler(Context, Spirv, OutErrors);
    if (!Compiler)
    {
        return false;
    }

    const spvc_entry_point* EntryPoints    = nullptr;
    size_t                  NumEntryPoints = 0;
    if (spvc_compiler_get_entry_points(Compiler, &EntryPoints, &NumEntryPoints) != SPVC_SUCCESS || NumEntryPoints == 0)
    {
        OutErrors += TEXT("The SPIR-V module has no entry points\n");
        return false;
    }

    const spvc_entry_point* EntryPoint = nullptr;
    for (size_t Index = 0; Index < NumEntryPoints && !EntryPoint; ++Index)
    {
        if (CompileInfo.EntryPoint == StringCast<CHAR_T>(EntryPoints[Index].name).Get())
        {
            EntryPoint = &EntryPoints[Index];
        }
    }

    // A module with a single entry point is unambiguous, whatever DXC named it
    if (!EntryPoint && NumEntryPoints == 1)
    {
        EntryPoint = &EntryPoints[0];
    }

    if (!EntryPoint || spvc_compiler_set_entry_point(Compiler, EntryPoint->name, EntryPoint->execution_model) != SPVC_SUCCESS)
    {
        OutErrors += FString::Printf(TEXT("The SPIR-V module has no entry point named '%s'\n"), *CompileInfo.EntryPoint);
        return false;
    }

    OutReflection.EntryPoint = StringCast<CHAR_T>(EntryPoint->name).ToString();

    spvc_resources Resources = nullptr;
    if (spvc_compiler_create_shader_resources(Compiler, &Resources) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to create the SPIR-V shader resources\n");
        return false;
    }

    // The order VulkanRHI assigns descriptor slots in
    constexpr spvc_resource_type ResourceTypes[] =
    {
        SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
        SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
        SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
        SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
        SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
        SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE,
    };

    for (spvc_resource_type ResourceType : ResourceTypes)
    {
        if (!AddResources(Compiler, Resources, ResourceType, CompileInfo, OutReflection, OutErrors))
        {
            return false;
        }
    }

    return ReflectPushConstants(Compiler, Resources, OutReflection, OutErrors);
}

static bool SplitSemantic(const CHAR_T* Semantic, FString& OutName, uint32& OutIndex)
{
    const int32 Length = static_cast<int32>(CString::Strlen(Semantic));

    int32 DigitsStart = Length;
    while (DigitsStart > 0 && Semantic[DigitsStart - 1] >= '0' && Semantic[DigitsStart - 1] <= '9')
    {
        --DigitsStart;
    }

    OutName  = FString(Semantic, DigitsStart);
    OutIndex = 0;
    for (int32 Index = DigitsStart; Index < Length; ++Index)
    {
        OutIndex = OutIndex * 10 + static_cast<uint32>(Semantic[Index] - '0');
    }

    return !OutName.IsEmpty();
}

static EShaderComponentType GetComponentType(spvc_type Type)
{
    switch (spvc_type_get_basetype(Type))
    {
        case SPVC_BASETYPE_FP32:   return EShaderComponentType::Float32;
        case SPVC_BASETYPE_INT32:  return EShaderComponentType::Int32;
        case SPVC_BASETYPE_UINT32: return EShaderComponentType::Uint32;
        case SPVC_BASETYPE_FP16:   return EShaderComponentType::Float16;
        case SPVC_BASETYPE_INT16:  return EShaderComponentType::Int16;
        case SPVC_BASETYPE_UINT16: return EShaderComponentType::Uint16;
        default:                   return EShaderComponentType::Unknown;
    }
}

bool FSpirvShaderReflector::ReflectVertexInputs(const TArray<uint32>& UntransformedSpirv, FShaderReflection& OutReflection, FString& OutErrors)
{
    FScopedSpvcContext Context;
    spvc_compiler Compiler = CreateReflectionCompiler(Context, UntransformedSpirv, OutErrors);
    if (!Compiler)
    {
        return false;
    }

    spvc_resources Resources = nullptr;
    if (spvc_compiler_create_shader_resources(Compiler, &Resources) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to create the SPIR-V shader resources\n");
        return false;
    }

    size_t                         NumInputs = 0;
    const spvc_reflected_resource* Inputs    = nullptr;
    if (spvc_resources_get_resource_list_for_type(Resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &Inputs, &NumInputs) != SPVC_SUCCESS)
    {
        OutErrors += TEXT("Failed to reflect the SPIR-V stage inputs\n");
        return false;
    }

    for (size_t Index = 0; Index < NumInputs; Index++)
    {
        const spvc_variable_id Id = Inputs[Index].id;
        if (!spvc_compiler_has_decoration(Compiler, Id, SpvDecorationLocation))
        {
            continue;
        }

        const ANSICHAR* Semantic = spvc_compiler_get_decoration_string(Compiler, Id, SpvDecorationUserSemantic);
        if (!Semantic || Semantic[0] == '\0')
        {
            OutErrors += FString::Printf(TEXT("Vertex input '%s' has no HLSL semantic, the vertex shader must be compiled with -fspv-reflect\n"), StringCast<CHAR_T>(spvc_compiler_get_name(Compiler, Id)).Get());
            return false;
        }

        FString SemanticName;
        uint32 SemanticIndex = 0;
        if (!SplitSemantic(StringCast<CHAR_T>(Semantic).Get(), SemanticName, SemanticIndex))
        {
            OutErrors += FString::Printf(TEXT("Vertex input semantic '%s' has no name\n"), StringCast<CHAR_T>(Semantic).Get());
            return false;
        }

        const uint32 Location = spvc_compiler_get_decoration(Compiler, Id, SpvDecorationLocation);
        if (Location > 0xFF || SemanticIndex > 0xFF)
        {
            OutErrors += FString::Printf(TEXT("Vertex input '%s' at location %u is outside the supported range\n"), Semantic, Location);
            return false;
        }

        const spvc_type Type = spvc_compiler_get_type_handle(Compiler, Inputs[Index].type_id);

        FShaderVertexInput& Input = OutReflection.VertexInputs.Emplace();
        Input.SemanticHash  = HashShaderSemantic(FStringView(SemanticName));
        Input.SemanticIndex = static_cast<uint8>(SemanticIndex);
        Input.Location      = static_cast<uint8>(Location);
        Input.ComponentType = GetComponentType(Type);
        Input.NumComponents = static_cast<uint8>(Math::Clamp<uint32>(spvc_type_get_vector_size(Type), 1, 4));
    }

    return FShaderReflectionUtils::SortAndValidateVertexInputs(OutReflection.VertexInputs, OutErrors);
}
