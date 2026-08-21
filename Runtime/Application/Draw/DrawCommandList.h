#pragma once
#include "Core/Containers/Array.h"
#include "Application/Draw/DrawTypes.h"

class APPLICATION_API FDrawCommandList
{
public:
    static constexpr int32 InvalidIndex = -1;

public:
    FDrawCommandList();
    ~FDrawCommandList();

    /**
     * @brief Appends a filled rectangle.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle to fill.
     * @param Tint    The fill color.
     */
    void AddBox(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint);

    /**
     * @brief Appends a run of text on one line.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle the text is anchored to at the top-left.
     * @param InText  The text to draw.
     * @param Font    The face the text was measured with, which may be null.
     * @param Tint    The text color.
     */
    void AddText(int32 LayerId, const FRectangle& Bounds, const String& InText, const IFontFace* Font, const FFloatColor& Tint);

    /**
     * @brief Appends a thin axis-aligned rule.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle covered by the line.
     * @param Tint    The line color.
     */
    void AddLine(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint);

    /**
     * @brief Opens a clip region, intersected with whatever region is already open.
     *
     * @param LayerId       The layer the region belongs to.
     * @param ClipRectangle The region to clip to.
     */
    void PushClip(int32 LayerId, const FRectangle& ClipRectangle);

    /**
     * @brief Closes the region opened by the matching PushClip.
     *
     * @param LayerId The layer the region belongs to.
     */
    void PopClip(int32 LayerId);

    /** @brief Drops every command and the clip state, so the list can be filled again. */
    void Reset();

    /** @brief True when every push has a matching pop and no pop was unmatched. */
    NODISCARD FORCEINLINE bool IsClipStackBalanced() const
    {
        return ClipStack.IsEmpty() && UnmatchedPopCount == 0;
    }

    /** @brief The region currently on top of the clip stack, or an empty rectangle when none is open. */
    NODISCARD FORCEINLINE const FRectangle& GetCurrentClipRectangle() const
    {
        return ClipStack.IsEmpty() ? EmptyClipRectangle : ClipStack.Last();
    }

    /** @brief The number of commands in the list. */
    NODISCARD FORCEINLINE int32 Size() const
    {
        return Commands.Size();
    }

    /** @brief True when nothing has been appended. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Commands.IsEmpty();
    }

    /** @brief The commands, in emission order. */
    NODISCARD FORCEINLINE const TArray<FDrawCommand>& GetCommands() const
    {
        return Commands;
    }

    NODISCARD FORCEINLINE const FDrawCommand& operator[](int32 Index) const
    {
        return Commands[Index];
    }

    /**
     * @brief Counts the commands of one type, so a test can check what an element emitted.
     *
     * @param Type The command type to count.
     * @return The number of commands of that type.
     */
    NODISCARD int32 CountCommandsOfType(EDrawCommandType Type) const;

    /**
     * @brief Finds the first text command whose string matches.
     *
     * @param InText The text to look for.
     * @return The index of the command, or InvalidIndex when there is none.
     */
    NODISCARD int32 FindTextCommand(const StringView& InText) const;

private:
    TArray<FDrawCommand> Commands;
    TArray<FRectangle>   ClipStack;
    FRectangle           EmptyClipRectangle;
    int32                UnmatchedPopCount;
};
