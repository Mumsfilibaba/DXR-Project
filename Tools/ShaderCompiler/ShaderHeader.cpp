#include "ShaderHeader.h"
#include "ShaderCompilerTool.h"
#include <Core/Filesystem/File.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerClient.h>
#include <ShaderCore/MSLShaderBindings.h>
#include <ShaderCore/ShaderCode.h>

static constexpr const CHAR* GStampPrefix  = "// Stamp:    0x";
static constexpr int32       GBytesPerLine = 12;

static String MakeEngineRelative(const String& Path)
{
    const String CollapsedPath = RemoteShaderCompilerProtocol::CollapsePath(Path);
    const String EnginePrefix  = RemoteShaderCompilerProtocol::CollapsePath(Paths::GetEngineDir()) + '/';

#if PLATFORM_WINDOWS
    const EStringCaseType CaseType = EStringCaseType::NoCase;
#else
    const EStringCaseType CaseType = EStringCaseType::CaseSensitive;
#endif

    if (CollapsedPath.StartsWith(EnginePrefix, CaseType))
    {
        return String(CollapsedPath.Data() + EnginePrefix.Length(), CollapsedPath.Length() - EnginePrefix.Length());
    }

    return CollapsedPath;
}

uint64 ShaderHeader::ComputeStamp(const FShaderCompileJob& Job, const TArray<FShaderSourceFile>& Sources)
{
    FShaderCompileJob StampJob = Job;
    StampJob.SourceFile = MakeEngineRelative(Job.SourceFile);

    for (String& IncludeDir : StampJob.IncludeDirs)
    {
        IncludeDir = MakeEngineRelative(IncludeDir);
    }

    uint64 Stamp = FShaderCompiler::Get().ComputeCompileHash(StampJob.SourceFile, StampJob.ToCompileInfo(), FShaderCompilerIdentity());
    for (const FShaderSourceFile& Source : Sources)
    {
        HashCombine(Stamp, Source.Path);
        HashCombine(Stamp, Source.Hash);
    }

    return Stamp;
}

static bool ParseHex(const CHAR* Text, uint64& OutValue)
{
    uint64 Value     = 0;
    int32  NumDigits = 0;

    for (const CHAR* Character = Text; *Character; ++Character)
    {
        uint64 Digit = 0;
        if (*Character >= '0' && *Character <= '9')
        {
            Digit = static_cast<uint64>(*Character - '0');
        }
        else if (*Character >= 'a' && *Character <= 'f')
        {
            Digit = static_cast<uint64>(*Character - 'a' + 10);
        }
        else if (*Character >= 'A' && *Character <= 'F')
        {
            Digit = static_cast<uint64>(*Character - 'A' + 10);
        }
        else
        {
            break;
        }

        Value = (Value << 4) | Digit;
        NumDigits++;
    }

    OutValue = Value;
    return NumDigits > 0 && NumDigits <= 16;
}

bool ShaderHeader::ReadStamp(const String& HeaderPath, uint64& OutStamp)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(HeaderPath);
    if (!FileHandle)
    {
        return false;
    }

    TArray<CHAR> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return false;
    }

    const String Contents(Text.Data());
    const int32  StampStart = Contents.Find(GStampPrefix);

    if (StampStart == String::InvalidIndex || StampStart > Contents.Find("inline constexpr"))
    {
        return false;
    }

    return ParseHex(Contents.Data() + StampStart + String(GStampPrefix).Length(), OutStamp);
}

static const CHAR* GetDimensionName(EShaderResourceDimension Dimension)
{
    switch (Dimension)
    {
        case EShaderResourceDimension::Buffer:           return "Buffer";
        case EShaderResourceDimension::BufferEx:         return "BufferEx";
        case EShaderResourceDimension::Texture1D:        return "Texture1D";
        case EShaderResourceDimension::Texture1DArray:   return "Texture1DArray";
        case EShaderResourceDimension::Texture2D:        return "Texture2D";
        case EShaderResourceDimension::Texture2DArray:   return "Texture2DArray";
        case EShaderResourceDimension::Texture2DMS:      return "Texture2DMS";
        case EShaderResourceDimension::Texture2DMSArray: return "Texture2DMSArray";
        case EShaderResourceDimension::Texture3D:        return "Texture3D";
        case EShaderResourceDimension::TextureCube:      return "TextureCube";
        case EShaderResourceDimension::TextureCubeArray: return "TextureCubeArray";
        default:                                         return "-";
    }
}

static const CHAR* GetSpaceName(EShaderBindingSpace Space)
{
    switch (Space)
    {
        case EShaderBindingSpace::Global:          return "global";
        case EShaderBindingSpace::RayTracingLocal: return "ray tracing local";
        case EShaderBindingSpace::BindlessHeap:    return "bindless heap";
        case EShaderBindingSpace::ShaderConstants: return "shader constants";
        default:                                   return "unknown";
    }
}

static String GetRegisterName(const FShaderResourceBinding& Binding)
{
    if (Binding.Space == EShaderBindingSpace::ShaderConstants)
    {
        return "constants";
    }

    const CHAR* Prefix = "t";
    switch (GetShaderResourceClass(Binding.Type))
    {
        case EShaderResourceClass::ConstantBuffer: Prefix = "b"; break;
        case EShaderResourceClass::Sampler:        Prefix = "s"; break;
        case EShaderResourceClass::UAV:            Prefix = "u"; break;
        default:                                   break;
    }

    return Binding.Count > 1 ? String::Printf("%s%u[%u]", Prefix, Binding.Register, Binding.Count) : String::Printf("%s%u", Prefix, Binding.Register);
}

static String GetNullTextureName(uint8 NullTextureType)
{
    static constexpr const CHAR* DimensionNames[] = { "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "TextureCube", "TextureCubeArray", "Texture3D", "Texture2DMS", "TextureBuffer" };
    static constexpr const CHAR* ComponentNames[] = { "Float", "Int", "Uint", "Depth" };
    static_assert(ARRAY_COUNT(DimensionNames) == static_cast<int32>(EMSLTextureDimension::Count) && ARRAY_COUNT(ComponentNames) == static_cast<int32>(EMSLTextureComponent::Count), "The null texture names are out of date");

    const uint32 Dimension = NullTextureType % static_cast<uint32>(EMSLTextureDimension::Count);
    const uint32 Component = NullTextureType / static_cast<uint32>(EMSLTextureDimension::Count);
    if (Component >= ARRAY_COUNT(ComponentNames))
    {
        return String::Printf("%u", NullTextureType);
    }

    return String::Printf("%s %s", DimensionNames[Dimension], ComponentNames[Component]);
}

static String BuildReflectionComment(const TArray<uint8>& ShaderCode)
{
    FShaderCodeView CodeView;
    if (!FShaderCodeReader::Read(ShaderCode, CodeView))
    {
        return String();
    }

    const FShaderReflectionInfo& Info = CodeView.GetInfo();
    const bool bIsMSL = CodeView.GetOutputLanguage() == EShaderOutputLanguage::MSL;

    String Comment = "// Reflection:\n";
    Comment += String::Printf("//   Shader constants: %u bytes\n", Info.ShaderConstantsSize);

    if (bIsMSL && CodeView.GetStage() == EShaderStage::Compute)
    {
        const FMSLShaderInfo& MSLInfo = CodeView.GetMSLInfo();
        Comment += String::Printf("//   Thread group:     %u x %u x %u\n", MSLInfo.ThreadGroupSize[0], MSLInfo.ThreadGroupSize[1], MSLInfo.ThreadGroupSize[2]);
    }

    if (Info.NumVertexInputs > 0)
    {
        Comment += String::Printf("//   Vertex inputs:    %u\n", Info.NumVertexInputs);
    }

    if (Info.RequiredFeatures != EShaderFeatureFlags::None)
    {
        Comment += String::Printf("//   Features:         0x%08x\n", static_cast<uint32>(Info.RequiredFeatures));
    }

    const TArrayView<const FShaderResourceBinding> Bindings = CodeView.GetBindings();
    const TArrayView<const FMSLBindingSlot>        MSLSlots = CodeView.GetMSLSlots();
    if (Bindings.IsEmpty())
    {
        Comment += "//   Bindings:         none\n";
        return Comment;
    }

    Comment += "//   Bindings:\n";
    for (int32 Index = 0; Index < Bindings.Size(); ++Index)
    {
        const FShaderResourceBinding& Binding = Bindings[Index];

        String Line = String::Printf("//     %-10s %-21s %-16s %s", *GetRegisterName(Binding), ToString(Binding.Type), GetDimensionName(Binding.Dimension), GetSpaceName(Binding.Space));
        if (bIsMSL && Index < MSLSlots.Size())
        {
            const EMSLBindingType BindingType = GetMSLBindingType(Binding);
            Line += String::Printf(", MSL %s slot %u", ToString(BindingType), MSLSlots[Index].Slot);

            if (MSLSlots[Index].NullTextureType != 0 || GetMSLBindingTable(BindingType) == EMSLBindingTable::Texture)
            {
                Line += String::Printf(", null texture %s", *GetNullTextureName(MSLSlots[Index].NullTextureType));
            }
        }

        Comment += Line + '\n';
    }

    return Comment;
}

String ShaderHeader::Build(const FShaderHeaderDesc& Desc, uint64 Stamp, const FShaderCompilerIdentity& Identity, const TArray<FShaderSourceFile>& Sources, const TArray<uint8>& ShaderCode)
{
    String Defines;
    for (const FShaderDefine& Define : Desc.Job.Defines)
    {
        if (!Defines.IsEmpty())
        {
            Defines += ' ';
        }

        Defines += Define.Define + '=' + Define.Value;
    }

    String Reads;
    for (const FShaderSourceFile& Source : Sources)
    {
        if (!Reads.IsEmpty())
        {
            Reads += ", ";
        }

        Reads += Source.LocalPath.IsEmpty() ? Source.Path : MakeEngineRelative(Source.LocalPath);
    }

    String Compiler = String::Printf("%s %u.%u, settings %u", *Identity.Name, Identity.VersionMajor, Identity.VersionMinor, Identity.SettingsVersion);
    if (Identity.TranslatorVersion != 0)
    {
        Compiler += String::Printf(", translator %u", Identity.TranslatorVersion);
    }

    String Header;
    Header += "// Generated by ShaderCompiler -compile -header. Do not edit, building the module regenerates it from the sources below.\n";
    Header += String::Printf("// Source:   %s (%s, %s, %s, %s)\n", *MakeEngineRelative(Desc.Job.SourceFile), *Desc.Job.EntryPoint, ToString(Desc.Job.ShaderStage), ToString(Desc.Job.ShaderModel), ToString(Desc.Job.OutputLanguage));
    Header += String::Printf("// Defines:  %s\n", Defines.IsEmpty() ? "none" : *Defines);
    Header += String::Printf("// Compiler: %s\n", *Compiler);
    Header += String::Printf("// Reads:    %s\n", *Reads);
    Header += BuildReflectionComment(ShaderCode);
    Header += String::Printf("%s%016llx\n", GStampPrefix, Stamp);
    Header += "#pragma once\n";
    Header += "#include \"Core/Core.h\"\n";
    Header += "\n";
    Header += "/** FShaderCode container, read with FShaderCodeReader::ReadEmbedded */\n";
    Header += String::Printf("inline constexpr uint8 %s[] =\n", *Desc.SymbolName);
    Header += "{\n";

    for (int32 Offset = 0; Offset < ShaderCode.Size(); Offset += GBytesPerLine)
    {
        Header += "   ";
        const int32 LineEnd = (Offset + GBytesPerLine < ShaderCode.Size()) ? (Offset + GBytesPerLine) : ShaderCode.Size();
        for (int32 Index = Offset; Index < LineEnd; ++Index)
        {
            Header += String::Printf(" 0x%02x,", static_cast<uint32>(ShaderCode[Index]));
        }

        Header += '\n';
    }

    Header += "};\n";
    return Header;
}

bool ShaderHeader::Write(const FShaderHeaderDesc& Desc, uint64 Stamp, const FShaderCompilerIdentity& Identity, const TArray<FShaderSourceFile>& Sources, const TArray<uint8>& ShaderCode, String& OutError)
{
    FShaderCodeHeader CodeHeader;
    if (!FShaderCodeReader::ReadHeader(ShaderCode, CodeHeader, &OutError))
    {
        return false;
    }

    const String Folder = File::GetDirectoryOf(Desc.HeaderPath);
    if (!File::CreateDirectoryTree(Folder))
    {
        OutError = String::Printf("Cannot create '%s'", *Folder);
        return false;
    }

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Desc.HeaderPath);
    if (!FileHandle || !File::WriteTextFile(FileHandle.Get(), Build(Desc, Stamp, Identity, Sources, ShaderCode)))
    {
        OutError = String::Printf("Cannot write '%s'", *Desc.HeaderPath);
        return false;
    }

    return true;
}

int32 ShaderHeader::RunCompile(const FToolOptions& Options)
{
    FShaderCompiler& Compiler = FShaderCompiler::Get();

    // ParseOptions only accepts -header with exactly one -rhi
    const FRHITarget* Target = FindRHITarget(Options.RHINames[0]);

    FShaderHeaderDesc Desc;
    Desc.HeaderPath = Options.HeaderPath;
    Desc.SymbolName = Options.SymbolName;

    Desc.Job.Name           = Options.SymbolName;
    Desc.Job.SourceFile     = Options.CompileFile;
    Desc.Job.EntryPoint     = Options.EntryPoint;
    Desc.Job.ShaderModel    = Options.ShaderModel;
    Desc.Job.ShaderStage    = Options.ShaderStage;
    Desc.Job.OutputLanguage = Target->OutputLanguage;
    Desc.Job.bOptimize      = true;
    Desc.Job.bDebugInfo     = false;
    Desc.Job.Defines        = Options.Defines;
    Desc.Job.IncludeDirs    = Options.IncludeDirs;

    const FShaderCompileInfo CompileInfo = Desc.Job.ToCompileInfo();
    const String             DisplayPath = MakeEngineRelative(Desc.HeaderPath);

    TArray<FShaderSourceFile> Sources;
    String                    Error;

    if (!Compiler.CollectSources(Desc.Job.SourceFile, CompileInfo, Sources, Error))
    {
        LOG_ERROR("[ShaderCompiler] %s: %s", *DisplayPath, *Error);
        return ToExitCode(EShaderCompilerExitCode::JobsFailed);
    }

    const uint64 Stamp = ComputeStamp(Desc.Job, Sources);

    uint64 ExistingStamp = 0;
    if (!Options.bForce && ReadStamp(Desc.HeaderPath, ExistingStamp) && ExistingStamp == Stamp)
    {
        LOG_INFO("[ShaderCompiler] '%s' is up to date", *DisplayPath);
        return ToExitCode(EShaderCompilerExitCode::Success);
    }

    if (Options.bCheck)
    {
        LOG_ERROR("[ShaderCompiler] '%s' is stale", *DisplayPath);
        return ToExitCode(EShaderCompilerExitCode::HeaderStale);
    }

    TArray<TUniquePtr<FRemoteShaderCompilerClient>> Clients;
    bool bRemoteUnreachable = false;
    const TArray<FRouteEntry> Routes = ShaderCompilerTool::BuildRoutes(Options, Clients, bRemoteUnreachable);

    const FRouteEntry* Route = ShaderCompilerTool::FindRoute(Routes, Target->Name);
    if (!Route || Route->Route == EJobRoute::Skipped)
    {
        LOG_ERROR("[ShaderCompiler] '%s' needs %s, which this machine cannot compile. Pass -remote=%s@<host> or -crosscompile.", *DisplayPath, Target->Name, Target->Name);
        return ToExitCode(bRemoteUnreachable ? EShaderCompilerExitCode::RemoteUnreachable : EShaderCompilerExitCode::JobsFailed);
    }

    TArray<uint8> ShaderCode;
    bool bCompiled = false;
    if (Route->Route == EJobRoute::Local)
    {
        bCompiled = Compiler.CompileFromFile(Desc.Job.SourceFile, CompileInfo, ShaderCode);
    }
    else
    {
        bCompiled = Compiler.CompileOnRemote(*Route->Client, Desc.Job.SourceFile, CompileInfo, ShaderCode, nullptr) == ERemoteCompileStatus::Succeeded;
    }

    for (TUniquePtr<FRemoteShaderCompilerClient>& Client : Clients)
    {
        Client->Disconnect();
    }

    if (!bCompiled || !Write(Desc, Stamp, Route->Identity, Sources, ShaderCode, Error))
    {
        LOG_ERROR("[ShaderCompiler] '%s' FAILED %s", *DisplayPath, *Error);
        return ToExitCode(EShaderCompilerExitCode::JobsFailed);
    }

    LOG_INFO("[ShaderCompiler] Wrote '%s'", *DisplayPath);
    return ToExitCode(EShaderCompilerExitCode::Success);
}
