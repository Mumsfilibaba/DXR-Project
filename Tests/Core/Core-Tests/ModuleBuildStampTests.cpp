#include "ModuleBuildStampTests.h"

#include <Core/Modules/ModuleImplementation.h>
#include <Core/Misc/BuildInfo.h>
#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformLibrary.h>

#include "TestCommon/TestMacros.h"

extern "C" MODULE_EXPORT extern const ANSICHAR GModuleBuildStampTestExport[] = "Module build stamp test export";

bool ModuleBuildStamp_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The build stamp is the commit, whether the tree was dirty, the configuration and the character width"));
    {
        const FString Expected = FString::Printf(TEXT("%s%s %s %s"), BuildInfo::GetCommit(), BuildInfo::IsWorkingTreeDirty() ? TEXT("+dirty") : TEXT(""),
            BuildInfo::GetConfigurationName(), sizeof(CHAR_T) == 1 ? TEXT("Narrow") : TEXT("Wide"));

        TEST_EXPECT(FModuleManager::GetBuildStamp().Equals(Expected));
        TEST_EXPECT(FModuleManager::GetBuildStamp().Equals(StringCast<CHAR_T>(ENGINE_MODULE_BUILD_STAMP).ToString()));
    }

#if PLATFORM_WINDOWS
    TEST_SECTION(TEXT("A string export is read from the library file without loading it"));
    {
        const FString Executable = FPlatformFile::GetExecutablePath();

        FString Value;
        TEST_EXPECT(FPlatformLibrary::ReadStringExport(*Executable, TEXT("GModuleBuildStampTestExport"), Value));
        TEST_EXPECT(Value.Equals(TEXT("Module build stamp test export")));
        TEST_EXPECT(!FPlatformLibrary::ReadStringExport(*Executable, TEXT("GModuleBuildStampMissingExport"), Value));
    }

    TEST_SECTION(TEXT("A file that is not a library, or no file, has no exports"));
    {
        const FString NotALibrary = TEXT("ModuleBuildStampTests.NotALibrary.dll");
        {
            TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(NotALibrary);
            TEST_EXPECT(FileHandle && File::WriteTextFile(FileHandle.Get(), FString(TEXT("Not a library"))));
        }

        FString Value;
        TEST_EXPECT(!FPlatformLibrary::ReadStringExport(*NotALibrary, TEXT("GModuleBuildStampTestExport"), Value));
        TEST_EXPECT(!FPlatformLibrary::ReadStringExport(TEXT("ModuleBuildStampTests.Missing.dll"), TEXT("GModuleBuildStampTestExport"), Value));
        TEST_EXPECT(FPlatformFile::DeleteFile(*NotALibrary));
    }
#endif

    TEST_END();
}
