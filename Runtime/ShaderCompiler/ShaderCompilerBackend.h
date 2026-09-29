#pragma once
#include "ShaderCompiler/ShaderCompiler.h"

struct FShaderCompileRequest
{
    const FShaderCompileInfo*       CompileInfo = nullptr;
    StringView                      Source;

    /** Empty when compiling from source */
    String                          FilePath;

    /** <Assets>/Shaders, where every shader include is resolved from */
    String                          IncludeDir;

    /** The frontend defines followed by FShaderCompileInfo::Defines */
    TArrayView<const FShaderDefine> Defines;

    bool                            bDebugInfo      = false;
    bool                            bVerboseLogging = false;
};

struct FShaderCompileResult
{
    TArray<uint8>  ByteCode;
    TArray<String> Dependencies;
    String         Messages;
};

class FShaderCompilerBackend
{
public:
    virtual ~FShaderCompilerBackend() = default;

    virtual bool Initialize() = 0;

    NODISCARD virtual const CHAR* GetName() const = 0;
    NODISCARD virtual bool SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const = 0;

    /** Adds the compiler identity (version) and every flag that changes the output */
    virtual void HashCompileSettings(const FShaderCompileInfo& CompileInfo, const String& IncludeDir, bool bDebugInfo, uint64& InOutHash) const = 0;

    /** Must be thread-safe, FShaderCache compiles permutations concurrently */
    virtual bool Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult) = 0;
};
