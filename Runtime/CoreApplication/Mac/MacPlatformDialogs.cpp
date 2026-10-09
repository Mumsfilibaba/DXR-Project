#include "Core/Mac/Mac.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Mac/ScopedAutoreleasePool.h"
#include "CoreApplication/Mac/MacPlatformDialogs.h"
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

enum class EFileDialogKind : uint8
{
    Open,
    Save,
    PickFolder,
};

static NSString* ToNSString(const String& Text)
{
    return [NSString stringWithUTF8String:*Text];
}

static NSArray<UTType*>* CreateContentTypes(const TArray<FFileDialogFilter>& Filters)
{
    NSMutableArray<UTType*>* Types = [NSMutableArray array];
    for (const FFileDialogFilter& Filter : Filters)
    {
        NSArray<NSString*>* Extensions = [ToNSString(Filter.Extensions) componentsSeparatedByString:@";"];
        for (NSString* Extension in Extensions)
        {
            if ([Extension isEqualToString:@"*"])
            {
                return nil;
            }

            UTType* Type = [UTType typeWithFilenameExtension:Extension];
            if (Type)
            {
                [Types addObject:Type];
            }
        }
    }

    return ([Types count] > 0) ? Types : nil;
}

static NSSavePanel* CreatePanel(const FFileDialogDesc& Desc, EFileDialogKind Kind)
{
    NSSavePanel* Panel = nil;
    if (Kind == EFileDialogKind::Save)
    {
        Panel = [NSSavePanel savePanel];
    }
    else
    {
        NSOpenPanel* OpenPanel = [NSOpenPanel openPanel];
        [OpenPanel setCanChooseFiles:(Kind == EFileDialogKind::Open)];
        [OpenPanel setCanChooseDirectories:(Kind == EFileDialogKind::PickFolder)];
        [OpenPanel setAllowsMultipleSelection:((Kind == EFileDialogKind::Open) && Desc.bAllowMultiple)];
        Panel = OpenPanel;
    }

    if (!Desc.Title.IsEmpty())
    {
        [Panel setMessage:ToNSString(Desc.Title)];
    }

    if (!Desc.DefaultDirectory.IsEmpty())
    {
        [Panel setDirectoryURL:[NSURL fileURLWithPath:ToNSString(Desc.DefaultDirectory) isDirectory:YES]];
    }

    if ((Kind == EFileDialogKind::Save) && !Desc.DefaultFilename.IsEmpty())
    {
        [Panel setNameFieldStringValue:ToNSString(Desc.DefaultFilename)];
    }

    if (Kind != EFileDialogKind::PickFolder)
    {
        NSArray<UTType*>* ContentTypes = CreateContentTypes(Desc.Filters);
        if (ContentTypes)
        {
            [Panel setAllowedContentTypes:ContentTypes];
        }
    }

    return Panel;
}

static bool ShowFileDialog(const FFileDialogDesc& Desc, EFileDialogKind Kind, TArray<String>& OutPaths)
{
    OutPaths.Clear();

    SCOPED_AUTORELEASE_POOL();

    NSMutableArray<NSString*>* SelectedPaths = [NSMutableArray array];
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSSavePanel* Panel = CreatePanel(Desc, Kind);
        if ([Panel runModal] != NSModalResponseOK)
        {
            return;
        }

        if (Kind == EFileDialogKind::Save)
        {
            [SelectedPaths addObject:[[Panel URL] path]];
        }
        else
        {
            for (NSURL* URL in [(NSOpenPanel*)Panel URLs])
            {
                [SelectedPaths addObject:[URL path]];
            }
        }
    }, NSDefaultRunLoopMode, true);

    for (NSString* Path in SelectedPaths)
    {
        OutPaths.Add(String([Path UTF8String]));
    }

    return !OutPaths.IsEmpty();
}

bool FMacPlatformDialogs::OpenFile(const FFileDialogDesc& Desc, TArray<String>& OutFilenames)
{
    return ShowFileDialog(Desc, EFileDialogKind::Open, OutFilenames);
}

bool FMacPlatformDialogs::SaveFile(const FFileDialogDesc& Desc, String& OutFilename)
{
    TArray<String> Paths;
    if (!ShowFileDialog(Desc, EFileDialogKind::Save, Paths))
    {
        return false;
    }

    OutFilename = Paths[0];
    return true;
}

bool FMacPlatformDialogs::PickFolder(const FFileDialogDesc& Desc, String& OutDirectory)
{
    TArray<String> Paths;
    if (!ShowFileDialog(Desc, EFileDialogKind::PickFolder, Paths))
    {
        return false;
    }

    OutDirectory = Paths[0];
    return true;
}
