#include "Core/Memory/Memory.h"
#include "ShaderCore/ShaderCode.h"

static bool SetError(String* OutError, const CHAR* Message)
{
    if (OutError)
    {
        *OutError = Message;
    }

    return false;
}

template<typename PodType>
static void AppendPod(TArray<uint8>& Bytes, const PodType& Value)
{
    Bytes.Append(reinterpret_cast<const uint8*>(&Value), static_cast<int32>(sizeof(PodType)));
}

template<typename PodType>
static void AppendArray(TArray<uint8>& Bytes, const TArray<PodType>& Values)
{
    if (!Values.IsEmpty())
    {
        Bytes.Append(reinterpret_cast<const uint8*>(Values.Data()), Values.SizeInBytes());
    }
}

static void AppendString(TArray<uint8>& Bytes, const String& Value)
{
    if (!Value.IsEmpty())
    {
        Bytes.Append(reinterpret_cast<const uint8*>(Value.Data()), Value.Size());
    }

    Bytes.Add(0);
}

static void PadTo(TArray<uint8>& Bytes, uint32 Alignment)
{
    while (static_cast<uint32>(Bytes.Size()) % Alignment != 0)
    {
        Bytes.Add(0);
    }
}

static uint32 AlignOffset(uint32 Offset, uint32 Alignment)
{
    return (Offset + Alignment - 1) & ~(Alignment - 1);
}

bool FShaderCodeWriter::Write(EShaderOutputLanguage OutputLanguage, EShaderStage Stage, EShaderCodeFlags Flags, const FShaderReflection& Reflection, TArrayView<const uint8> NativeCode, TArray<uint8>& OutShaderCode, String* OutError)
{
    const bool  bHasDebugInfo = IsEnumFlagSet(Flags, EShaderCodeFlags::DebugInfo);
    const int32 NumBindings   = Reflection.Bindings.Size();

    if (NumBindings > 0xFF)
    {
        return SetError(OutError, "More than 255 bindings");
    }

    if (Reflection.SpirvOffsets.Size() != (OutputLanguage == EShaderOutputLanguage::SPIRV ? NumBindings : 0))
    {
        return SetError(OutError, "SpirvOffsets must have one entry per binding for SPIR-V, and none otherwise");
    }

    if (Reflection.BindingNames.Size() != (bHasDebugInfo ? NumBindings : 0))
    {
        return SetError(OutError, "BindingNames must have one entry per binding in a DebugInfo container, and none otherwise");
    }

    if (Reflection.MSLSlots.Size() != (OutputLanguage == EShaderOutputLanguage::MSL ? NumBindings : 0))
    {
        return SetError(OutError, "MSLSlots must have one entry per binding for MSL, and none otherwise");
    }

    if (Reflection.VertexInputs.Size() > 0xFF || (!Reflection.VertexInputs.IsEmpty() && Stage != EShaderStage::Vertex))
    {
        return SetError(OutError, "Only vertex shaders have vertex inputs, at most 255 of them");
    }

    FShaderReflectionInfo Info = Reflection.Info;
    Info.NumVertexInputs = static_cast<uint8>(Reflection.VertexInputs.Size());
    Info.NumBindings     = static_cast<uint8>(NumBindings);

    // Header, info, vertex inputs and bindings. The vertex inputs start at offset 20, so their uint32 hashes are aligned.
    TArray<uint8> Bytes;
    Bytes.Reserve(static_cast<int32>(sizeof(FShaderCodeHeader) + sizeof(FShaderReflectionInfo)) + NativeCode.Size() + 256);

    FShaderCodeHeader Header;
    Header.OutputLanguage = OutputLanguage;
    Header.Stage          = Stage;
    Header.Flags          = Flags;

    AppendPod(Bytes, Header);
    AppendPod(Bytes, Info);
    AppendArray(Bytes, Reflection.VertexInputs);
    AppendArray(Bytes, Reflection.Bindings);

    // SPIR-V offsets and the MSL info are read in place as uint16
    if (!Reflection.SpirvOffsets.IsEmpty())
    {
        PadTo(Bytes, alignof(FSpirvBindingOffsets));
        AppendArray(Bytes, Reflection.SpirvOffsets);
    }

    if (OutputLanguage == EShaderOutputLanguage::MSL)
    {
        PadTo(Bytes, alignof(FMSLShaderInfo));
        AppendPod(Bytes, Reflection.MSLInfo);
        AppendArray(Bytes, Reflection.MSLSlots);
    }

    // Strings: the entry point always, the names only in DebugInfo containers
    AppendString(Bytes, Reflection.EntryPoint);
    for (const String& Name : Reflection.BindingNames)
    {
        AppendString(Bytes, Name);
    }

    // The native code
    PadTo(Bytes, CodeAlignment);

    const uint32 CodeOffset = static_cast<uint32>(Bytes.Size());
    Memory::Memcpy(Bytes.Data() + offsetof(FShaderCodeHeader, CodeOffset), &CodeOffset, sizeof(CodeOffset));

    if (!NativeCode.IsEmpty())
    {
        Bytes.Append(NativeCode.Data(), NativeCode.Size());
    }

    OutShaderCode = ::Move(Bytes);
    return true;
}

bool FShaderCodeReader::ReadHeader(const TArray<uint8>& ShaderCode, FShaderCodeHeader& OutHeader, String* OutError)
{
    constexpr uint32 MinReflectionEnd = static_cast<uint32>(sizeof(FShaderCodeHeader) + sizeof(FShaderReflectionInfo));

    if (ShaderCode.Size() < static_cast<int32>(sizeof(FShaderCodeHeader)))
    {
        return SetError(OutError, "The shader code is smaller than the header");
    }

    Memory::Memcpy(&OutHeader, ShaderCode.Data(), sizeof(FShaderCodeHeader));

    if (OutHeader.Magic != FShaderCodeHeader::MagicValue)
    {
        return SetError(OutError, "The shader code has the wrong magic");
    }

    if (OutHeader.Version != FShaderCodeHeader::CurrentVersion)
    {
        return SetError(OutError, "The shader code was written by another version of the container");
    }

    if (OutHeader.OutputLanguage == EShaderOutputLanguage::Unknown || static_cast<uint8>(OutHeader.OutputLanguage) > static_cast<uint8>(EShaderOutputLanguage::DXBC))
    {
        return SetError(OutError, "The shader code has an unknown output language");
    }

    if (OutHeader.Stage == EShaderStage::Unknown || static_cast<uint8>(OutHeader.Stage) > static_cast<uint8>(EShaderStage::RayCallable))
    {
        return SetError(OutError, "The shader code has an unknown stage");
    }

    if (OutHeader.CodeOffset < MinReflectionEnd || OutHeader.CodeOffset > static_cast<uint32>(ShaderCode.Size()) || (OutHeader.CodeOffset % FShaderCodeWriter::CodeAlignment) != 0)
    {
        return SetError(OutError, "The native code offset is unaligned or outside the shader code");
    }

    return true;
}

bool FShaderCodeReader::Read(const TArray<uint8>& ShaderCode, FShaderCodeView& OutView, String* OutError)
{
    FShaderCodeView View;
    if (!ReadHeader(ShaderCode, View.Header, OutError))
    {
        return false;
    }

    const uint8* const Bytes         = ShaderCode.Data();
    const uint32       ReflectionEnd = View.Header.CodeOffset;

    uint32 Offset = static_cast<uint32>(sizeof(FShaderCodeHeader));
    Memory::Memcpy(&View.Info, Bytes + Offset, sizeof(FShaderReflectionInfo));
    Offset += static_cast<uint32>(sizeof(FShaderReflectionInfo));

    const uint32 NumVertexInputs = View.Info.NumVertexInputs;
    const uint32 NumBindings     = View.Info.NumBindings;

    if (NumVertexInputs > 0 && View.Header.Stage != EShaderStage::Vertex)
    {
        return SetError(OutError, "Only vertex shaders have vertex inputs");
    }

    // Vertex inputs
    if (Offset + NumVertexInputs * sizeof(FShaderVertexInput) > ReflectionEnd)
    {
        return SetError(OutError, "The vertex inputs run past the reflection block");
    }

    View.VertexInputs = TArrayView<const FShaderVertexInput>(reinterpret_cast<const FShaderVertexInput*>(Bytes + Offset), static_cast<int32>(NumVertexInputs));
    Offset += NumVertexInputs * static_cast<uint32>(sizeof(FShaderVertexInput));

    for (const FShaderVertexInput& VertexInput : View.VertexInputs)
    {
        if (static_cast<uint8>(VertexInput.ComponentType) >= static_cast<uint8>(EShaderComponentType::Count) || VertexInput.NumComponents < 1 || VertexInput.NumComponents > 4)
        {
            return SetError(OutError, "A vertex input has an invalid component type or count");
        }
    }

    // Bindings
    if (Offset + NumBindings * sizeof(FShaderResourceBinding) > ReflectionEnd)
    {
        return SetError(OutError, "The bindings run past the reflection block");
    }

    View.Bindings = TArrayView<const FShaderResourceBinding>(reinterpret_cast<const FShaderResourceBinding*>(Bytes + Offset), static_cast<int32>(NumBindings));
    Offset += NumBindings * static_cast<uint32>(sizeof(FShaderResourceBinding));

    for (const FShaderResourceBinding& Binding : View.Bindings)
    {
        const bool bValidType      = static_cast<uint8>(Binding.Type) < static_cast<uint8>(EShaderResourceType::Count);
        const bool bValidDimension = static_cast<uint8>(Binding.Dimension) < static_cast<uint8>(EShaderResourceDimension::Count);
        const bool bValidSpace     = static_cast<uint8>(Binding.Space) < static_cast<uint8>(EShaderBindingSpace::Count);

        if (!bValidType || !bValidDimension || !bValidSpace || Binding.Count < 1)
        {
            return SetError(OutError, "A binding has an invalid type, dimension, space or count");
        }
    }

    // SPIR-V patch offsets
    if (View.Header.OutputLanguage == EShaderOutputLanguage::SPIRV && NumBindings > 0)
    {
        Offset = AlignOffset(Offset, alignof(FSpirvBindingOffsets));
        if (Offset + NumBindings * sizeof(FSpirvBindingOffsets) > ReflectionEnd)
        {
            return SetError(OutError, "The SPIR-V offsets run past the reflection block");
        }

        View.SpirvOffsets = TArrayView<const FSpirvBindingOffsets>(reinterpret_cast<const FSpirvBindingOffsets*>(Bytes + Offset), static_cast<int32>(NumBindings));
        Offset += NumBindings * static_cast<uint32>(sizeof(FSpirvBindingOffsets));
    }

    // MSL info and slots
    if (View.Header.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        Offset = AlignOffset(Offset, alignof(FMSLShaderInfo));
        if (Offset + sizeof(FMSLShaderInfo) + NumBindings * sizeof(FMSLBindingSlot) > ReflectionEnd)
        {
            return SetError(OutError, "The MSL info runs past the reflection block");
        }

        Memory::Memcpy(&View.MSLInfo, Bytes + Offset, sizeof(FMSLShaderInfo));
        Offset += static_cast<uint32>(sizeof(FMSLShaderInfo));

        View.MSLSlots = TArrayView<const FMSLBindingSlot>(reinterpret_cast<const FMSLBindingSlot*>(Bytes + Offset), static_cast<int32>(NumBindings));
        Offset += NumBindings * static_cast<uint32>(sizeof(FMSLBindingSlot));
    }

    // Strings: the entry point, then one name per binding in DebugInfo containers
    auto ReadString = [&](const CHAR*& OutString) -> bool
    {
        const uint32 Start = Offset;
        while (Offset < ReflectionEnd && Bytes[Offset] != 0)
        {
            ++Offset;
        }

        if (Offset >= ReflectionEnd)
        {
            return false;
        }

        OutString = reinterpret_cast<const CHAR*>(Bytes + Start);
        ++Offset;
        return true;
    };

    if (!ReadString(View.EntryPoint))
    {
        return SetError(OutError, "The entry point is not null-terminated inside the reflection block");
    }

    if (View.HasDebugInfo())
    {
        View.BindingNames.Reserve(static_cast<int32>(NumBindings));
        for (uint32 Index = 0; Index < NumBindings; ++Index)
        {
            const CHAR* Name = nullptr;
            if (!ReadString(Name))
            {
                return SetError(OutError, "A binding name is missing or not null-terminated inside the reflection block");
            }

            View.BindingNames.Add(Name);
        }
    }

    // Only the zero padding before the native code may remain
    if (ReflectionEnd - Offset >= FShaderCodeWriter::CodeAlignment)
    {
        return SetError(OutError, "The reflection block has unexpected trailing data");
    }

    for (; Offset < ReflectionEnd; ++Offset)
    {
        if (Bytes[Offset] != 0)
        {
            return SetError(OutError, "The padding before the native code is not zero");
        }
    }

    View.NativeCode = TArrayView<const uint8>(Bytes + ReflectionEnd, static_cast<int32>(static_cast<uint32>(ShaderCode.Size()) - ReflectionEnd));

    OutView = ::Move(View);
    return true;
}
