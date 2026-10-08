#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "ShaderCore/ShaderReflection.h"

enum class EShaderCodeFlags : uint8
{
    None = 0,

    /** The native code has debug information and its embedded reflection, and the container has binding names */
    DebugInfo = FLAG(0),
};

ENUM_CLASS_OPERATORS(EShaderCodeFlags);

struct FShaderCodeHeader
{
    static constexpr uint32 MagicValue     = 0x43535844; // 'DXSC'
    static constexpr uint8  CurrentVersion = 1;

    uint32                Magic          = MagicValue;
    uint8                 Version        = CurrentVersion;
    EShaderOutputLanguage OutputLanguage = EShaderOutputLanguage::Unknown;
    EShaderStage          Stage          = EShaderStage::Unknown;
    EShaderCodeFlags      Flags          = EShaderCodeFlags::None;

    /** The native code runs from here to the end of the container */
    uint32                CodeOffset     = 0;
};

static_assert(sizeof(FShaderCodeHeader) == 12, "FShaderCodeHeader is serialized as-is, bump CurrentVersion when it changes");

class SHADERCORE_API FShaderCodeView
{
public:
    FShaderCodeView() = default;

    NODISCARD const FShaderCodeHeader& GetHeader() const
    {
        return Header;
    }

    NODISCARD EShaderStage GetStage() const
    {
        return Header.Stage;
    }

    NODISCARD EShaderOutputLanguage GetOutputLanguage() const
    {
        return Header.OutputLanguage;
    }

    NODISCARD bool HasDebugInfo() const
    {
        return IsEnumFlagSet(Header.Flags, EShaderCodeFlags::DebugInfo);
    }

    NODISCARD const FShaderReflectionInfo& GetInfo() const
    {
        return Info;
    }

    /** Empty unless the stage is Vertex */
    NODISCARD TArrayView<const FShaderVertexInput> GetVertexInputs() const
    {
        return VertexInputs;
    }

    NODISCARD TArrayView<const FShaderResourceBinding> GetBindings() const
    {
        return Bindings;
    }

    /** Empty unless the output language is SPIR-V, otherwise index-aligned with GetBindings */
    NODISCARD TArrayView<const FSpirvBindingOffsets> GetSpirvOffsets() const
    {
        return SpirvOffsets;
    }

    /** Zeros unless the output language is MSL */
    NODISCARD const FMSLShaderInfo& GetMSLInfo() const
    {
        return MSLInfo;
    }

    /** Empty unless the output language is MSL, otherwise index-aligned with GetBindings */
    NODISCARD TArrayView<const FMSLBindingSlot> GetMSLSlots() const
    {
        return MSLSlots;
    }

    /** Null-terminated, empty when the RHI does not need an entry point */
    NODISCARD const CHAR* GetEntryPoint() const
    {
        return EntryPoint;
    }

    /** Null-terminated, empty when the container was compiled without debug info */
    NODISCARD const CHAR* GetBindingName(int32 BindingIndex) const
    {
        return BindingNames.IsEmpty() ? "" : BindingNames[BindingIndex];
    }

    NODISCARD TArrayView<const uint8> GetNativeCode() const
    {
        return NativeCode;
    }

private:
    friend struct FShaderCodeReader;

    FShaderCodeHeader                        Header;
    FShaderReflectionInfo                    Info;
    TArrayView<const FShaderVertexInput>     VertexInputs;
    TArrayView<const FShaderResourceBinding> Bindings;
    TArrayView<const FSpirvBindingOffsets>   SpirvOffsets;
    FMSLShaderInfo                           MSLInfo;
    TArrayView<const FMSLBindingSlot>        MSLSlots;
    const CHAR*                              EntryPoint = "";

    /** Only allocated for DebugInfo containers */
    TArray<const CHAR*>                      BindingNames;

    TArrayView<const uint8>                  NativeCode;
};

struct SHADERCORE_API FShaderCodeWriter
{
    /** SPIR-V is read in place as uint32 words */
    static constexpr uint32 CodeAlignment = 4;

    /** @brief Fails when the reflection breaks the limits of the packed types or its arrays do not match Bindings */
    static bool Write(EShaderOutputLanguage OutputLanguage, EShaderStage Stage, EShaderCodeFlags Flags, const FShaderReflection& Reflection, TArrayView<const uint8> NativeCode, TArray<uint8>& OutShaderCode, String* OutError = nullptr);
};

struct SHADERCORE_API FShaderCodeReader
{
    /** @brief Rejects a wrong magic, other versions, and a CodeOffset that is unaligned or outside ShaderCode */
    NODISCARD static bool ReadHeader(const TArray<uint8>& ShaderCode, FShaderCodeHeader& OutHeader, String* OutError = nullptr);

    /** @brief ReadHeader, then checks every enum value, array range and string inside the reflection block */
    NODISCARD static bool Read(const TArray<uint8>& ShaderCode, FShaderCodeView& OutView, String* OutError = nullptr);
};
