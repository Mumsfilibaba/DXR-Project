#pragma once
#include <Core/Containers/Array.h>
#include <Core/Containers/ArrayView.h>
#include <Core/Containers/String.h>
#include <Core/Containers/UniquePtr.h>
#include <RHI/RHI.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompiler/ShaderCompilerIdentity.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerClient.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h>

enum class EShaderCompilerExitCode : int32
{
    Success           = 0,
    JobsFailed        = 1,
    RemoteUnreachable = 2,
    InvalidUsage      = 3,
    ServerFailed      = 4,
    HeaderStale       = 5, // -compile -header -check found a header that needs regenerating
};

NODISCARD inline int32 ToExitCode(EShaderCompilerExitCode ExitCode)
{
    return static_cast<int32>(ExitCode);
}

struct FRHITarget
{
    ERHIType              Type;
    const CHAR*           Name;           // ToString(Type), the name -rhi and -remote take
    EShaderOutputLanguage OutputLanguage; // RHI::GetShaderOutputLanguage(Type)
};

inline constexpr ERHIType GRHITypes[] =
{
    ERHIType::D3D11,
    ERHIType::D3D12,
    ERHIType::Vulkan,
    ERHIType::Metal,
};

/** @return One target per entry of GRHITypes, in the same order */
NODISCARD TArrayView<const FRHITarget> GetRHITargets();

/** @return The target named Name, compared without case, or nullptr */
NODISCARD const FRHITarget* FindRHITarget(const String& Name);

struct FRemoteTarget
{
    const FRHITarget* Target = nullptr;
    String            Host;
    uint16            Port   = RemoteShaderCompilerProtocol::DefaultPort;
};

struct FToolOptions
{
    bool                  bShowHelp     = false;                                     // -help
    bool                  bServer       = false;                                     // -server
    bool                  bCheck        = false;                                     // -check with -header, reports a stale header without writing it
    String                CompileFile;                                               // -compile=<file>, compiles one shader file, absolute once parsed
    String                HeaderPath;                                                // -header=<file> with -compile, writes a C++ header embedding the container instead, absolute once parsed
    String                SymbolName;                                                // -symbol=<name> with -header, the array the header declares
    String                EntryPoint;                                                // -entry= with -compile
    EShaderStage          ShaderStage   = EShaderStage::Unknown;                     // -stage= with -compile
    EShaderModel          ShaderModel   = EShaderModel::Unknown;                     // -model= with -compile
    TArray<FShaderDefine> Defines;                                                   // -define=NAME[=VALUE] with -compile, repeatable, the value defaults to 1
    TArray<String>        IncludeDirs;                                               // -includedir=<dir>, repeatable, absolute once parsed. Used by -compile.
    String                JobFilePath;                                               // -jobs=, defaults to FShaderJobFile::GetDefaultFilePath()
    TArray<String>        RHINames;                                                  // -rhi=D3D11,Metal, defaults to every entry of GRHITypes
    TArray<FRemoteTarget> RemoteTargets;                                             // -remote=RHI@host[:port], repeatable; IPv6 literals in brackets, -remote=Metal@[fe80::1]:9371
    String                OutputPath;                                                // -output=, absolute once parsed. The bytecode cache in batch mode (default Assets/Shaders.shaderbytecode), the container with -compile (default <shader>.shadercode)
    bool                  bForce        = false;                                     // -force, recompiles jobs that are already cached and rewrites a -header that is up to date
    bool                  bCrossCompile = false;                                     // -crosscompile, compiles a language this platform has no RHI for locally when a backend can still produce it and no -remote is given
    String                BindAddress;                                               // -bind=, only with -allowremote
    uint16                Port          = RemoteShaderCompilerProtocol::DefaultPort; // -port=
    bool                  bAllowRemote  = false;                                     // -allowremote
};

enum class EJobRoute : uint8
{
    Local,
    Remote,
    Skipped,
};

struct FRouteEntry
{
    const FRHITarget*            Target   = nullptr;
    EJobRoute                    Route    = EJobRoute::Skipped;
    FRemoteShaderCompilerClient* Client   = nullptr;
    FShaderCompilerIdentity      Identity;
    String                       Reason; // Shown in the routing table when the RHI is skipped
};

struct ShaderCompilerTool
{
    static bool ParseOptions(FToolOptions& OutOptions, String& OutError);
    static void PrintUsage();

    /** @return True when the RHI is one the options select, every RHI when -rhi was not given */
    NODISCARD static bool IsRHISelected(const FToolOptions& Options, const FRHITarget& Target);

    /**
     * @brief Picks a route per selected RHI and prints the table. Local wins whenever this machine can produce the language,
     *        a -remote host is only used for the rest.
     * @param OutClients Owns one client per distinct remote host, which the routes point into
     * @param bOutRemoteUnreachable True when a -remote host that a route needed could not be reached
     */
    static TArray<FRouteEntry> BuildRoutes(const FToolOptions& Options, TArray<TUniquePtr<FRemoteShaderCompilerClient>>& OutClients, bool& bOutRemoteUnreachable);

    NODISCARD static const FRouteEntry* FindRoute(const TArray<FRouteEntry>& Routes, const String& RHIName);

    /** @return Path when it is absolute, otherwise Path below the working directory. Called while parsing, before the program changes directory. */
    NODISCARD static String MakeAbsolutePath(const String& Path);

    static int32 RunBatch(const FToolOptions& Options);
    static int32 RunServer(const FToolOptions& Options);
    static int32 RunCompile(const FToolOptions& Options);
};
