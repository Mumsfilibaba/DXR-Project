#include "ShaderCore/ShaderBindingConventions.h"
#include "ShaderCompiler/Reflection/ShaderReflectionUtils.h"

bool FShaderReflectionUtils::AddBinding(const CHAR* Name, EShaderResourceType Type, EShaderResourceDimension Dimension, EShaderBindingSpace Space, uint32 Register, uint32 Count, bool bKeepName, FShaderReflection& OutReflection, String& OutErrors)
{
    const uint32 BindingCount = Count == 0 ? 1 : Count;
    if (Register > 0xFF || BindingCount > 0xFF)
    {
        OutErrors += String::Printf("Shader Parameter '%s' at register %u with %u elements is outside the supported range (register and count at most 255)\n", Name, Register, BindingCount);
        return false;
    }

    if (OutReflection.Bindings.Size() >= 0xFF)
    {
        OutErrors += "The shader has more than 255 bindings\n";
        return false;
    }

    FShaderResourceBinding& Binding = OutReflection.Bindings.Emplace();
    Binding.Type      = Type;
    Binding.Dimension = Dimension;
    Binding.Space     = Space;
    Binding.Register  = static_cast<uint8>(Register);
    Binding.Count     = static_cast<uint8>(BindingCount);

    if (bKeepName)
    {
        OutReflection.BindingNames.Emplace(Name ? Name : "");
    }

    return true;
}

bool FShaderReflectionUtils::SetShaderConstantsSize(const CHAR* Name, uint32 SizeInBytes, FShaderReflection& OutReflection, String& OutErrors)
{
    constexpr uint32 MaxSizeInBytes = ShaderBindings::MaxShaderConstants * sizeof(uint32);

    if (OutReflection.Info.ShaderConstantsSize != 0)
    {
        OutErrors += String::Printf("Shader Parameter '%s' declares a second block of shader constants, only one is supported per shader\n", Name);
        return false;
    }

    if (SizeInBytes == 0)
    {
        OutErrors += String::Printf("Shader Parameter '%s' is a block of shader constants, but its size could not be retrieved from reflection\n", Name);
        return false;
    }

    if (SizeInBytes > MaxSizeInBytes)
    {
        OutErrors += String::Printf("Shader Parameter '%s' is %u bytes, which exceeds the maximum of %u bytes of shader constants\n", Name, SizeInBytes, MaxSizeInBytes);
        return false;
    }

    OutReflection.Info.ShaderConstantsSize = static_cast<uint8>(SizeInBytes);
    return true;
}

bool FShaderReflectionUtils::SortAndValidateVertexInputs(TArray<FShaderVertexInput>& InOutVertexInputs, String& OutErrors)
{
    // Insertion sort: a vertex shader has a handful of inputs
    for (int32 Index = 1; Index < InOutVertexInputs.Size(); ++Index)
    {
        const FShaderVertexInput Current = InOutVertexInputs[Index];

        int32 Insert = Index;
        while (Insert > 0 && InOutVertexInputs[Insert - 1].Location > Current.Location)
        {
            InOutVertexInputs[Insert] = InOutVertexInputs[Insert - 1];
            --Insert;
        }

        InOutVertexInputs[Insert] = Current;
    }

    for (int32 Index = 0; Index < InOutVertexInputs.Size(); ++Index)
    {
        for (int32 Other = Index + 1; Other < InOutVertexInputs.Size(); ++Other)
        {
            if (InOutVertexInputs[Index].SemanticHash == InOutVertexInputs[Other].SemanticHash && InOutVertexInputs[Index].SemanticIndex == InOutVertexInputs[Other].SemanticIndex)
            {
                OutErrors += String::Printf("The vertex inputs at locations %u and %u have the same semantic\n", InOutVertexInputs[Index].Location, InOutVertexInputs[Other].Location);
                return false;
            }
        }
    }

    return true;
}
