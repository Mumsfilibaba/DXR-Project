#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Docking/DockNode.h"

struct APPLICATION_API FDockWindowLayout
{
    FDockWindowLayout();

    /** @brief The caption the window had, which a restored host shows until its tabs change. */
    String Title;

    /** @brief Where the window sat on the desktop, ignored for the main window. */
    IntVector2 Position;

    /** @brief The client size the window had, zero when the file carried none. */
    IntVector2 Size;

    /** @brief Whether the window was maximized, which restores on top of the size it had before that. */
    bool bIsMaximized;

    /** @brief The tree the window's area held. */
    FDockNode Root;
};

class APPLICATION_API FDockLayoutFile
{
public:

    /** @brief The version written today, which carries a section per window. */
    static constexpr int32 CurrentVersion = 2;

    /** @brief The version that held one tree and no window sections, read as a main window on its own. */
    static constexpr int32 SingleWindowVersion = 1;

public:

    /**
     * @brief Writes every window's tree to one file, flattened to a section per node.
     *
     * @param Filename Where to write it.
     * @param Windows  The windows to write, the main window first.
     * @return True when the file was written.
     */
    static bool Save(const String& Filename, const TArray<FDockWindowLayout>& Windows);

    /**
     * @brief Reads every window back, taking a version 1 file as a main window on its own. A host whose tree
     * cannot be read is skipped rather than failing the whole file, because losing one torn-off window is
     * better than losing the arrangement of all of them.
     *
     * @param Filename   The file to read.
     * @param OutWindows Receives the windows, the main window first, cleared before anything is added.
     * @return True when at least the main window was read.
     */
    static bool Load(const String& Filename, TArray<FDockWindowLayout>& OutWindows);
};
