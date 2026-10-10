#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "ShaderCompiler/ShaderCompileJob.h"

struct FShaderJobFileEntry
{
    /** "D3D11", "D3D12", "Vulkan" or "Metal" */
    FString           RHIName;
    FShaderCompileJob Job;
};

class SHADERCOMPILER_API FShaderJobFile
{
public:
    static constexpr int32 Version = 1;

    /** @return <Assets>/ followed by Renderer.ShaderCache.JobFileName */
    NODISCARD static FString GetDefaultFilePath();

    /** @brief Appends the entries in the file. A missing file is not an error, a different Version is. */
    bool Load(const FString& FilePath, FString& OutError);
    bool Save(const FString& FilePath) const;

    /** @return False when the same job for the same RHI is already in the file */
    bool Add(const FString& RHIName, FShaderCompileJob&& Job);

    /** @return The entries for the listed RHIs, compared without case. An empty list returns every entry. */
    NODISCARD TArray<const FShaderJobFileEntry*> Filter(TArrayView<const FString> RHINames) const;

    NODISCARD const TArray<FShaderJobFileEntry>& GetEntries() const
    {
        return Entries;
    }

private:
    TArray<FShaderJobFileEntry> Entries;
    TSet<uint64>                Keys;
};
