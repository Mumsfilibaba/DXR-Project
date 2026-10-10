#pragma once
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <ShaderCompiler/ShaderCompileJob.h>
#include <ShaderCompiler/ShaderCompilerIdentity.h>

struct FToolOptions;

struct FShaderHeaderDesc
{
    /** Absolute */
    String            HeaderPath;

    /** The array the header declares */
    String            SymbolName;

    /** Always optimized and without debug info */
    FShaderCompileJob Job;
};

struct ShaderHeader
{
    /** @return The stamp stored in a generated header, computed from the job and every file it reads, relative to the engine directory */
    NODISCARD static uint64 ComputeStamp(const FShaderCompileJob& Job, const TArray<FShaderSourceFile>& Sources);

    /** @return False when the header does not exist or has no "// Stamp:" line */
    static bool ReadStamp(const String& HeaderPath, uint64& OutStamp);

    /** @return The whole header. The output does not change between runs with the same inputs, so regenerating leaves no diff. */
    NODISCARD static String Build(const FShaderHeaderDesc& Desc, uint64 Stamp, const FShaderCompilerIdentity& Identity, const TArray<FShaderSourceFile>& Sources, const TArray<uint8>& ShaderCode);

    static bool Write(const FShaderHeaderDesc& Desc, uint64 Stamp, const FShaderCompilerIdentity& Identity, const TArray<FShaderSourceFile>& Sources, const TArray<uint8>& ShaderCode, String& OutError);

    /** @brief -compile with -header: writes the header unless its stamp is current, or only reports a stale header with -check */
    static int32 RunCompile(const FToolOptions& Options);
};
