#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "ShaderCompiler/ShaderCompiler.h"

struct SHADERCOMPILER_API FShaderCompileJob
{
    NODISCARD static FShaderCompileJob FromCompileInfo(const String& InSourceFile, const FShaderCompileInfo& CompileInfo);

    /** The returned info views Defines and IncludeDirs, so the job has to outlive it */
    NODISCARD FShaderCompileInfo ToCompileInfo();

    /** @return A hash of every field except Name, used to drop duplicates */
    NODISCARD uint64 GetKey() const;

    NODISCARD FJsonValue ToJson() const;
    static bool FromJson(const FJsonValue& Value, FShaderCompileJob& OutJob, String& OutError);

    /** Only shown in logs, "FDeferredLightingCS#12" for typed shaders */
    String                Name;

    /** The same string FShaderCompiler::CompileFromFile takes. Jobs sent over the network use the names CollectSources gives. */
    String                SourceFile;
    String                EntryPoint;
    EShaderModel          ShaderModel    = EShaderModel::Unknown;
    EShaderStage          ShaderStage    = EShaderStage::Unknown;
    EShaderOutputLanguage OutputLanguage = EShaderOutputLanguage::Unknown;
    bool                  bOptimize      = true;
    bool                  bDebugInfo     = false;

    /** True when Defines already holds what BuildCompileDefines adds. Jobs sent over the network carry the requester's full set. */
    bool                  bHasEngineDefines = false;
    TArray<FShaderDefine> Defines;

    /** FShaderCompileInfo::IncludeDirs, named like SourceFile */
    TArray<String>        IncludeDirs;
};

struct FShaderSourceFile
{
    /** The name FShaderCompiler::CollectSources gives it, with '/' separators */
    String        Path;

    /** Where it was read on this machine, empty for files a server received */
    String        LocalPath;

    /** Line endings normalized with ShaderSourceHash::NormalizeLineEndings */
    TArray<uint8> Contents;
    uint32        Hash = 0;
};

NODISCARD SHADERCOMPILER_API bool TryParseShaderStage(const String& Text, EShaderStage& OutStage);
NODISCARD SHADERCOMPILER_API bool TryParseShaderModel(const String& Text, EShaderModel& OutModel);
NODISCARD SHADERCOMPILER_API bool TryParseShaderOutputLanguage(const String& Text, EShaderOutputLanguage& OutOutputLanguage);
