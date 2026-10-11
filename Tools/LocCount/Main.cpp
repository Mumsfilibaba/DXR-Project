#include <LaunchProgram/ProgramEntry.h>
#include <Core/Algorithms/Algorithm.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/String.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/OutputDeviceManager.h>
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
    FString    Name;
    FLocTotals Totals;
};

static FLocTotals                GTotals;
static TMap<FString, FLocTotals> GByExtension;
static TMap<FString, FLocTotals> GByModule;
static TMap<FString, FString>    GModuleByDirectory;

static bool ShouldSkipDirectory(const FString& Name)
{
    return (Name == TEXT("ThirdParty"))
        || (Name == TEXT("Build"))
        || (Name == TEXT("Solutions"))
        || (Name == TEXT(".git"))
        || (Name == TEXT("Generated"));
}

static bool IsCountedExtension(const FString& Extension)
{
    static const CHAR_T* CountedExtensions[] =
    {
        TEXT(".h"),
        TEXT(".hpp"),
        TEXT(".inl"),
        TEXT(".c"),
        TEXT(".cpp"),
        TEXT(".hlsl"),
        TEXT(".glsl"),
        TEXT(".lua"),
        TEXT(".bat"),
    };

    for (const CHAR_T* Counted : CountedExtensions)
    {
        if (Extension.Compare(Counted, EStringCaseType::NoCase) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool IsModuleRoot(const FString& Directory)
{
    return FPlatformFile::IsFile(*File::CombinePath(Directory, TEXT("Module.lua")))
        || FPlatformFile::IsFile(*File::CombinePath(Directory, TEXT("Target.lua")));
}

static FString ResolveModuleName(const FString& Directory, const FString& ScanRoot, const FString& ScanRootName)
{
    if (const FString* Cached = GModuleByDirectory.Find(Directory))
    {
        return *Cached;
    }

    FString Name;
    if (Directory.Length() <= ScanRoot.Length())
    {
        Name = ScanRootName;
    }
    else if (IsModuleRoot(Directory))
    {
        const int32 Offset = ScanRoot.Length() + 1;
        Name = File::CombinePath(ScanRootName, FString(*Directory + Offset, Directory.Length() - Offset));
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

static void Accumulate(const FLineStats& Stats, const FString& Extension, const FString& Module)
{
    AddTo(GByExtension.FindOrAdd(Extension), Stats);
    AddTo(GByModule.FindOrAdd(Module), Stats);
    AddTo(GTotals, Stats);
}

static TArray<FLocEntry> SortedByTotal(TMap<FString, FLocTotals>& Buckets)
{
    TArray<FLocEntry> Entries;
    Entries.Reserve(Buckets.Size());

    for (TMap<FString, FLocTotals>::IteratorType It = Buckets.CreateIterator(); !It.IsEnd(); ++It)
    {
        Entries.Emplace(FLocEntry{ It.GetKey(), It.GetValue() });
    }

    Algorithm::Sort(Entries, [](const FLocEntry& First, const FLocEntry& Second) -> bool
    {
        return First.Totals.Total > Second.Totals.Total;
    });

    return Entries;
}

static void PrintRow(const CHAR_T* Name, const FLocTotals& Totals)
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
    LOG_INFO("[LocCount]   %-40s %6s %9s %9s %9s %7s", TEXT("Module"), TEXT("Files"), TEXT("Total"), TEXT("Code"), TEXT("Blank"), TEXT("Share"));

    const TArray<FLocEntry> Modules = SortedByTotal(GByModule);
    for (const FLocEntry& Entry : Modules)
    {
        PrintRow(*Entry.Name, Entry.Totals);
    }

    LOG_INFO("[LocCount]");
    LOG_INFO("[LocCount]   %-40s %6s %9s %9s %9s %7s", TEXT("Extension"), TEXT("Files"), TEXT("Total"), TEXT("Code"), TEXT("Blank"), TEXT("Share"));

    const TArray<FLocEntry> Extensions = SortedByTotal(GByExtension);
    for (const FLocEntry& Entry : Extensions)
    {
        PrintRow(*Entry.Name, Entry.Totals);
    }

    LOG_INFO("[LocCount]");
    PrintRow(TEXT("All modules"), GTotals);
}

static int32 LocCountMain()
{
    FString Root = Paths::GetEngineDir();

    FStringView RootOption;
    if (CommandLine::FindOption(TEXT("root"), RootOption) && (RootOption.Size() > 0))
    {
        Root = FString(RootOption.Data(), RootOption.Size());
    }

    LOG_INFO("[LocCount] Scanning '%s'", *Root);

    static const CHAR_T* Folders[] =
    {
        TEXT("Runtime"),
        TEXT("Tools"),
        TEXT("Tests"),
        TEXT("Sandbox"),
        TEXT("SetupScripts"),
    };

    for (const CHAR_T* Folder : Folders)
    {
        if (FProgramLoop::IsExitRequested())
        {
            break;
        }

        const FString FolderPath = File::CombinePath(Root, Folder);
        if (!FPlatformFile::IsDirectory(*FolderPath))
        {
            LOG_WARNING("[LocCount] Skipping missing folder '%s'", *FolderPath);
            continue;
        }

        File::IterateDirectoryTree(FolderPath, [&](const FString& Path, bool bIsDirectory) -> bool
        {
            if (FProgramLoop::IsExitRequested())
            {
                return false;
            }

            if (bIsDirectory)
            {
                return !ShouldSkipDirectory(File::ExtractFilename(Path));
            }

            const FString Extension = File::ExtractExtension(Path);
            if (!IsCountedExtension(Extension))
            {
                return true;
            }

            TFileRef<IPlatformFile> SourceFile = FPlatformFile::OpenForRead(Path);
            TArray<CHAR_T>          Text;
            if (!SourceFile.IsValid() || !File::ReadTextFile(SourceFile.Get(), Text))
            {
                LOG_WARNING("[LocCount] Failed to read '%s'", *Path);
                return true;
            }

            const FString Module = ResolveModuleName(File::GetDirectoryOf(Path), FolderPath, Folder);
            Accumulate(Parse::CountLines(Text.Data()), Extension, Module);
            return true;
        });
    }

    PrintReport();
    return 0;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("LocCount"), &LocCountMain);
