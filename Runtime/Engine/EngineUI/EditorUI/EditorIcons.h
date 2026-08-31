#pragma once
#include "Application/Draw/DrawTypes.h"
#include "RHI/RHIResources.h"

struct ENGINE_API FEditorIcons
{
    /**
     * @brief Packs every editor icon into one texture and fills in the brushes that sample it.
     *
     * @return True when the atlas was built. A single missing file leaves that one brush unset and still returns false.
     */
    static bool Initialize();

    /** @brief Drops the atlas and unsets every brush. */
    static void Release();

    static FUIBrush Undo;
    static FUIBrush Search;
    static FUIBrush Locked;
    static FUIBrush Unlocked;
    static FUIBrush Folder;
    static FUIBrush FolderSmall;
    static FUIBrush FolderSmall2;
    static FUIBrush FolderOpenSmall;
    static FUIBrush Document;
    static FUIBrush DocumentSmall;
    static FUIBrush Checkmark;
    static FUIBrush Forbidden;
    static FUIBrush CircledCheckmark;
    static FUIBrush Next;
    static FUIBrush Previous;
    static FUIBrush Close;
    static FUIBrush Filter;
    static FUIBrush RightArrow;
    static FUIBrush DownArrow;
    static FUIBrush CollapseArrowDown;
    static FUIBrush CollapseArrowRight;
};
