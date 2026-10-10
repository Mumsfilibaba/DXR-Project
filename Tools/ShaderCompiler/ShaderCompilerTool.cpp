#include "ShaderCompilerTool.h"
#include <LaunchProgram/ProgramEntry.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Core/Network/SocketAddress.h>
#include <Core/Platform/PlatformFile.h>
#include <ShaderCompiler/ShaderCompileJob.h>
#include <ShaderCompiler/ShaderCompiler.h>

static constexpr uint32 GRemoteConnectMilliseconds = 5000;

TArrayView<const FRHITarget> GetRHITargets()
{
    static const TArray<FRHITarget> Targets = []()
    {
        TArray<FRHITarget> Result;
        for (ERHIType Type : GRHITypes)
        {
            Result.Add({ Type, ToString(Type), RHI::GetShaderOutputLanguage(Type) });
        }

        return Result;
    }();

    return TArrayView<const FRHITarget>(Targets);
}

const FRHITarget* FindRHITarget(const String& Name)
{
    for (const FRHITarget& Target : GetRHITargets())
    {
        if (Name.Equals(Target.Name, EStringCaseType::NoCase))
        {
            return &Target;
        }
    }

    return nullptr;
}

static bool ParsePort(const String& Text, uint16& OutPort)
{
    int64 Value = 0;
    if (Text.IsEmpty() || !TTypeFromString<int64>::FromString(Text, Value) || (Value < 1) || (Value > 65535))
    {
        return false;
    }

    OutPort = static_cast<uint16>(Value);
    return true;
}

static bool ParseHostAndPort(const String& Text, String& OutHost, uint16& OutPort)
{
    if (Text.IsEmpty())
    {
        return false;
    }

    if (Text[0] == '[')
    {
        const int32 Close = Text.FindChar(']');
        if (Close == String::InvalidIndex || Close == 1)
        {
            return false;
        }

        OutHost = Text.SubString(1, Close - 1);
        if (Close + 1 == Text.Length())
        {
            return true;
        }

        return (Text[Close + 1] == ':') && ParsePort(Text.SubString(Close + 2, Text.Length() - Close - 2), OutPort);
    }

    const int32 FirstColon = Text.FindChar(':');
    const int32 LastColon  = Text.FindLastChar(':');
    if (FirstColon == String::InvalidIndex || FirstColon != LastColon)
    {
        OutHost = Text;
        return true;
    }

    OutHost = Text.SubString(0, FirstColon);
    return !OutHost.IsEmpty() && ParsePort(Text.SubString(FirstColon + 1, Text.Length() - FirstColon - 1), OutPort);
}

String ShaderCompilerTool::MakeAbsolutePath(const String& Path)
{
    if (!FPlatformFile::IsPathRelative(*Path))
    {
        return RemoteShaderCompilerProtocol::CollapsePath(Path);
    }

    const String WorkingDirectory(FPlatformFile::GetCurrentWorkingDirectory().Data());
    return RemoteShaderCompilerProtocol::CollapsePath(File::CombinePath(WorkingDirectory, Path));
}

static void SplitList(const String& Text, TArray<String>& OutItems)
{
    int32 Start = 0;
    while (Start <= Text.Length())
    {
        int32 End = Start;
        while ((End < Text.Length()) && (Text[End] != ','))
        {
            ++End;
        }

        String Item(Text.Data() + Start, End - Start);
        Item.TrimInline();
        if (!Item.IsEmpty())
        {
            OutItems.Add(::Move(Item));
        }

        Start = End + 1;
    }
}

bool ShaderCompilerTool::ParseOptions(FToolOptions& OutOptions, String& OutError)
{
    const TArray<String>& Arguments = GProgramArgs;
    for (int32 ArgumentIndex = 0; ArgumentIndex < Arguments.Size(); ++ArgumentIndex)
    {
        const String& Argument = Arguments[ArgumentIndex];

        int32 NameStart = 0;
        while ((NameStart < Argument.Length()) && (Argument[NameStart] == '-'))
        {
            ++NameStart;
        }

        if (NameStart == 0)
        {
            OutError = String::Printf("Unexpected argument '%s'", *Argument);
            return false;
        }

        int32 NameEnd = NameStart;
        while ((NameEnd < Argument.Length()) && (Argument[NameEnd] != '='))
        {
            ++NameEnd;
        }

        const String Name(Argument.Data() + NameStart, NameEnd - NameStart);
        const bool   bHasValue = NameEnd < Argument.Length();
        const String Value     = bHasValue ? String(Argument.Data() + NameEnd + 1, Argument.Length() - NameEnd - 1) : String();

        if (Name.Equals("help", EStringCaseType::NoCase) || Name.Equals("?"))
        {
            OutOptions.bShowHelp = true;
        }
        else if (Name.Equals("server", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bServer = true;
        }
        else if (Name.Equals("check", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bCheck = true;
        }
        else if (Name.Equals("force", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bForce = true;
        }
        else if (Name.Equals("allowremote", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bAllowRemote = true;
        }
        else if (Name.Equals("crosscompile", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bCrossCompile = true;
        }
        else if (Name.Equals("compile", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.CompileFile = MakeAbsolutePath(Value);
        }
        else if (Name.Equals("header", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.HeaderPath = MakeAbsolutePath(Value);
        }
        else if (Name.Equals("symbol", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.SymbolName = Value;
        }
        else if (Name.Equals("entry", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.EntryPoint = Value;
        }
        else if (Name.Equals("stage", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            if (!TryParseShaderStage(Value, OutOptions.ShaderStage))
            {
                OutError = String::Printf("'%s' is not a shader stage, expected Vertex, Pixel, Compute, ...", *Value);
                return false;
            }
        }
        else if (Name.Equals("model", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            if (!TryParseShaderModel(Value, OutOptions.ShaderModel))
            {
                OutError = String::Printf("'%s' is not a shader model, expected SM_5_0 or SM_6_0 to SM_6_10", *Value);
                return false;
            }
        }
        else if (Name.Equals("define", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            const int32 Equals = Value.FindChar('=');
            if (Equals == 0)
            {
                OutError = String::Printf("'-define=%s' has to be NAME or NAME=VALUE", *Value);
                return false;
            }

            if (Equals == String::InvalidIndex)
            {
                OutOptions.Defines.Emplace(Value, String("1"));
            }
            else
            {
                OutOptions.Defines.Emplace(Value.SubString(0, Equals), Value.SubString(Equals + 1, Value.Length() - Equals - 1));
            }
        }
        else if (Name.Equals("includedir", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.IncludeDirs.Add(MakeAbsolutePath(Value));
        }
        else if (Name.Equals("jobs", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.JobFilePath = MakeAbsolutePath(Value);
        }
        else if (Name.Equals("output", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.OutputPath = MakeAbsolutePath(Value);
        }
        else if (Name.Equals("bind", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.BindAddress = Value;
        }
        else if (Name.Equals("port", EStringCaseType::NoCase))
        {
            if (!ParsePort(Value, OutOptions.Port))
            {
                OutError = String::Printf("'%s' is not a port", *Value);
                return false;
            }
        }
        else if (Name.Equals("rhi", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            TArray<String> RHINames;
            SplitList(Value, RHINames);

            for (const String& RHIName : RHINames)
            {
                const FRHITarget* Target = FindRHITarget(RHIName);
                if (!Target)
                {
                    OutError = String::Printf("Unknown RHI '%s', expected D3D11, D3D12, Vulkan or Metal", *RHIName);
                    return false;
                }

                OutOptions.RHINames.Add(Target->Name);
            }
        }
        else if (Name.Equals("remote", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            TArray<String> Entries;
            SplitList(Value, Entries);

            for (const String& Entry : Entries)
            {
                const int32 At = Entry.FindChar('@');
                if (At == String::InvalidIndex)
                {
                    OutError = String::Printf("'-remote=%s' has to be RHI@host[:port]", *Entry);
                    return false;
                }

                FRemoteTarget RemoteTarget;
                RemoteTarget.Target = FindRHITarget(Entry.SubString(0, At));
                if (!RemoteTarget.Target)
                {
                    OutError = String::Printf("Unknown RHI in '-remote=%s', expected D3D11, D3D12, Vulkan or Metal", *Entry);
                    return false;
                }

                if (!ParseHostAndPort(Entry.SubString(At + 1, Entry.Length() - At - 1), RemoteTarget.Host, RemoteTarget.Port))
                {
                    OutError = String::Printf("'-remote=%s' has to be RHI@host[:port], with IPv6 literals in brackets", *Entry);
                    return false;
                }

                OutOptions.RemoteTargets.Add(::Move(RemoteTarget));
            }
        }
        else if (Name.Contains('.'))
        {
            // A console variable, which FConsoleManager::LoadFromCommandLine applies
        }
        else
        {
            OutError = String::Printf("Unknown or malformed option '%s'", *Argument);
            return false;
        }
    }

    const bool bCompile = !OutOptions.CompileFile.IsEmpty();
    const bool bHeader  = !OutOptions.HeaderPath.IsEmpty();
    if (OutOptions.bServer && bCompile)
    {
        OutError = "-server and -compile cannot be combined";
        return false;
    }

    if (bCompile && (OutOptions.EntryPoint.IsEmpty() || OutOptions.ShaderStage == EShaderStage::Unknown || OutOptions.ShaderModel == EShaderModel::Unknown))
    {
        OutError = "-compile needs -entry, -stage and -model";
        return false;
    }

    if (!bCompile && (!OutOptions.EntryPoint.IsEmpty() || OutOptions.ShaderStage != EShaderStage::Unknown || OutOptions.ShaderModel != EShaderModel::Unknown ||
        !OutOptions.Defines.IsEmpty() || !OutOptions.IncludeDirs.IsEmpty() || bHeader || !OutOptions.SymbolName.IsEmpty()))
    {
        OutError = "-entry, -stage, -model, -define, -includedir, -header and -symbol only work with -compile";
        return false;
    }

    if (bHeader != !OutOptions.SymbolName.IsEmpty())
    {
        OutError = "-header and -symbol have to be given together";
        return false;
    }

    // One header holds one container, so the language has to be known
    if (bHeader && OutOptions.RHINames.Size() != 1)
    {
        OutError = "-header needs exactly one -rhi";
        return false;
    }

    if (OutOptions.bCheck && !bHeader)
    {
        OutError = "-check only works with -header";
        return false;
    }

    if (bCompile && !bHeader && OutOptions.OutputPath.IsEmpty())
    {
        OutOptions.OutputPath = MakeAbsolutePath(File::ExtractFilenameWithoutExtension(OutOptions.CompileFile) + ".shadercode");
    }

    return true;
}

void ShaderCompilerTool::PrintUsage()
{
    LOG_INFO(
        "Usage: ShaderCompiler [options]\n"
        "\n"
        "Without -server or -compile, compiles every job in the job file the editor writes (Assets/Shaders.shaderjob)\n"
        "into the shader bytecode cache, on this machine for every RHI it can compile and on -remote hosts for the rest.\n"
        "\n"
        "Modes:\n"
        "  (none)                    Batch: compile the job file\n"
        "  -server                   Serve compile requests from engines and other ShaderCompiler instances\n"
        "  -compile=<file>           Compile one shader file into an FShaderCode container per RHI\n"
        "\n"
        "Options:\n"
        "  -rhi=D3D11,D3D12,...      RHIs to compile, from D3D11, D3D12, Vulkan and Metal (default all)\n"
        "  -remote=RHI@host[:port]   Where to compile an RHI this machine cannot compile, may be repeated (port %u)\n"
        "  -includedir=<dir>         With -compile, an include directory searched after the including file's folder and\n"
        "                            before Assets/Shaders, which is always searched. May be repeated.\n"
        "  -entry=<name>             With -compile, the entry point\n"
        "  -stage=<stage>            With -compile, Vertex, Pixel, Compute, ...\n"
        "  -model=<model>            With -compile, SM_5_0 or SM_6_0 to SM_6_10\n"
        "  -define=NAME[=VALUE]      With -compile, a define, may be repeated (the value defaults to 1)\n"
        "  -header=<file>            With -compile and one -rhi, write a C++ header that embeds the container, unless the\n"
        "                            header's stamp shows its sources have not changed. The RHIs build theirs this way.\n"
        "  -symbol=<name>            With -header, the name of the array the header declares\n"
        "  -jobs=<file>              Job file to compile (default Assets/Shaders.shaderjob)\n"
        "  -output=<file>            Bytecode cache to write (default Assets/Shaders.shaderbytecode). With -compile the\n"
        "                            container (default <shader>.shadercode in the working directory), with _<RHI>\n"
        "                            added before the extension when several RHIs are compiled.\n"
        "  -force                    Recompile jobs that are cached, rewrite a -header that is up to date\n"
        "  -check                    With -header, only report whether the header is stale (exit code 5)\n"
        "  -crosscompile             Compile a language this platform has no RHI for (MSL on Windows) locally when\n"
        "                            no -remote is given for it, instead of skipping it\n"
        "  -port=<port>              Port the server listens on (default %u)\n"
        "  -allowremote              Let other machines connect to the server. There is no authentication.\n"
        "  -bind=<address>           Address the server binds with -allowremote (default ::, every interface)\n"
        "  -<Console.Variable>=<v>   Sets a console variable, the same way as for the engine\n"
        "\n"
        "Examples:\n"
        "  ShaderCompiler -remote=Metal@192.168.1.20\n"
        "  ShaderCompiler -server -allowremote\n"
        "  ShaderCompiler -compile=MyShader.hlsl -entry=Main -stage=Compute -model=SM_6_2 -rhi=Vulkan -includedir=Include\n"
        "  ShaderCompiler -compile=Clear.hlsl -entry=Main -stage=Compute -model=SM_6_2 -rhi=Metal -crosscompile\n"
        "                 -define=CLEAR_ELEMENT_UINT=1 -header=Generated/Clear_Uint.h -symbol=GMetalClear_Uint",
        RemoteShaderCompilerProtocol::DefaultPort, RemoteShaderCompilerProtocol::DefaultPort);
}

bool ShaderCompilerTool::IsRHISelected(const FToolOptions& Options, const FRHITarget& Target)
{
    if (Options.RHINames.IsEmpty())
    {
        return true;
    }

    for (const String& RHIName : Options.RHINames)
    {
        if (RHIName.Equals(Target.Name, EStringCaseType::NoCase))
        {
            return true;
        }
    }

    return false;
}

static FRemoteShaderCompilerClient* FindOrConnectClient(const FRemoteTarget& RemoteTarget, TArray<TUniquePtr<FRemoteShaderCompilerClient>>& Clients, TArray<String>& ClientAddresses, String& OutError)
{
    const String Address = FSocketAddress::FormatHostAndPort(RemoteTarget.Host, RemoteTarget.Port);
    for (int32 Index = 0; Index < ClientAddresses.Size(); ++Index)
    {
        if (ClientAddresses[Index] == Address)
        {
            if (!Clients[Index]->IsConnected())
            {
                OutError = Clients[Index]->GetLastError();
                return nullptr;
            }

            return Clients[Index].Get();
        }
    }

    TUniquePtr<FRemoteShaderCompilerClient> Client = MakeUniquePtr<FRemoteShaderCompilerClient>();
    const bool bConnected = Client->Connect(RemoteTarget.Host, RemoteTarget.Port, GRemoteConnectMilliseconds);
    OutError = Client->GetLastError();

    FRemoteShaderCompilerClient* Result = bConnected ? Client.Get() : nullptr;
    Clients.Add(::Move(Client));
    ClientAddresses.Add(Address);
    return Result;
}

TArray<FRouteEntry> ShaderCompilerTool::BuildRoutes(const FToolOptions& Options, TArray<TUniquePtr<FRemoteShaderCompilerClient>>& OutClients, bool& bOutRemoteUnreachable)
{
    FShaderCompiler& Compiler = FShaderCompiler::Get();

    TArray<String>      ClientAddresses;
    TArray<FRouteEntry> Routes;
    bOutRemoteUnreachable = false;

    for (const FRHITarget& Target : GetRHITargets())
    {
        if (!IsRHISelected(Options, Target))
        {
            continue;
        }

        const FRemoteTarget* RemoteTarget = nullptr;
        for (const FRemoteTarget& Candidate : Options.RemoteTargets)
        {
            if (Candidate.Target == &Target)
            {
                RemoteTarget = &Candidate;
            }
        }

        FRouteEntry Route;
        Route.Target = &Target;

        if (Compiler.CanCompileLocally(Target.OutputLanguage))
        {
            Route.Route    = EJobRoute::Local;
            Route.Identity = Compiler.GetLocalIdentity(Target.OutputLanguage);

            if (RemoteTarget)
            {
                LOG_INFO("[ShaderCompiler] %s compiles on this machine, -remote=%s@%s is not used", Target.Name, Target.Name, *RemoteTarget->Host);
            }
        }
        else if (RemoteTarget)
        {
            String Error;
            FRemoteShaderCompilerClient* Client = FindOrConnectClient(*RemoteTarget, OutClients, ClientAddresses, Error);
            if (!Client)
            {
                Route.Reason          = Error;
                bOutRemoteUnreachable = true;
            }
            else if (TOptional<FShaderCompilerIdentity> Identity = Client->GetIdentity(Target.OutputLanguage))
            {
                Route.Route    = EJobRoute::Remote;
                Route.Client   = Client;
                Route.Identity = *Identity;
            }
            else
            {
                Route.Reason          = String::Printf("%s does not serve %s", *Client->GetPeerName(), ToString(Target.OutputLanguage));
                bOutRemoteUnreachable = true;
            }
        }
        else if (Options.bCrossCompile && Compiler.IsOutputLanguageSupported(Target.OutputLanguage))
        {
            Route.Route    = EJobRoute::Local;
            Route.Identity = Compiler.GetLocalIdentity(Target.OutputLanguage);
            LOG_WARNING("[ShaderCompiler] Cross-compiling %s on this %s machine, which has no %s RHI to test the result with", ToString(Target.OutputLanguage), RemoteShaderCompilerProtocol::GetPlatformName(), Target.Name);
        }
        else
        {
            Route.Reason = String::Printf("this %s machine cannot compile %s, pass -remote=%s@<host>", RemoteShaderCompilerProtocol::GetPlatformName(), ToString(Target.OutputLanguage), Target.Name);
        }

        Routes.Add(::Move(Route));
    }

    LOG_INFO("[ShaderCompiler] Routing:");
    for (const FRouteEntry& Route : Routes)
    {
        if (Route.Route == EJobRoute::Local)
        {
            LOG_INFO("    %-7s %-6s local, %s %u.%u", Route.Target->Name, ToString(Route.Target->OutputLanguage), *Route.Identity.Name, Route.Identity.VersionMajor, Route.Identity.VersionMinor);
        }
        else if (Route.Route == EJobRoute::Remote)
        {
            LOG_INFO("    %-7s %-6s %s, %s %u.%u", Route.Target->Name, ToString(Route.Target->OutputLanguage), *Route.Client->GetPeerName(), *Route.Identity.Name, Route.Identity.VersionMajor, Route.Identity.VersionMinor);
        }
        else
        {
            LOG_WARNING("    %-7s %-6s skipped, %s", Route.Target->Name, ToString(Route.Target->OutputLanguage), *Route.Reason);
        }
    }

    return Routes;
}

const FRouteEntry* ShaderCompilerTool::FindRoute(const TArray<FRouteEntry>& Routes, const String& RHIName)
{
    for (const FRouteEntry& Route : Routes)
    {
        if (RHIName.Equals(Route.Target->Name, EStringCaseType::NoCase))
        {
            return &Route;
        }
    }

    return nullptr;
}
