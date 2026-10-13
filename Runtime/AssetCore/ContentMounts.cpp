#include "AssetCore/ContentMounts.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/Paths.h"

static bool IsValidRoot(const FString& Root)
{
    if (Root.Length() < 3 || Root[0] != '/' || Root[Root.Length() - 1] != '/')
    {
        return false;
    }

    for (int32 Index = 1; Index < Root.Length() - 1; ++Index)
    {
        const CHAR_T Character = Root[Index];
        const bool bIsLetter   = (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z');
        const bool bIsDigit    = Character >= '0' && Character <= '9';
    
        if (!bIsLetter && !bIsDigit && Character != '_' && Character != '-')
        {
            return false;
        }
    }

    return true;
}

static bool SplitVirtualPath(const FString& VirtualPath, FString& OutRoot, FString& OutRest)
{
    FString Path = VirtualPath;
    Path.ReplaceAll('\\', '/');

    if (Path.Length() < 2 || Path[0] != '/')
    {
        return false;
    }

    int32 RootEnd = 1;
    while (RootEnd < Path.Length() && Path[RootEnd] != '/')
    {
        ++RootEnd;
    }

    OutRoot = Path.SubString(0, RootEnd) + TEXT("/");
    OutRest = RootEnd < Path.Length() ? Path.SubString(RootEnd + 1, Path.Length() - RootEnd - 1) : FString();
    return true;
}

FContentMounts FContentMounts::CreateDefault()
{
    FContentMounts Mounts;
    Mounts.Mount(EngineRoot, Paths::GetEngineContentDir());
    Mounts.Mount(ProjectRoot, Paths::GetProjectContentDir());
    return Mounts;
}

bool FContentMounts::Mount(const FString& Root, const FString& Directory)
{
    if (!IsValidRoot(Root) || Directory.IsEmpty())
    {
        return false;
    }

    const FString AbsoluteDirectory = File::MakeAbsolute(Directory);
    for (FContentMount& Existing : Mounts)
    {
        if (Existing.Root.Equals(Root))
        {
            Existing.Directory = AbsoluteDirectory;
            return true;
        }
    }

    FContentMount& NewMount = Mounts.Emplace();
    NewMount.Root      = Root;
    NewMount.Directory = AbsoluteDirectory;
    return true;
}

bool FContentMounts::Unmount(const FString& Root)
{
    for (int32 Index = 0; Index < Mounts.Size(); ++Index)
    {
        if (Mounts[Index].Root.Equals(Root))
        {
            Mounts.RemoveAt(Index);
            return true;
        }
    }

    return false;
}

const FContentMount* FContentMounts::FindMount(const FString& VirtualPath) const
{
    FString Root;
    FString Rest;

    if (!SplitVirtualPath(VirtualPath, Root, Rest))
    {
        return nullptr;
    }

    for (const FContentMount& Mount : Mounts)
    {
        if (Mount.Root.Equals(Root))
        {
            return &Mount;
        }
    }

    return nullptr;
}

bool FContentMounts::ToDiskPath(const FString& VirtualPath, FString& OutDiskPath) const
{
    const FContentMount* Mount = FindMount(VirtualPath);

    FString Root;
    FString Rest;

    if (!Mount || !SplitVirtualPath(VirtualPath, Root, Rest))
    {
        return false;
    }

    while (!Rest.IsEmpty() && Rest[0] == '/')
    {
        Rest = Rest.SubString(1, Rest.Length() - 1);
    }

    const FString Relative = File::NormalizePath(Rest);
    if (File::IsAbsolutePath(Relative) || Relative.Equals(TEXT("..")) || Relative.StartsWith(TEXT("../")))
    {
        return false;
    }

    OutDiskPath = (Relative.IsEmpty() || Relative.Equals(TEXT("."))) ? Mount->Directory : File::CombinePath(Mount->Directory, Relative);
    return true;
}

bool FContentMounts::ToVirtualPath(const FString& DiskPath, FString& OutVirtualPath) const
{
    const FString AbsolutePath = File::MakeAbsolute(DiskPath);

    const FContentMount* Deepest = nullptr;
    for (const FContentMount& Mount : Mounts)
    {
        if (File::IsUnderDirectory(AbsolutePath, Mount.Directory) && (!Deepest || Mount.Directory.Length() > Deepest->Directory.Length()))
        {
            Deepest = &Mount;
        }
    }

    if (!Deepest)
    {
        return false;
    }

    const FString Relative = File::MakeRelative(AbsolutePath, Deepest->Directory);
    OutVirtualPath = Relative.Equals(TEXT(".")) ? Deepest->Root : Deepest->Root + Relative;
    return true;
}
