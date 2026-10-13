#include "ContentMountsTests.h"

#include <AssetCore/ContentMounts.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

bool ContentMounts_Test()
{
    TEST_BEGIN();

    const FString Base    = Paths::GetUserSettingsDir() + TEXT("/Tests/ContentMountsTests");
    const FString Engine  = Base + TEXT("/Engine Folder/Content");
    const FString Project = Base + TEXT("/My Game/Content");

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::EngineRoot, Engine));
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, Project + TEXT("/")));

    TEST_SECTION(TEXT("The default mounts are the engine's and the project's Content directories"));
    {
        const FContentMounts Defaults = FContentMounts::CreateDefault();
        TEST_EXPECT(Defaults.GetMounts().Size() == 2);
        TEST_EXPECT(Defaults.FindMount(TEXT("/Engine/A")) && Defaults.FindMount(TEXT("/Engine/A"))->Directory.Equals(File::MakeAbsolute(Paths::GetEngineContentDir())));
        TEST_EXPECT(Defaults.FindMount(TEXT("/Project/A")) && Defaults.FindMount(TEXT("/Project/A"))->Directory.Equals(File::MakeAbsolute(Paths::GetProjectContentDir())));
    }

    TEST_SECTION(TEXT("A virtual path becomes a path in its mount's folder"));
    {
        FString DiskPath;
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Engine/Textures/Brick.dxrasset"), DiskPath) && DiskPath.Equals(Engine + TEXT("/Textures/Brick.dxrasset")));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project/Scenes/Level 1.dxrscene"), DiskPath) && DiskPath.Equals(Project + TEXT("/Scenes/Level 1.dxrscene")));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("\\Project\\Scenes\\A.dxrscene"), DiskPath) && DiskPath.Equals(Project + TEXT("/Scenes/A.dxrscene")));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project//Scenes/./B/../A.dxrscene"), DiskPath) && DiskPath.Equals(Project + TEXT("/Scenes/A.dxrscene")));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project/Scenes/"), DiskPath) && DiskPath.Equals(Project + TEXT("/Scenes")));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project"), DiskPath) && DiskPath.Equals(Project));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project/"), DiskPath) && DiskPath.Equals(Project));
        TEST_EXPECT(Mounts.ToDiskPath(TEXT("/Project/Scenes/.."), DiskPath) && DiskPath.Equals(Project));
    }

    TEST_SECTION(TEXT("A virtual path outside every mount, or leaving its mount, has no disk path"));
    {
        FString DiskPath;
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/Project/../Secret.txt"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/Project/Scenes/../../Secret.txt"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/Project/.."), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/Other/A.dxrasset"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/project/A.dxrasset"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("Project/A.dxrasset"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/"), DiskPath));
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT(""), DiskPath));
#if PLATFORM_WINDOWS
        TEST_EXPECT(!Mounts.ToDiskPath(TEXT("/Project/C:/Windows/System32"), DiskPath));
#endif
    }

    TEST_SECTION(TEXT("A disk path becomes a virtual path under the mount holding it"));
    {
        FString VirtualPath;
        TEST_EXPECT(Mounts.ToVirtualPath(Engine + TEXT("/Textures/Brick.dxrasset"), VirtualPath) && VirtualPath.Equals(TEXT("/Engine/Textures/Brick.dxrasset")));
        TEST_EXPECT(Mounts.ToVirtualPath(Project + TEXT("\\Scenes\\A.dxrscene"), VirtualPath) && VirtualPath.Equals(TEXT("/Project/Scenes/A.dxrscene")));
        TEST_EXPECT(Mounts.ToVirtualPath(Project + TEXT("/Scenes/B/../A.dxrscene"), VirtualPath) && VirtualPath.Equals(TEXT("/Project/Scenes/A.dxrscene")));
        TEST_EXPECT(Mounts.ToVirtualPath(Project, VirtualPath) && VirtualPath.Equals(TEXT("/Project/")));
        TEST_EXPECT(Mounts.ToVirtualPath(Project + TEXT("/"), VirtualPath) && VirtualPath.Equals(TEXT("/Project/")));
        TEST_EXPECT(!Mounts.ToVirtualPath(Base + TEXT("/My Game/Config/Engine.ini"), VirtualPath));
        TEST_EXPECT(!Mounts.ToVirtualPath(Base + TEXT("/My Game/ContentSources/A.png"), VirtualPath));
#if PLATFORM_WINDOWS
        TEST_EXPECT(Mounts.ToVirtualPath(Project.ToUpper() + TEXT("/Scenes/A.dxrscene"), VirtualPath) && VirtualPath.Equals(TEXT("/Project/Scenes/A.dxrscene")));
#endif
    }

    TEST_SECTION(TEXT("A relative disk path is taken from the working directory"));
    {
        FContentMounts Local;
        TEST_EXPECT(Local.Mount(TEXT("/Local/"), FPlatformFile::GetCurrentWorkingDirectory() + TEXT("/Content")));

        FString VirtualPath;
        TEST_EXPECT(Local.ToVirtualPath(TEXT("Content/A.dxrasset"), VirtualPath) && VirtualPath.Equals(TEXT("/Local/A.dxrasset")));
    }

    TEST_SECTION(TEXT("Disk and virtual paths round trip"));
    {
        const FString VirtualPaths[] = { TEXT("/Engine/Meshes/Primitives/Cube.dxrasset"), TEXT("/Project/A.dxrasset"), TEXT("/Project/") };
        for (const FString& Path : VirtualPaths)
        {
            FString DiskPath;
            FString VirtualPath;
            TEST_EXPECT(Mounts.ToDiskPath(Path, DiskPath) && Mounts.ToVirtualPath(DiskPath, VirtualPath) && VirtualPath.Equals(Path));
        }
    }

    TEST_SECTION(TEXT("The deepest mount holding a path wins"));
    {
        FContentMounts Nested = Mounts;
        TEST_EXPECT(Nested.Mount(TEXT("/Plugin/"), Engine + TEXT("/Plugins/Water")));

        FString VirtualPath;
        TEST_EXPECT(Nested.ToVirtualPath(Engine + TEXT("/Plugins/Water/Foam.dxrasset"), VirtualPath) && VirtualPath.Equals(TEXT("/Plugin/Foam.dxrasset")));
        TEST_EXPECT(Nested.ToVirtualPath(Engine + TEXT("/Plugins/Other.dxrasset"), VirtualPath) && VirtualPath.Equals(TEXT("/Engine/Plugins/Other.dxrasset")));
    }

    TEST_SECTION(TEXT("A root is one name between slashes, mounting it again moves it, and it can be unmounted"));
    {
        FContentMounts Edited = Mounts;
        TEST_EXPECT(!Edited.Mount(TEXT("/Two/Names/"), Project));
        TEST_EXPECT(!Edited.Mount(TEXT("NoSlashes"), Project));
        TEST_EXPECT(!Edited.Mount(TEXT("/Spaced Name/"), Project));
        TEST_EXPECT(!Edited.Mount(TEXT("/../"), Project));
        TEST_EXPECT(!Edited.Mount(TEXT("//"), Project));
        TEST_EXPECT(!Edited.Mount(TEXT("/Empty/"), TEXT("")));
        TEST_EXPECT(Edited.GetMounts().Size() == 2);

        TEST_EXPECT(Edited.Mount(FContentMounts::ProjectRoot, Base + TEXT("/Other Game/Content")));
        TEST_EXPECT(Edited.GetMounts().Size() == 2);

        FString DiskPath;
        TEST_EXPECT(Edited.ToDiskPath(TEXT("/Project/A.dxrasset"), DiskPath) && DiskPath.Equals(Base + TEXT("/Other Game/Content/A.dxrasset")));

        TEST_EXPECT(Edited.Unmount(FContentMounts::ProjectRoot));
        TEST_EXPECT(!Edited.Unmount(FContentMounts::ProjectRoot));
        TEST_EXPECT(!Edited.ToDiskPath(TEXT("/Project/A.dxrasset"), DiskPath));
        TEST_EXPECT(Edited.FindMount(TEXT("/Engine/A")) != nullptr);
    }

    TEST_END();
}
