#include "CoreApplication/Windows/WindowsPlatformDialogs.h"
#include "Core/Windows/Windows.h"
#include "Core/Containers/ComPtr.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include <objbase.h>
#include <shobjidl.h>

enum class EFileDialogKind : uint8
{
    Open,
    Save,
    PickFolder,
};

static DWORD GetDialogOptions(EFileDialogKind Kind, bool bAllowMultiple)
{
    DWORD Options = FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;

    if (Kind == EFileDialogKind::Open)
    {
        Options |= FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST;
    }

    if ((Kind == EFileDialogKind::Open) && bAllowMultiple)
    {
        Options |= FOS_ALLOWMULTISELECT;
    }

    if (Kind == EFileDialogKind::Save)
    {
        Options |= FOS_OVERWRITEPROMPT;
    }

    if (Kind == EFileDialogKind::PickFolder)
    {
        Options |= FOS_PICKFOLDERS;
    }

    return Options;
}

static String CreateFilterSpec(const String& Extensions)
{
    String Spec;
    String Extension;

    for (int32 Index = 0; Index <= Extensions.Length(); ++Index)
    {
        if ((Index < Extensions.Length()) && (Extensions[Index] != ';'))
        {
            Extension.Append(Extensions[Index]);
            continue;
        }

        if (!Extension.IsEmpty())
        {
            Spec.Append(Spec.IsEmpty() ? "*." : ";*.");
            Spec.Append(Extension);
        }

        Extension.Clear();
    }

    return Spec;
}

static String ShellItemToPath(IShellItem* Item)
{
    PWSTR WidePath = nullptr;
    if (FAILED(Item->GetDisplayName(SIGDN_FILESYSPATH, &WidePath)))
    {
        return String();
    }

    String Path = WideToChar(WStringView(WidePath));
    ::CoTaskMemFree(WidePath);

    Path.ReplaceAll('\\', '/');
    return Path;
}

static void ApplyDesc(IFileDialog* Dialog, const FFileDialogDesc& Desc, EFileDialogKind Kind, TArray<WString>& OutFilterStrings)
{
    DWORD Options = 0;
    Dialog->GetOptions(&Options);
    Dialog->SetOptions(Options | GetDialogOptions(Kind, Desc.bAllowMultiple));

    if (!Desc.Title.IsEmpty())
    {
        Dialog->SetTitle(*CharToWide(Desc.Title));
    }

    if (!Desc.DefaultFilename.IsEmpty())
    {
        Dialog->SetFileName(*CharToWide(Desc.DefaultFilename));
    }

    if (!Desc.DefaultDirectory.IsEmpty())
    {
        String Directory = Desc.DefaultDirectory;
        Directory.ReplaceAll('/', '\\');

        TComPtr<IShellItem> Folder;
        if (SUCCEEDED(::SHCreateItemFromParsingName(*CharToWide(Directory), nullptr, IID_PPV_ARGS(&Folder))))
        {
            Dialog->SetFolder(Folder.Get());
        }
    }

    if ((Kind != EFileDialogKind::PickFolder) && !Desc.Filters.IsEmpty())
    {
        OutFilterStrings.Clear();
        for (const FFileDialogFilter& Filter : Desc.Filters)
        {
            OutFilterStrings.Add(CharToWide(Filter.Description));
            OutFilterStrings.Add(CharToWide(CreateFilterSpec(Filter.Extensions)));
        }

        TArray<COMDLG_FILTERSPEC> FilterEntries;
        for (int32 Index = 0; Index < OutFilterStrings.Size(); Index += 2)
        {
            COMDLG_FILTERSPEC Entry;
            Entry.pszName = *OutFilterStrings[Index];
            Entry.pszSpec = *OutFilterStrings[Index + 1];
            FilterEntries.Add(Entry);
        }

        Dialog->SetFileTypes(static_cast<UINT>(FilterEntries.Size()), FilterEntries.Data());
        Dialog->SetFileTypeIndex(1);
    }

    if ((Kind == EFileDialogKind::Save) && !Desc.Filters.IsEmpty())
    {
        const String& Extensions = Desc.Filters[0].Extensions;
        const int32   Separator  = Extensions.FindChar(';');
        const String  First      = (Separator == String::InvalidIndex) ? Extensions : String(*Extensions, Separator);

        if (!First.IsEmpty() && !First.Equals("*"))
        {
            Dialog->SetDefaultExtension(*CharToWide(First));
        }
    }
}

static void CollectResults(IFileDialog* Dialog, EFileDialogKind Kind, TArray<String>& OutPaths)
{
    if (Kind != EFileDialogKind::Open)
    {
        TComPtr<IShellItem> Item;
        if (SUCCEEDED(Dialog->GetResult(&Item)))
        {
            OutPaths.Add(ShellItemToPath(Item.Get()));
        }

        return;
    }

    TComPtr<IFileOpenDialog> OpenDialog;
    TComPtr<IShellItemArray> Items;

    if (FAILED(Dialog->QueryInterface(IID_PPV_ARGS(&OpenDialog))) || FAILED(OpenDialog->GetResults(&Items)))
    {
        return;
    }

    DWORD NumItems = 0;
    Items->GetCount(&NumItems);

    for (DWORD Index = 0; Index < NumItems; ++Index)
    {
        TComPtr<IShellItem> Item;
        if (SUCCEEDED(Items->GetItemAt(Index, &Item)))
        {
            OutPaths.Add(ShellItemToPath(Item.Get()));
        }
    }
}

static bool RunFileDialog(const FFileDialogDesc& Desc, EFileDialogKind Kind, TArray<String>& OutPaths)
{
    const CLSID DialogClass = (Kind == EFileDialogKind::Save) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;

    TComPtr<IFileDialog> Dialog;
    if (FAILED(::CoCreateInstance(DialogClass, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog))))
    {
        LOG_ERROR("[FWindowsPlatformDialogs]: Failed to create the file dialog");
        return false;
    }

    TArray<WString> FilterStrings;
    ApplyDesc(Dialog.Get(), Desc, Kind, FilterStrings);

    const HRESULT ShowResult = Dialog->Show(reinterpret_cast<HWND>(Desc.ParentWindowHandle));
    if (FAILED(ShowResult))
    {
        if (ShowResult != HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            LOG_ERROR("[FWindowsPlatformDialogs]: The file dialog failed with error 0x%08x", static_cast<uint32>(ShowResult));
        }

        return false;
    }

    CollectResults(Dialog.Get(), Kind, OutPaths);
    return !OutPaths.IsEmpty();
}

static bool ShowFileDialog(const FFileDialogDesc& Desc, EFileDialogKind Kind, TArray<String>& OutPaths)
{
    OutPaths.Clear();

    const HRESULT InitializeResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool    bAccepted        = RunFileDialog(Desc, Kind, OutPaths);

    if (SUCCEEDED(InitializeResult))
    {
        ::CoUninitialize();
    }

    return bAccepted;
}

bool FWindowsPlatformDialogs::OpenFile(const FFileDialogDesc& Desc, TArray<String>& OutFilenames)
{
    return ShowFileDialog(Desc, EFileDialogKind::Open, OutFilenames);
}

bool FWindowsPlatformDialogs::SaveFile(const FFileDialogDesc& Desc, String& OutFilename)
{
    TArray<String> Paths;
    if (!ShowFileDialog(Desc, EFileDialogKind::Save, Paths))
    {
        return false;
    }

    OutFilename = Paths[0];
    return true;
}

bool FWindowsPlatformDialogs::PickFolder(const FFileDialogDesc& Desc, String& OutDirectory)
{
    TArray<String> Paths;
    if (!ShowFileDialog(Desc, EFileDialogKind::PickFolder, Paths))
    {
        return false;
    }

    OutDirectory = Paths[0];
    return true;
}
