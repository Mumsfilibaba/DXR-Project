#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Application/Draw/UIAtlas.h"
#include "Core/Containers/Array.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"

struct FIconRequest
{
    const CHAR_T* RelativePath;
    FUIBrush*     Brush;
};

struct FIconImage
{
    TArray<uint8> Pixels;
    int32         Width  = 0;
    int32         Height = 0;
};

static const FIconRequest GIconRequests[] =
{
    { TEXT("Editor/Icons/Undo.png"),               &FEditorIcons::Undo               },
    { TEXT("Editor/Icons/Search.png"),             &FEditorIcons::Search             },
    { TEXT("Editor/Icons/Locked.png"),             &FEditorIcons::Locked             },
    { TEXT("Editor/Icons/Unlocked.png"),           &FEditorIcons::Unlocked           },
    { TEXT("Editor/Icons/Folder.png"),             &FEditorIcons::Folder             },
    { TEXT("Editor/Icons/FolderSmall.png"),        &FEditorIcons::FolderSmall        },
    { TEXT("Editor/Icons/FolderSmall2.png"),       &FEditorIcons::FolderSmall2       },
    { TEXT("Editor/Icons/FolderOpenSmall.png"),    &FEditorIcons::FolderOpenSmall    },
    { TEXT("Editor/Icons/Document.png"),           &FEditorIcons::Document           },
    { TEXT("Editor/Icons/DocumentSmall.png"),      &FEditorIcons::DocumentSmall      },
    { TEXT("Editor/Icons/Checkmark.png"),          &FEditorIcons::Checkmark          },
    { TEXT("Editor/Icons/Forbidden.png"),          &FEditorIcons::Forbidden          },
    { TEXT("Editor/Icons/CircledCheckmark.png"),   &FEditorIcons::CircledCheckmark   },
    { TEXT("Editor/Icons/Next.png"),               &FEditorIcons::Next               },
    { TEXT("Editor/Icons/Previous.png"),           &FEditorIcons::Previous           },
    { TEXT("Editor/Icons/Close.png"),              &FEditorIcons::Close              },
    { TEXT("Editor/Icons/Filter.png"),             &FEditorIcons::Filter             },
    { TEXT("Editor/Icons/RightArrow.png"),         &FEditorIcons::RightArrow         },
    { TEXT("Editor/Icons/DownArrow.png"),          &FEditorIcons::DownArrow          },
    { TEXT("Editor/Icons/CollapseArrowDown.png"),  &FEditorIcons::CollapseArrowDown  },
    { TEXT("Editor/Icons/CollapseArrowRight.png"), &FEditorIcons::CollapseArrowRight },
    { TEXT("Editor/Icons/MatchCase.png"),          &FEditorIcons::MatchCase          },
    { TEXT("Editor/Icons/Translate.png"),          &FEditorIcons::Translate          },
    { TEXT("Editor/Icons/Rotate.png"),             &FEditorIcons::Rotate             },
    { TEXT("Editor/Icons/Scale.png"),              &FEditorIcons::Scale              },
    { TEXT("Editor/Icons/GizmoCenter.png"),        &FEditorIcons::GizmoCenter        },
    { TEXT("Editor/Icons/GizmoPivot.png"),         &FEditorIcons::GizmoPivot         },
    { TEXT("Editor/Icons/GizmoLocal.png"),         &FEditorIcons::GizmoLocal         },
    { TEXT("Editor/Icons/GizmoWorld.png"),         &FEditorIcons::GizmoWorld         },
    { TEXT("Editor/Icons/Play.png"),               &FEditorIcons::Play               },
    { TEXT("Editor/Icons/Pause.png"),              &FEditorIcons::Pause              },
    { TEXT("Editor/Icons/Stop.png"),               &FEditorIcons::Stop               },
};

static FUIAtlas GIconAtlas;

static bool LoadIconImage(const CHAR_T* RelativePath, FIconImage& OutImage)
{
    FString FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith(TEXT("/")))
    {
        FullPath += TEXT("/");
    }

    FullPath += RelativePath;

    FTextureImporterBase Importer;

    TSharedPtr<FTexture> Texture   = Importer.ImportFromFile(FStringView(FullPath));
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
FUIBrush FEditorIcons::MatchCase;
FUIBrush FEditorIcons::Translate;
FUIBrush FEditorIcons::Rotate;
FUIBrush FEditorIcons::Scale;
FUIBrush FEditorIcons::GizmoCenter;
FUIBrush FEditorIcons::GizmoPivot;
FUIBrush FEditorIcons::GizmoLocal;
FUIBrush FEditorIcons::GizmoWorld;
FUIBrush FEditorIcons::Play;
FUIBrush FEditorIcons::Pause;
FUIBrush FEditorIcons::Stop;

bool FEditorIcons::Initialize()
{
    Release();

    GIconAtlas.SetDebugName(TEXT("Editor UI IconAtlas"));

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
