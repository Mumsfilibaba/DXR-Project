#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include "Core/Containers/UniquePtr.h"
#include "ImGuiPlugin/ImGuiCore.h"

struct FImGuiTexture;

// Name of the platform's shortcut modifier, for composing menu shortcut labels
#if PLATFORM_MACOS
    #define EDITOR_SHORTCUT_MOD "Cmd"
#else
    #define EDITOR_SHORTCUT_MOD "Ctrl"
#endif

enum class EVector3ControlType : uint8
{
    Default,
    Position,
    RotationDegrees,
    Scale,
};

enum class EPropertyTableVerticalAlign : uint8
{
    Top,
    Center,
};

enum class EPopupPlacement : uint8
{
    // Below the anchor, flipping above and/or right-aligning when there is no room
    BelowAnchor,

    // Right of the anchor, flipping to its left when there is no room
    RightOfAnchor,

    // Keep the requested position, only nudge it back inside the monitor
    ClampOnly,
};

struct FPopupAnchor
{
    ImVec2 Min              = ImVec2(0.0f, 0.0f);
    ImVec2 Max              = ImVec2(0.0f, 0.0f);
    bool   bRequestPosition = false;
};

inline constexpr float MenuContentIndentX    = 20.0f;
inline constexpr float MenuItemPaddingX      = 8.0f;
inline constexpr float MenuLabelIndentX      = MenuContentIndentX + MenuItemPaddingX;
inline constexpr float MenuRadioLabelIndentX = 42.0f;
inline constexpr float MenuDefaultMinWidth   = 180.0f;
inline constexpr float MenuSubMenuOverlapX   = 2.0f;

inline constexpr float EditorDefaultWindowWidthFraction  = 0.60f;
inline constexpr float EditorDefaultWindowHeightFraction = 0.50f;
inline constexpr float EditorDefaultWindowMinWidth       = 480.0f;
inline constexpr float EditorDefaultWindowMaxWidth       = 1152.0f;
inline constexpr float EditorDefaultWindowMinHeight      = 360.0f;
inline constexpr float EditorDefaultWindowMaxHeight      = 960.0f;

struct FSubMenuState
{
    FPopupAnchor  Anchor;
    const CHAR_T* Label        = nullptr;
    const CHAR_T* PopupId      = nullptr;
    float         LabelIndentX = MenuLabelIndentX;
    float         MinWidth     = MenuDefaultMinWidth;
    bool          bRowHovered  = false;
};

struct FRichTextSpan
{
    int32 TextOffset      = 0;
    int32 TextLength      = 0;
    float Width           = 0.0f;
    ImU32 TextColor       = IM_COL32(255, 255, 255, 255);
    ImU32 BackgroundColor = 0;
    bool  bHasBackground  = false;
};

struct FRichTextLine
{
    TArray<FRichTextSpan> Spans;
    int32                 TotalChars = 0;
};

struct FRichTextSelectionPoint
{
    int32 Line   = 0;
    int32 Column = 0;
};

struct FRichTextViewContext
{
    void Clear()
    {
        Lines.Clear();
        TextArena.Clear();

        MaxLineChars  = 0;
        bHasSelection = false;
        bSelecting    = false;
    }

    const CHAR_T* GetSpanText(const FRichTextSpan& InSpan) const
    {
        return TextArena.Data() + InSpan.TextOffset;
    }

    TArray<FRichTextLine>   Lines;
    TArray<CHAR_T>          TextArena;
    FRichTextSelectionPoint SelStart;
    FRichTextSelectionPoint SelEnd;
    ImFont*                 MeasuredFont        = nullptr;
    float                   MeasuredFontSize    = 0.0f;
    ImGuiID                 ViewId              = 0;
    ImVec2                  Padding             = ImVec2(12.0f, 8.0f);
    ImVec2                  ContentStart        = ImVec2(0, 0);
    float                   LineHeight          = 0.0f;
    float                   CharWidth           = 0.0f;
    int32                   MaxLineChars        = 0;
    bool                    bAutoScroll         = true;
    bool                    bScrollToBottom     = false;
    bool                    bIsScrolledToBottom = true;
    bool                    bSelecting          = false;
    bool                    bHasSelection       = false;
    bool                    bActive             = false;
};

struct FErrorWindowContext
{
    TArray<FString> Entries;
    FString         Title;
    FString         HeaderText;
    bool            bVisible = false;
};

struct FConfirmDialogContext
{
    FString Title;
    FString Message;
    bool    bVisible = false;
};

// -----------------------------------------------------------------------------------------
// Axis colors (gizmo + details panel)
// -----------------------------------------------------------------------------------------

namespace EditorAxisColors
{
    constexpr ImVec4 X          = ImVec4(204.0f / 255.0f, 26.0f / 255.0f, 38.0f / 255.0f, 1.0f);
    constexpr ImVec4 Y          = ImVec4(51.0f / 255.0f, 179.0f / 255.0f, 51.0f / 255.0f, 1.0f);
    constexpr ImVec4 Z          = ImVec4(26.0f / 255.0f, 64.0f / 255.0f, 204.0f / 255.0f, 1.0f);
    constexpr ImVec4 Selection  = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    constexpr float  PlaneAlpha = 0.40f;
}

// -----------------------------------------------------------------------------------------
// Style-vars
// -----------------------------------------------------------------------------------------

struct ENGINE_API EditorStyleVars
{
    static float  MainMenuBarHeight;

    static ImVec2 InputFieldFramePadding;
    static float  InputFieldBorderThickness;
    static float  InputFieldBorderRounding;
    static ImU32  InputFieldBorderColor;
    static ImVec4 InputFieldSelectionColor;
    
    static ImVec2 SceneHierarchyItemSpacing;
    static ImVec2 SceneHierarchyWindowPadding;
    static float  SceneHierarchyTableRowHeight;

    static ImVec2 PropertiesItemSpacing;
    static ImVec2 PropertiesWindowPadding;
    static ImVec2 PropertiesCollapsingHeaderItemSpacing;
    static ImVec2 PropertiesCollapsingFramePadding;
    static float  PropertiesCollapsingFrameRounding;

    static float CheckboxSizeScale;

    static float PropertyTableLabelIndentX;
    static float PropertyTableCellPaddingX;

    static float ButtonRounding;
    static float ButtonPaddingX;
    static float ButtonFramePaddingY;
    static float ButtonContentGap;
    static float ButtonArrowSize;
    static ImU32 ButtonBgIdle;
    static ImU32 ButtonBgHovered;
    static ImU32 ButtonBgSelected;
    static ImU32 ButtonBgSelectedHovered;
};

struct FPropertyTableStyle
{
    FPropertyTableStyle()
        : LabelIndentX(EditorStyleVars::PropertyTableLabelIndentX)
        , CellPaddingX(EditorStyleVars::PropertyTableCellPaddingX)
        , VerticalAlign(EPropertyTableVerticalAlign::Center)
    {
    }

    float                       LabelIndentX;
    float                       CellPaddingX;
    EPropertyTableVerticalAlign VerticalAlign;
};

// -----------------------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------------------

struct ENGINE_API EditorHelpers
{
    static FORCEINLINE ImU32 CreateBrighterColorU32(const ImVec4& HoveredColor, float BrightenAmount = 0.20f)
    {
        ImVec4 BrighterColor = HoveredColor;
        BrighterColor.x = (BrighterColor.x + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.x + BrightenAmount);
        BrighterColor.y = (BrighterColor.y + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.y + BrightenAmount);
        BrighterColor.z = (BrighterColor.z + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.z + BrightenAmount);
        BrighterColor.w = 1.0f;

        return ImGui::GetColorU32(BrighterColor);
    }

    static const CHAR_T* GetTrimmedQuery(const CHAR_T* InText, CHAR_T* OutBuf, int32 OutBufSize);
    static void FormatBytes(int64 Bytes, char* OutBuffer, int32 BufferSize);
};

// -----------------------------------------------------------------------------------------
// Widgets
// -----------------------------------------------------------------------------------------

struct ENGINE_API EditorWidgets
{
    // -----------------------------------------------------------------------------------------
    // Inputs
    // -----------------------------------------------------------------------------------------

    static bool DrawFloat3Control(const CHAR_T* Label, FVector3& OutValue, float Speed, const FVector3* InRevertValue, EVector3ControlType InType);
    static bool DrawFloatProperty(const CHAR_T* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR_T* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled = true);
    static bool DrawIntProperty(const CHAR_T* Label, int32& InOutValue, float Speed, int32 MinValue, int32 MaxValue, const CHAR_T* Format, bool bUseSlider, const int32* InRevertValue, bool bEnabled = true);
    static bool DrawCheckbox(const CHAR_T* Label, bool& InOutValue, bool bEnabled = true);
    static bool DrawCheckboxProperty(const CHAR_T* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled = true);
    static bool DrawComboProperty(const CHAR_T* Label, int32& InOutValue, const CHAR_T* const* Items, int32 ItemCount, const int32* InRevertValue, bool bEnabled = true);
    static bool DrawTextProperty(const CHAR_T* Label, const CHAR_T* ValueText);
    static void DrawReadOnlyFloat3Property(const CHAR_T* Label, const FVector3& Value);

    static void DrawTextureProperty(const CHAR_T* Label, ImTextureID Texture, float PreviewSize = 48.0f);

    static bool DrawColor3Property(const CHAR_T* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags);
 
    static FORCEINLINE bool DrawColor3Property(const CHAR_T* Label, FFloatColor& InOutColor, const FFloatColor& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
    {
        return DrawColor3Property(Label, InOutColor.RGBA, InRevertColor.RGBA, bEnabled, Flags);
    }

    static FORCEINLINE bool DrawColor3Property(const CHAR_T* Label, FVector3& InOutColor, const FVector3& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
    {
        return DrawColor3Property(Label, InOutColor.XYZ, InRevertColor.XYZ, bEnabled, Flags);
    }

    static FORCEINLINE bool DrawColorEdit3(const CHAR_T* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.XYZ, Flags);
    }

    static FORCEINLINE bool DrawColorEdit3(const CHAR_T* Label, FFloatColor& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.RGBA, Flags);
    }

    // -----------------------------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------------------------

    static bool DrawSearchField(const CHAR_T* InId, const CHAR_T* InHint, CHAR_T* InOutBuffer, int32 InBufferSize, float InWidth = -1.0f, bool bDrawBorder = true);
    static void DrawTextWithSearchHighlight(ImDrawList* DrawList, const ImVec2& TextPos, const CHAR_T* Text, const CHAR_T* FilterText, ImU32 BaseTextU32, float HighlightPadX = 1.0f, float HighlightPadY = 1.0f, const ImVec2* ClampMin = nullptr, const ImVec2* ClampMax = nullptr);

    // -----------------------------------------------------------------------------------------
    // Popup
    // -----------------------------------------------------------------------------------------

    static ImRect GetPopupExtentRect(const ImVec2& RefPos);

    static bool SetNextPopupPos(const CHAR_T* WindowId, const FPopupAnchor& Anchor, EPopupPlacement Placement, const ImVec2& FixedSize = ImVec2(0.0f, 0.0f));
    static bool SetNextBeginPopupPos(const CHAR_T* PopupId, const FPopupAnchor& Anchor, EPopupPlacement Placement, const ImVec2& FixedSize = ImVec2(0.0f, 0.0f));

    static bool BeginMenuPopup(const CHAR_T* PopupId, const FPopupAnchor& Anchor, float MinWidth = 180.0f, EPopupPlacement Placement = EPopupPlacement::BelowAnchor);
    static bool BeginPopupContextWindow(const CHAR_T* PopupId, ImGuiPopupFlags Flags = ImGuiPopupFlags_MouseButtonRight);
    static bool BeginPopupContextItem(const CHAR_T* PopupId);
    static bool BeginPopupContext(const CHAR_T* PopupId);

    static bool BeginSubMenu(FSubMenuState& InOutState, const CHAR_T* PopupId, const CHAR_T* Label, bool bEnabled = true);
    static void EndSubMenu(FSubMenuState& InOutState);

    static bool MenuSubMenuRow(const CHAR_T* Label, FPopupAnchor& OutAnchor, bool& bOutHovered, bool bForceActive, float LabelIndentX = MenuLabelIndentX);
    static void MenuSubMenuOverlay(const CHAR_T* Label, const FPopupAnchor& Anchor, float LabelIndentX = MenuLabelIndentX);

    static void MenuSeparator(float Thickness = 1.0f, float PaddingY = 4.0f);
    static void MenuLabeledSeparator(const CHAR_T* Label, float Thickness = 1.0f, float PaddingY = 4.0f);
    static bool MenuItem(const CHAR_T* Label, const CHAR_T* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true, bool bDrawBorder = false);
    static bool MenuSliderFloat(const CHAR_T* Label, float& InOutValue, float MinValue, float MaxValue, const CHAR_T* Format, float ValueWidth = 96.0f, bool bEnabled = true);
    static bool MenuDragFloat(const CHAR_T* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR_T* Format, float ValueWidth = 96.0f, bool bEnabled = true);
    static void MenuButton(const CHAR_T* Label, const CHAR_T* PopupId, bool bAnyPopupOpen, float ButtonHeight, FPopupAnchor& OutAnchor, bool bDrawBorder = false);
    
    static void EndMenuPopup();
    static void EndPopupContext();

    // -----------------------------------------------------------------------------------------
    // Property Table
    // -----------------------------------------------------------------------------------------

    static bool BeginPropertyTable(const CHAR_T* TableId, float LabelColumnWidth = 200.0f, float RevertColumnWidth = 20.0f, const FPropertyTableStyle& Style = FPropertyTableStyle());
    static void EndPropertyTable();
    static void PropertyRowLabel(const CHAR_T* Label);
    static void PropertyTableBeginValueCell(float ContentHeight = -1.0f);
    static void PropertySeparatorRow(float PaddingY = 4.0f);

    // -----------------------------------------------------------------------------------------
    // Rich Text View
    // -----------------------------------------------------------------------------------------

    static bool BeginRichTextView(const CHAR_T* InId, const ImVec2& InSize, FRichTextViewContext& InOutContext, ImGuiWindowFlags InFlags = 0, bool bWithContextMenu = true);
    static void RichTextSelectAll(FRichTextViewContext& InOutContext);
    static void RichTextNewLine(FRichTextViewContext& InOutContext);
    static void RichTextAddText(FRichTextViewContext& InOutContext, const CHAR_T* InText, ImU32 InTextColor);
    static void RichTextAddText(FRichTextViewContext& InOutContext, const CHAR_T* InText, int32 InLength, ImU32 InTextColor);
    static void RichTextAddTextBg(FRichTextViewContext& InOutContext, const CHAR_T* InText, ImU32 InTextColor, ImU32 InBackgroundColor);
    static void RichTextAddTextBg(FRichTextViewContext& InOutContext, const CHAR_T* InText, int32 InLength, ImU32 InTextColor, ImU32 InBackgroundColor);
    static void EndRichTextView(FRichTextViewContext& InOutContext);

    static FString GetSelectedRichText(const FRichTextViewContext& InContext);
    static FString GetAllRichText(const FRichTextViewContext& InContext);

    // -----------------------------------------------------------------------------------------
    // Buttons
    // -----------------------------------------------------------------------------------------

    static ImVec2 GetButtonSize(const CHAR_T* Label, const ImVec2& Size);

    static bool DrawButton(const CHAR_T* Label, const ImVec2& Size = ImVec2(0.0f, 0.0f), bool bSelected = false, ImDrawFlags Corners = ImDrawFlags_RoundCornersAll);
    static bool DrawDropdownButton(const CHAR_T* InId, const CHAR_T* Label, const ImVec2& Size, bool bPopupOpen, FPopupAnchor& OutAnchor, bool& bOutHovered);
    static bool DrawDialogButton(const CHAR_T* Label, const ImVec2& Size);
    static bool DrawButtonCenteredOnLine(const CHAR_T* Label, float Alignment = 0.5f);

    // -----------------------------------------------------------------------------------------
    // Editor window
    // -----------------------------------------------------------------------------------------

    static ImVec2 GetDefaultEditorWindowSize();
    static ImVec2 ScaleEditorWindowSize(const ImVec2& LogicalSize);
    static bool BeginEditorWindow(const CHAR_T* Title, bool* pbVisible, ImGuiWindowFlags ExtraFlags = 0);
    static void EndEditorWindow();

    // -----------------------------------------------------------------------------------------
    // Error handling
    // -----------------------------------------------------------------------------------------

    static void DrawErrorWindow(FErrorWindowContext& InOutContext);

    // -----------------------------------------------------------------------------------------
    // Confirmation dialog
    // -----------------------------------------------------------------------------------------

    static bool DrawConfirmDialog(FConfirmDialogContext& InOutContext);

    // -----------------------------------------------------------------------------------------
    // Other
    // -----------------------------------------------------------------------------------------

    static void DrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize);

    static void DrawIcon(ImDrawList* DrawList, const struct FEditorIcon& Icon, const ImVec2& Min, const ImVec2& Max, ImU32 Tint = IM_COL32_WHITE);
};

// -----------------------------------------------------------------------------------------
// Icons
// -----------------------------------------------------------------------------------------

struct FEditorIcon
{
    NODISCARD bool IsValid() const
    {
        return Texture != nullptr;
    }

    explicit operator bool() const
    {
        return IsValid();
    }

    ImTextureID Texture = nullptr;
    ImVec2      UVMin   = ImVec2(0.0f, 0.0f);
    ImVec2      UVMax   = ImVec2(1.0f, 1.0f);
};

struct ENGINE_API EditorIcons
{
    static FEditorIcon UndoIcon;
    static FEditorIcon SearchIcon;
    static FEditorIcon LockedIcon;
    static FEditorIcon UnlockedIcon;
    static FEditorIcon FolderIcon;
    static FEditorIcon FolderSmallIcon;
    static FEditorIcon FolderSmall2Icon;
    static FEditorIcon FolderOpenSmallIcon;
    static FEditorIcon DocumentIcon;
    static FEditorIcon DocumentSmallIcon;
    static FEditorIcon CheckmarkIcon;
    static FEditorIcon ForbiddenIcon;
    static FEditorIcon CircledCheckmarkIcon;
    static FEditorIcon NextIcon;
    static FEditorIcon PreviousIcon;
    static FEditorIcon CloseIcon;
    static FEditorIcon FilterIcon;
    static FEditorIcon RightArrowIcon;
    static FEditorIcon DownArrowIcon;
    static FEditorIcon CollapseArrowDown;
    static FEditorIcon CollapseArrowRight;

    static bool Initialize();
    static void Release();
};

// -----------------------------------------------------------------------------------------
// Fonts
// -----------------------------------------------------------------------------------------

struct ENGINE_API EditorFonts
{
    static ImFont* DefaultFont;
    static ImFont* SystemIcons;
    static ImFont* SegoeUI_18;
    static ImFont* SegoeUI_22;
    static ImFont* Consola_16;

    static bool Initialize();
    static void Release();
};
