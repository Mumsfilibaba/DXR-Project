#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

/** @brief Called once a tick for whether a row still holds its default, which is what shows and hides its revert arrow. */
DECLARE_RETURN_DELEGATE(FOnPropertyModified, bool);

struct FPropertyRow
{
    /** @brief Drawn in the left column, moved right by the indent level. */
    String Label;

    /** @brief Shown while the cursor rests on the row, or empty for a row that explains itself. */
    String ToolTipText;

    /** @brief The element the right column holds, which is null for a heading. */
    TSharedPtr<FVisualElement> Editor;

    /** @brief Placed just past the label at its desired size, which is where a per-row toggle goes. */
    TSharedPtr<FVisualElement> LabelAccessory;

    /** @brief Whether the row is a heading, which spans both columns. */
    bool bIsHeader = false;

    /** @brief How many levels the label is moved right by, which is what nests a struct's fields. */
    int32 IndentLevel = 0;

    /** @brief How tall the row is, in pixels, where zero takes the table's height instead. */
    int32 HeightOverride = 0;

    /** @brief Polled each frame to decide whether the revert arrow is drawn, an unbound delegate never drawing it. */
    FOnPropertyModified IsModified;

    /** @brief Fired when the arrow is clicked, which is what puts the row back to its default. */
    FOnClicked OnRevert;
};

class APPLICATION_API FPropertyTable final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The face the labels are measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief How tall every row is, in pixels. */
        int32 RowHeight = FUIStyle::GetDefault().Metrics.RowHeight;

        /** @brief The share of the width the label column takes, clamped when it is applied. */
        float LabelColumnFraction = 0.4f;

        /** @brief How far one indent level moves a label right, in pixels. */
        int32 IndentPerLevel = 12;

        /** @brief The look of the rows and the rules between them, defaulting to the shared table style. */
        FUIPropertyTableStyle Style = FUIStyle::GetDefault().PropertyTable;

        /** @brief Drawn in the revert column, an unset brush falling back to a stroked arc and head. */
        FUIBrush RevertIcon;

        /**
         * @brief Whether the table keeps a column at its right edge for the revert arrows. ImGui reserves the
         * column whether or not a row fills it, so the editors of a table with one revertable row all end at
         * the same place.
         */
        bool bShowRevertColumn = false;

        /** @brief Whether every second row is filled with the alternate shade rather than the normal one. */
        bool bAlternateRowColors = false;
    };

public:

    /** @brief The narrowest share of the width the label column can be dragged down to. */
    static constexpr float MinLabelColumnFraction = 0.15f;

    /** @brief The widest share of the width the label column can be dragged up to. */
    static constexpr float MaxLabelColumnFraction = 0.85f;

    /** @brief How wide the band the column divider is grabbed by is, in pixels. */
    static constexpr int32 DividerGrabWidth = 4;

    /** @brief The index reported while the cursor is over no row. */
    static constexpr int32 InvalidRowIndex = -1;

public:
    static TSharedPtr<FPropertyTable> Create(const FDesc& Desc);

public:
    FPropertyTable();
    virtual ~FPropertyTable();

    /**
     * @brief Initializes the property table with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

    /**
     * @brief Appends a label and editor pair below the rows already there.
     *
     * @param Label  The text the left column draws.
     * @param Editor The element the right column holds, which may be null.
     * @return The new row, so the caller can set its tool tip and its indent level.
     */
    FPropertyRow& AddRow(const String& Label, const TSharedPtr<FVisualElement>& Editor);

    /**
     * @brief Appends a heading below the rows already there, which spans both columns and takes no editor.
     *
     * @param Label The text the heading draws.
     * @return The new row, so the caller can set its tool tip and its indent level.
     */
    FPropertyRow& AddHeaderRow(const String& Label);

    /**
     * @brief Places an element beside a row's label, which is what puts the uniform lock next to Scale.
     *
     * @param Index     The row to place it on, which has to name a row.
     * @param Accessory The element to place, a null one clearing whatever the row carried.
     */
    void SetRowLabelAccessory(int32 Index, const TSharedPtr<FVisualElement>& Accessory);

    /** @brief Drops every row, so the table can be refilled. */
    void ClearRows();

    /** @return How many rows the table holds, headings counted. */
    NODISCARD FORCEINLINE int32 GetNumRows() const
    {
        return Rows.Size();
    }

    /**
     * @brief One row of the table.
     *
     * @param Index The row to read, which has to name a row.
     * @return The row, in the order the rows were added.
     */
    NODISCARD const FPropertyRow& GetRow(int32 Index) const;

    /**
     * @brief Moves the column divider, which is what a drag on it does.
     *
     * @param InFraction The share of the width the label column takes, clamped into the allowed range.
     */
    void SetLabelColumnFraction(float InFraction);

    /** @return The share of the width the label column takes, between the two clamps. */
    NODISCARD FORCEINLINE float GetLabelColumnFraction() const
    {
        return LabelColumnFraction;
    }

    /**
     * @brief The rectangle one row occupies.
     *
     * @param Index  The row to place.
     * @param Bounds The rectangle the table was arranged into.
     * @return The row's rectangle, which is empty when the index names no row.
     */
    NODISCARD FRectangle GetRowRectangle(int32 Index, const FRectangle& Bounds) const;

    /**
     * @brief The band the column divider is grabbed by, which is wider than the line it draws.
     *
     * @param Bounds The rectangle the table was arranged into.
     * @return The band, centred on the divider and DividerGrabWidth wide.
     */
    NODISCARD FRectangle GetDividerRectangle(const FRectangle& Bounds) const;

    /**
     * @brief The row a point falls on.
     *
     * @param ClientPosition The point to place, in client coordinates.
     * @return The row's index, or InvalidRowIndex when the point is outside the rows.
     */
    NODISCARD int32 FindRowAtPoint(const IntVector2& ClientPosition) const;

private:
    NODISCARD int32 GetDividerOffset(const FRectangle& Bounds) const;
    NODISCARD bool IsPointOnDivider(const IntVector2& ClientPosition) const;
    NODISCARD int32 GetRowHeight(int32 Index) const;
    NODISCARD int32 GetRowOffset(int32 Index) const;
    NODISCARD int32 GetTotalRowHeight() const;
    NODISCARD int32 GetEditorColumnRight(const FRectangle& Bounds) const;
    NODISCARD FRectangle GetLabelRectangle(int32 Index, const FRectangle& Bounds) const;
    NODISCARD FRectangle GetLabelAccessoryRectangle(int32 Index, const FRectangle& Bounds) const;
    NODISCARD FRectangle GetRevertRectangle(int32 Index, const FRectangle& Bounds) const;
    NODISCARD bool IsRowModified(int32 Index) const;

    void UpdateHoveredRow(const IntVector2& ClientPosition);
    void ClearHoveredRow();

    TArray<FPropertyRow>  Rows;
    TSharedPtr<IFontFace> Font;
    FUIPropertyTableStyle Style;
    FUIBrush              RevertIcon;
    IntVector2            DragOrigin;
    float                 LabelColumnFraction;
    float                 DragStartFraction;
    int32                 RowHeight;
    int32                 IndentPerLevel;
    int32                 HoveredRowIndex;
    bool                  bShowRevertColumn;
    bool                  bAlternateRowColors;
    bool                  bIsDraggingDivider;
    bool                  bIsDividerHovered;
};
