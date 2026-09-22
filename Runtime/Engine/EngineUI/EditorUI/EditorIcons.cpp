#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Application/Draw/UIAtlas.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"

struct FIconRequest
{
    const CHAR* RelativePath;
    FUIBrush*   Brush;
};

struct FIconImage
{
    TArray<uint8> Pixels;
    int32         Width  = 0;
    int32         Height = 0;
};

static const FIconRequest GIconRequests[] =
{
    { "Editor/Icons/Undo.png",               &FEditorIcons::Undo               },
    { "Editor/Icons/Search.png",             &FEditorIcons::Search             },
    { "Editor/Icons/Locked.png",             &FEditorIcons::Locked             },
    { "Editor/Icons/Unlocked.png",           &FEditorIcons::Unlocked           },
    { "Editor/Icons/Folder.png",             &FEditorIcons::Folder             },
    { "Editor/Icons/FolderSmall.png",        &FEditorIcons::FolderSmall        },
    { "Editor/Icons/FolderSmall2.png",       &FEditorIcons::FolderSmall2       },
    { "Editor/Icons/FolderOpenSmall.png",    &FEditorIcons::FolderOpenSmall    },
    { "Editor/Icons/Document.png",           &FEditorIcons::Document           },
    { "Editor/Icons/DocumentSmall.png",      &FEditorIcons::DocumentSmall      },
    { "Editor/Icons/Checkmark.png",          &FEditorIcons::Checkmark          },
    { "Editor/Icons/Forbidden.png",          &FEditorIcons::Forbidden          },
    { "Editor/Icons/CircledCheckmark.png",   &FEditorIcons::CircledCheckmark   },
    { "Editor/Icons/Next.png",               &FEditorIcons::Next               },
    { "Editor/Icons/Previous.png",           &FEditorIcons::Previous           },
    { "Editor/Icons/Close.png",              &FEditorIcons::Close              },
    { "Editor/Icons/Filter.png",             &FEditorIcons::Filter             },
    { "Editor/Icons/RightArrow.png",         &FEditorIcons::RightArrow         },
    { "Editor/Icons/DownArrow.png",          &FEditorIcons::DownArrow          },
    { "Editor/Icons/CollapseArrowDown.png",  &FEditorIcons::CollapseArrowDown  },
    { "Editor/Icons/CollapseArrowRight.png", &FEditorIcons::CollapseArrowRight },
};

static FUIAtlas GIconAtlas;

static bool LoadIconImage(const CHAR* RelativePath, FIconImage& OutImage)
{
    String FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += RelativePath;

    FTextureImporterBase Importer;

    TSharedRef<FTexture> Texture   = Importer.ImportFromFile(StringView(FullPath));
    FTexture2D*          Texture2D = Texture ? Texture->GetTexture2D() : nullptr;

    if (!Texture2D)
    {
        LOG_ERROR("[FEditorIcons]: Failed to load icon '%s'", *FullPath);
        return false;
    }

    const EFormat Format = Texture2D->GetFormat();
    if (Format != EFormat::R8G8B8A8_Unorm)
    {
        LOG_ERROR("[FEditorIcons]: Icon '%s' is '%s', and the atlas only takes 'R8G8B8A8_Unorm'", *FullPath, ToString(Format));
        return false;
    }

    const FTextureResourceData* IconData     = Texture2D->GetTextureResourceData();
    const uint8*                SourcePixels = IconData ? reinterpret_cast<const uint8*>(IconData->GetMipData(0)) : nullptr;

    if (!SourcePixels)
    {
        LOG_ERROR("[FEditorIcons]: Icon '%s' has no pixel data", *FullPath);
        return false;
    }

    OutImage.Width  = static_cast<int32>(Texture2D->GetWidth());
    OutImage.Height = static_cast<int32>(Texture2D->GetHeight());

    const int64 NumBytes = static_cast<int64>(OutImage.Width) * OutImage.Height * 4;
    OutImage.Pixels.ResizeUninitialized(static_cast<int32>(NumBytes));
    Memory::Memcpy(OutImage.Pixels.Data(), SourcePixels, NumBytes);

    return true;
}

FUIBrush FEditorIcons::Undo;
FUIBrush FEditorIcons::Search;
FUIBrush FEditorIcons::Locked;
FUIBrush FEditorIcons::Unlocked;
FUIBrush FEditorIcons::Folder;
FUIBrush FEditorIcons::FolderSmall;
FUIBrush FEditorIcons::FolderSmall2;
FUIBrush FEditorIcons::FolderOpenSmall;
FUIBrush FEditorIcons::Document;
FUIBrush FEditorIcons::DocumentSmall;
FUIBrush FEditorIcons::Checkmark;
FUIBrush FEditorIcons::Forbidden;
FUIBrush FEditorIcons::CircledCheckmark;
FUIBrush FEditorIcons::Next;
FUIBrush FEditorIcons::Previous;
FUIBrush FEditorIcons::Close;
FUIBrush FEditorIcons::Filter;
FUIBrush FEditorIcons::RightArrow;
FUIBrush FEditorIcons::DownArrow;
FUIBrush FEditorIcons::CollapseArrowDown;
FUIBrush FEditorIcons::CollapseArrowRight;

bool FEditorIcons::Initialize()
{
    Release();

    GIconAtlas.SetDebugName("Editor UI IconAtlas");

    bool bResult = true;

    for (const FIconRequest& Request : GIconRequests)
    {
        FIconImage Image;
        if (!LoadIconImage(Request.RelativePath, Image))
        {
            bResult = false;
            continue;
        }

        *Request.Brush = GIconAtlas.Add(Image.Pixels.Data(), Image.Width, Image.Height);

        if (!Request.Brush->IsValid())
        {
            LOG_ERROR("[FEditorIcons]: Icon '%s' did not fit the atlas", Request.RelativePath);
            bResult = false;
        }
    }

    return GIconAtlas.Build() && bResult;
}

void FEditorIcons::Release()
{
    for (const FIconRequest& Request : GIconRequests)
    {
        *Request.Brush = FUIBrush();
    }

    GIconAtlas.Release();
}
