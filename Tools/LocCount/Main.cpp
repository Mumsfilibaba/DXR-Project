#include <LaunchProgram/ProgramEntry.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/String.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Core/Misc/Parse.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Templates/CString.h>

#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();

struct FLocTotals
{
    int32 Files    = 0;
    int32 Total    = 0;
    int32 Blank    = 0;
    int32 NonBlank = 0;
};

struct FLocEntry
{
    String     Name;
    FLocTotals Totals;
};

static FLocTotals               GTotals;
static TMap<String, FLocTotals> GByExtension;
static TMap<String, FLocTotals> GByModule;
static TMap<String, String>     GModuleByDirectory;

static bool ShouldSkipDirectory(const String& Name)
{
    return (Name == "ThirdParty")
        || (Name == "Build")
        || (Name == "Solutions")
        || (Name == ".git")
        || (Name == "Generated");
}

static bool IsCountedExtension(const String& Extension)
{
    static const CHAR* CountedExtensions[] =
    {
        ".h",
        ".hpp",
        ".inl",
        ".c",
        ".cpp",
        ".hlsl",
        ".glsl",
        ".lua",
        ".bat",
    };

    for (const CHAR* Counted : CountedExtensions)
    {
        if (Extension.Compare(Counted, EStringCaseType::NoCase) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool IsModuleRoot(const String& Directory)
{
    return FPlatformFile::IsFile(*File::CombinePath(Directory, "Module.lua"))
        || FPlatformFile::IsFile(*File::CombinePath(Directory, "Target.lua"));
}

static String ResolveModuleName(const String& Directory, const String& ScanRoot, const String& ScanRootName)
{
    if (const String* Cached = GModuleByDirectory.Find(Directory))
    {
        return *Cached;
    }

    String Name;
    if (Directory.Length() <= ScanRoot.Length())
    {
        Name = ScanRootName;
    }
    else if (IsModuleRoot(Directory))
    {
        const int32 Offset = ScanRoot.Length() + 1;
        Name = File::CombinePath(ScanRootName, String(*Directory + Offset, Directory.Length() - Offset));
    }
    else
    {
        Name = ResolveModuleName(File::GetDirectoryOf(Directory), ScanRoot, ScanRootName);
    }

    GModuleByDirectory.Add(Directory, Name);
    return Name;
}

static void AddTo(FLocTotals& Bucket, const FLineStats& Stats)
{
    ++Bucket.Files;
    Bucket.Total    += Stats.Total;
    Bucket.Blank    += Stats.Blank;
    Bucket.NonBlank += Stats.NonBlank;
}

static void Accumulate(const FLineStats& Stats, const String& Extension, const String& Module)
{
    AddTo(GByExtension.FindOrAdd(Extension), Stats);
    AddTo(GByModule.FindOrAdd(Module), Stats);
    AddTo(GTotals, Stats);
}

static TArray<FLocEntry> SortedByTotal(TMap<String, FLocTotals>& Buckets)
{
    TArray<FLocEntry> Entries;
    Entries.Reserve(Buckets.Size());

    for (TMap<String, FLocTotals>::IteratorType It = Buckets.CreateIterator(); !It.IsEnd(); ++It)
    {
        Entries.Emplace(FLocEntry{ It.GetKey(), It.GetValue() });
    }

    Entries.SortWithPredicate([](const FLocEntry& First, const FLocEntry& Second) -> bool
    {
        return First.Totals.Total > Second.Totals.Total;
    });

    return Entries;
}

static void PrintRow(const CHAR* Name, const FLocTotals& Totals)
{
    const float Share = (GTotals.Total > 0) ? ((100.0f * Totals.Total) / GTotals.Total) : 0.0f;
    LOG_INFO("[LocCount]   %-40s %6d %9d %9d %9d %6.1f%%",
        Name,
        Totals.Files,
        Totals.Total,
        Totals.NonBlank,
        Totals.Blank,
        Share);
}

static void PrintReport()
{
    LOG_INFO("[LocCount]   %-40s %6s %9s %9s %9s %7s", "Module", "Files", "Total", "Code", "Blank", "Share");

    const TArray<FLocEntry> Modules = SortedByTotal(GByModule);
    for (const FLocEntry& Entry : Modules)
    {
        PrintRow(*Entry.Name, Entry.Totals);
    }

    LOG_INFO("[LocCount]");
    LOG_INFO("[LocCount]   %-40s %6s %9s %9s %9s %7s", "Extension", "Files", "Total", "Code", "Blank", "Share");

    const TArray<FLocEntry> Extensions = SortedByTotal(GByExtension);
    for (const FLocEntry& Entry : Extensions)
    {
        PrintRow(*Entry.Name, Entry.Totals);
    }

    LOG_INFO("[LocCount]");
    PrintRow("All modules", GTotals);
}

static int32 LocCountMain()
{
    String Root = Paths::GetEngineDir();

    StringView RootOption;
    if (CommandLine::FindOption("root", RootOption) && (RootOption.Size() > 0))
    {
        Root = String(RootOption.Data(), RootOption.Size());
    }

    LOG_INFO("[LocCount] Scanning '%s'", *Root);

    static const CHAR* Folders[] =
    {
        "Runtime",
        "Tools",
        "Tests",
        "Sandbox",
        "SetupScripts",
    };

    for (const CHAR* Folder : Folders)
    {
        if (FProgramLoop::IsExitRequested())
        {
            break;
        }

        const String FolderPath = File::CombinePath(Root, Folder);
        if (!FPlatformFile::IsDirectory(*FolderPath))
        {
            LOG_WARNING("[LocCount] Skipping missing folder '%s'", *FolderPath);
            continue;
        }

        File::IterateDirectoryTree(FolderPath, [&](const String& Path, bool bIsDirectory) -> bool
        {
            if (FProgramLoop::IsExitRequested())
            {
                return false;
            }

            if (bIsDirectory)
            {
                return !ShouldSkipDirectory(File::ExtractFilename(Path));
            }

            const String Extension = File::ExtractExtension(Path);
            if (!IsCountedExtension(Extension))
            {
                return true;
            }

            TFileRef<IPlatformFile> SourceFile = FPlatformFile::OpenForRead(Path);
            TArray<CHAR>            Text;
            if (!SourceFile.IsValid() || !File::ReadTextFile(SourceFile.Get(), Text))
            {
                LOG_WARNING("[LocCount] Failed to read '%s'", *Path);
                return true;
            }

            const String Module = ResolveModuleName(File::GetDirectoryOf(Path), FolderPath, Folder);
            Accumulate(Parse::CountLines(Text.Data()), Extension, Module);
            return true;
        });
    }

    PrintReport();
    return 0;
}

IMPLEMENT_PROGRAM_MAIN("LocCount", &LocCountMain);
