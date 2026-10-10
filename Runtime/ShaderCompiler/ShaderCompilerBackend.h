#pragma once
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/ShaderCompilerIdentity.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "ShaderCore/ShaderReflection.h"

struct FShaderCompileRequest
{
    const FShaderCompileInfo* CompileInfo = nullptr;

    /** Preprocessed by FShaderPreprocessor, so there are no includes or defines left to resolve */
    StringView                Source;

    /** Empty when compiling from source */
    String                    FilePath;

    /** <Assets>/Shaders, where every shader include was resolved from */
    String                    IncludeDir;

    bool                      bVerboseLogging = false;
};

struct FShaderCompileResult
{
    /** Native code: a DXIL or DXBC container, SPIR-V words or MSL source */
    TArray<uint8>     ByteCode;
    FShaderReflection Reflection;
    String            Messages;
};

class FShaderCompilerBackend
{
public:
    virtual ~FShaderCompilerBackend() = default;

    virtual bool Initialize() = 0;

    NODISCARD virtual const CHAR* GetName() const = 0;
    NODISCARD virtual bool SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const = 0;

    /** Must not depend on paths or anything else that differs between machines */
    NODISCARD virtual FShaderCompilerIdentity GetIdentity() const = 0;

    /** @brief Rewrites the preprocessed source into what the compiler accepts. Errors are added to InOutSource.Errors. */
    virtual bool TranslateSource(FShaderPreprocessorOutput& /* InOutSource */) const
    {
        return true;
    }

    /** Must be thread-safe, FShaderCache compiles permutations concurrently */
    virtual bool Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult) = 0;
};
