#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include "Core/Containers/UniquePtr.h"
#include <imgui.h>

struct PopupAnchor
{
    ImVec2 Min              = ImVec2(0.0f, 0.0f);
    ImVec2 Max              = ImVec2(0.0f, 0.0f);
    bool   bRequestPosition = false;
};

struct EditorStyleVars
{
    static float  MainMenuBarHeight;

    static ImVec2 InputFieldFramePadding;
    static float  InputFieldBorderThickness;
    static float  InputFieldBorderRounding;
    static ImU32  InputFieldBorderColor;
    
    static ImVec2 SceneHierarchyItemSpacing;
    static ImVec2 SceneHierarchyWindowPadding;
    static float  SceneHierarchyTableRowHeight;

    static ImVec2 PropertiesItemSpacing;
    static ImVec2 PropertiesWindowPadding;
    static ImVec2 PropertiesCollapsingHeaderItemSpacing;
    static ImVec2 PropertiesCollapsingFramePadding;
    static float  PropertiesCollapsingFrameRounding;
};

struct ENGINE_API EditorHelpers
{
    static FORCEINLINE ImU32 MakeBrighterColorU32(const ImVec4& HoveredColor, float BrightenAmount = 0.20f)
    {
        ImVec4 BrighterColor = HoveredColor;
        BrighterColor.x = (BrighterColor.x + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.x + BrightenAmount);
        BrighterColor.y = (BrighterColor.y + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.y + BrightenAmount);
        BrighterColor.z = (BrighterColor.z + BrightenAmount > 1.0f) ? 1.0f : (BrighterColor.z + BrightenAmount);
        BrighterColor.w = 1.0f;

        return ImGui::GetColorU32(BrighterColor);
    }
};

enum class EVector3ControlType : uint8
{
    Default,
    Position,
    RotationDegrees,
    Scale,
};

struct FRichTextSpan
{
    FString Text;
    
    ImU32 TextColor       = IM_COL32(255, 255, 255, 255);
    bool  bHasBackground  = false;
    ImU32 BackgroundColor = 0;
};

struct FRichTextLine
{
    TArray<FRichTextSpan> Spans;
    int32 TotalChars = 0;
};

struct FRichTextSelectionPoint
{
    int32 Line   = 0;
    int32 Column = 0;
};

struct FRichTextViewContext
{
    void ClearForNewFrame()
    {
        Lines.Clear();
        bActive = false;
    }

    bool    bAutoScroll     = true;
    bool    bScrollToBottom = false;
    bool    bSelecting      = false;
    bool    bHasSelection   = false;
    bool    bActive         = false;
    float   LineHeight      = 0.0f;
    float   CharWidth       = 0.0f;
    ImVec2  ContentStart    = ImVec2(0, 0);
    ImVec2  Padding         = ImVec2(8.0f, 4.0f);
    ImGuiID ViewId          = 0;

    FRichTextSelectionPoint SelStart;
    FRichTextSelectionPoint SelEnd;
    TArray<FRichTextLine>   Lines;

};

struct ENGINE_API EditorWidgets
{
    // -----------------------------------------------------------------------------------------
    // Menu
    // -----------------------------------------------------------------------------------------

    static bool DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float Speed, const FVector3* InRevertValue, EVector3ControlType InType);
    static bool DrawFloatProperty(const CHAR* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled = true);
    static bool DrawCheckboxProperty(const CHAR* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled = true);
    static void DrawTextProperty(const CHAR* Label, const CHAR* ValueText);
    static void DrawReadOnlyFloat3Property(const CHAR* Label, const FVector3& Value);

    static bool DrawColor3Property(const CHAR* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags);
 
    static FORCEINLINE bool DrawColor3Property(const CHAR* Label, FFloatColor& InOutColor, const FFloatColor& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
    {
        return DrawColor3Property(Label, InOutColor.RGBA, InRevertColor.RGBA, bEnabled, Flags);
    }

    static FORCEINLINE bool DrawColor3Property(const CHAR* Label, FVector3& InOutColor, const FVector3& InRevertColor, bool bEnabled = true, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
    {
        return DrawColor3Property(Label, InOutColor.XYZ, InRevertColor.XYZ, bEnabled, Flags);
    }

    static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FVector3& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.XYZ, Flags);
    }

    static FORCEINLINE bool DrawColorEdit3(const CHAR* Label, FFloatColor& OutColor, ImGuiColorEditFlags Flags = 0)
    {
        return ImGui::ColorEdit3(Label, OutColor.RGBA, Flags);
    }

    // -----------------------------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------------------------

    static bool EditorSearchField(const CHAR* InId, const CHAR* InHint, CHAR* InOutBuffer, int32 InBufferSize, float InWidth = -1.0f, bool bDrawBorder = true);

    // -----------------------------------------------------------------------------------------
    // Menu
    // -----------------------------------------------------------------------------------------

    static void EditorMenuSeparator(float Thickness = 1.0f, float PaddingY = 4.0f);
    static void EditorMenuLabeledSeparator(const CHAR* Label, float Thickness = 1.0f, float PaddingY = 4.0f);
    static bool EditorMenuItem(const CHAR* Label, const CHAR* Shortcut = nullptr, bool bSelected = false, bool bEnabled = true, bool bDrawBorder = false);
    static void EditorDrawMenuButton(const CHAR* Label, const CHAR* PopupId, bool bAnyPopupOpen, float ButtonHeight, PopupAnchor& OutAnchor, bool bDrawBorder = false);
    static bool EditorBeginMenuPopup(const CHAR* PopupId, const PopupAnchor& Anchor, float MinWidth = 180.0f);
    static void EditorResetMenuPopup();

    // -----------------------------------------------------------------------------------------
    // Property Table
    // -----------------------------------------------------------------------------------------

    static bool BeginPropertyTable(const CHAR* TableId, float LabelColumnWidth = 200.0f, float RevertColumnWidth = 20.0f);
    static void EndPropertyTable();
    static void PropertyRowLabel(const CHAR* Label);
    static void PropertySeparatorRow(float PaddingY = 4.0f);

    // -----------------------------------------------------------------------------------------
    // Rich Text View
    // -----------------------------------------------------------------------------------------

    static bool BeginRichTextView(const CHAR* InId, const ImVec2& InSize, FRichTextViewContext& InOutContext, ImGuiWindowFlags InFlags = 0);
    static void RichTextLineBegin(FRichTextViewContext& InOutContext);
    static void RichTextAddText(FRichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor);
    static void RichTextAddTextBg(FRichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor, ImU32 InBackgroundColor);
    static void RichTextLineEnd(FRichTextViewContext& InOutContext);
    static void EndRichTextView(FRichTextViewContext& InOutContext);

    // -----------------------------------------------------------------------------------------
    // Other
    // -----------------------------------------------------------------------------------------

    static bool ButtonCenteredOnLine(const CHAR* Label, float Alignment = 0.5f);
    static void EditorDrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize);
};

struct FImGuiTexture;

struct EditorIcons
{
    static ImTextureID UndoIcon;
    static ImTextureID SearchIcon;
    static ImTextureID LockedIcon;
    static ImTextureID UnlockedIcon;
    static ImTextureID FolderIcon;
    static ImTextureID FolderSmallIcon;
    static ImTextureID FolderOpenSmallIcon;
    static ImTextureID DocumentIcon;
    static ImTextureID DocumentSmallIcon;
    static ImTextureID CheckmarkIcon;
    static ImTextureID NextIcon;
    static ImTextureID PreviousIcon;
    static ImTextureID CloseIcon;
    static ImTextureID FilterIcon;
    static ImTextureID RightArrowIcon;
    static ImTextureID DownArrowIcon;
    static ImTextureID CollapseArrowDown;
    static ImTextureID CollapseArrowRight;

    static bool Initialize();
    static void Release();
};

struct EditorFonts
{
    static ImFont* DefaultFont;
    static ImFont* SegoeUI_18;
    static ImFont* SegoeUI_22;
    static ImFont* Consola_16;

    static bool Initialize();
    static void Release();
};
