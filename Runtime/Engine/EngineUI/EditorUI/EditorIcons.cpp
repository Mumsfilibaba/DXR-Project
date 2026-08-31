#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "RendererCore/TextureFactory.h"

/** @brief How wide the atlas every icon is packed into is, in pixels. */
constexpr int32 ICON_ATLAS_WIDTH = 1024;

/** @brief The space kept between two packed icons, so filtering cannot bleed one into the next, in pixels. */
constexpr int32 ICON_ATLAS_PADDING = 2;

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
    int32         X      = 0;
    int32         Y      = 0;
    bool          bValid = false;
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

static FRHITextureRef GAtlasTexture;

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

    OutImage.bValid = true;
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

    constexpr int32 IconCount = static_cast<int32>(ARRAY_COUNT(GIconRequests));

    TArray<FIconImage> Images;
    Images.Resize(IconCount);

    bool bResult = true;

    int32 ShelfX      = 0;
    int32 ShelfY      = 0;
    int32 ShelfHeight = 0;
    int32 AtlasHeight = 0;

    for (int32 Index = 0; Index < IconCount; ++Index)
    {
        FIconImage& Image = Images[Index];
        if (!LoadIconImage(GIconRequests[Index].RelativePath, Image))
        {
            bResult = false;
            continue;
        }

        if (Image.Width > ICON_ATLAS_WIDTH)
        {
            LOG_ERROR("[FEditorIcons]: Icon '%s' is %d wide, the atlas is only %d", GIconRequests[Index].RelativePath, Image.Width, ICON_ATLAS_WIDTH);

            Image.bValid = false;
            bResult      = false;
            continue;
        }

        if (ShelfX + Image.Width > ICON_ATLAS_WIDTH)
        {
            ShelfY     += ShelfHeight;
            ShelfX      = 0;
            ShelfHeight = 0;
        }

        Image.X = ShelfX;
        Image.Y = ShelfY;

        ShelfX      += Image.Width + ICON_ATLAS_PADDING;
        ShelfHeight  = Math::Max(ShelfHeight, Image.Height + ICON_ATLAS_PADDING);
        AtlasHeight  = Math::Max(AtlasHeight, ShelfY + Image.Height);
    }

    if (AtlasHeight <= 0)
    {
        LOG_ERROR("[FEditorIcons]: No icon could be loaded, leaving the atlas empty");
        return false;
    }

    TArray<uint8> AtlasPixels;
    AtlasPixels.ResizeUninitialized(ICON_ATLAS_WIDTH * AtlasHeight * 4);

    Memory::Memzero(AtlasPixels.Data(), static_cast<uint64>(AtlasPixels.Size()));

    for (const FIconImage& Image : Images)
    {
        if (!Image.bValid)
        {
            continue;
        }

        const int64 RowBytes = static_cast<int64>(Image.Width) * 4;
        for (int32 Y = 0; Y < Image.Height; ++Y)
        {
            uint8* Destination = AtlasPixels.Data() + ((static_cast<int64>(Image.Y + Y) * ICON_ATLAS_WIDTH) + Image.X) * 4;
            Memory::Memcpy(Destination, Image.Pixels.Data() + static_cast<int64>(Y) * RowBytes, RowBytes);
        }
    }

    GAtlasTexture = FTextureFactory::Get().LoadFromMemory(AtlasPixels.Data(), ICON_ATLAS_WIDTH, AtlasHeight, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!GAtlasTexture)
    {
        LOG_ERROR("[FEditorIcons]: Failed to create the icon atlas texture");
        return false;
    }

    GAtlasTexture->SetDebugName("Editor UI IconAtlas");

    const float AtlasWidthF  = static_cast<float>(ICON_ATLAS_WIDTH);
    const float AtlasHeightF = static_cast<float>(AtlasHeight);

    for (int32 Index = 0; Index < IconCount; ++Index)
    {
        const FIconImage& Image = Images[Index];
        if (!Image.bValid)
        {
            continue;
        }

        FUIBrush& Brush = *GIconRequests[Index].Brush;

        Brush.Texture     = GAtlasTexture.Get();
        Brush.MinTexCoord = Vector2(static_cast<float>(Image.X) / AtlasWidthF, static_cast<float>(Image.Y) / AtlasHeightF);
        Brush.MaxTexCoord = Vector2(static_cast<float>(Image.X + Image.Width) / AtlasWidthF, static_cast<float>(Image.Y + Image.Height) / AtlasHeightF);
    }

    return bResult;
}

void FEditorIcons::Release()
{
    for (const FIconRequest& Request : GIconRequests)
    {
        *Request.Brush = FUIBrush();
    }

    GAtlasTexture.Reset();
}
