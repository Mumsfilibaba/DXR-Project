#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "Engine/Resources/Texture.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureResourceData.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui_internal.h>

enum class ERichTextCharClass
{
    Word,
    Whitespace,
    Symbol,
};

enum class EMenuFloatWidget : uint8
{
    Slider = 0,
    Drag   = 1,
};

struct FPropertyTableState
{
    FPropertyTableStyle Style;
    float               CurrentRowHeight = 0.0f;
    ImVec2              BorderMin        = ImVec2(0.0f, 0.0f);
    float               BorderWidth      = 0.0f;
    bool                bActive          = false;
};

struct FSubMenuFrame
{
    bool bChildOpen = false;
};

struct FEditorIconImage
{
    TArray<uint8> Pixels;
    int32         Width  = 0;
    int32         Height = 0;
    int32         X      = 0;
    int32         Y      = 0;
    bool          bValid = false;
};

struct FEditorIconRequest
{
    const CHAR*  RelativePath;
    FEditorIcon* Icon;
};

struct FEditorIconAtlas
{
    FRHITextureRef            Texture      = nullptr;
    TUniquePtr<FImGuiTexture> ImGuiTexture = nullptr;
};

static bool GEditorWindowBegun               = false;
static bool GEditorWindowOpen                = false;
static bool GEditorWindowChildBegun          = false;
static bool GEditorWindowOuterPaddingPushed  = false;
static bool GEditorWindowContentStylePushed  = false;
static bool GEditorWindowNestedChildBgPushed = false;

static FPropertyTableState GPropertyTableState;

static TArray<FSubMenuFrame> GSubMenuStack;

static FEditorIconAtlas GIconAtlas;

static ImGuiStorage GPopupLastDirections;
static ImGuiStorage GPopupLastPlacedFrame;

static const FEditorIconRequest GIconRequests[] =
{
    { "Editor/Icons/Undo.png",               &EditorIcons::UndoIcon             },
    { "Editor/Icons/Search.png",             &EditorIcons::SearchIcon           },
    { "Editor/Icons/Locked.png",             &EditorIcons::LockedIcon           },
    { "Editor/Icons/Unlocked.png",           &EditorIcons::UnlockedIcon         },
    { "Editor/Icons/Folder.png",             &EditorIcons::FolderIcon           },
    { "Editor/Icons/FolderSmall.png",        &EditorIcons::FolderSmallIcon      },
    { "Editor/Icons/FolderSmall2.png",       &EditorIcons::FolderSmall2Icon     },
    { "Editor/Icons/FolderOpenSmall.png",    &EditorIcons::FolderOpenSmallIcon  },
    { "Editor/Icons/Document.png",           &EditorIcons::DocumentIcon         },
    { "Editor/Icons/DocumentSmall.png",      &EditorIcons::DocumentSmallIcon    },
    { "Editor/Icons/Checkmark.png",          &EditorIcons::CheckmarkIcon        },
    { "Editor/Icons/Forbidden.png",          &EditorIcons::ForbiddenIcon        },
    { "Editor/Icons/CircledCheckmark.png",   &EditorIcons::CircledCheckmarkIcon },
    { "Editor/Icons/Next.png",               &EditorIcons::NextIcon             },
    { "Editor/Icons/Previous.png",           &EditorIcons::PreviousIcon         },
    { "Editor/Icons/Close.png",              &EditorIcons::CloseIcon            },
    { "Editor/Icons/Filter.png",             &EditorIcons::FilterIcon           },
    { "Editor/Icons/RightArrow.png",         &EditorIcons::RightArrowIcon       },
    { "Editor/Icons/DownArrow.png",          &EditorIcons::DownArrowIcon        },
    { "Editor/Icons/CollapseArrowDown.png",  &EditorIcons::CollapseArrowDown    },
    { "Editor/Icons/CollapseArrowRight.png", &EditorIcons::CollapseArrowRight   },
};

static constexpr float EditorWindowBorderInset = 5.0f;
static constexpr float FixedRevertButtonSize   = 20.0f;
static constexpr float SubMenuArrowSize        = 12.0f;

static constexpr float MenuRowPaddingY   = 4.0f;
static constexpr float MenuRowIndentX    = 20.0f;
static constexpr float MenuPopupPadY     = 10.0f;
static constexpr float MenuFinalMinWidth = MenuDefaultMinWidth + (MenuContentIndentX * 2.0f);

static constexpr int32 IconAtlasWidth   = 1024;
static constexpr int32 IconAtlasPadding = 2;

#if PLATFORM_WINDOWS
// The size WinUI's own caption button style renders the glyphs at.
static constexpr float SystemIconSizePixels = 10.0f;
#endif

static bool IsWindowFloating(const ImGuiWindow* Window)
{
    return Window != nullptr && Window->DockId == 0;
}

static void DrawFrameBand(ImDrawList* DrawList, const ImVec2& Min, const ImVec2& Max, float Inset, float Thickness, ImU32 Color)
{
    const float X0 = Min.x + Inset;
    const float Y0 = Min.y + Inset;
    const float X1 = Max.x - Inset;
    const float Y1 = Max.y - Inset;

    DrawList->AddRectFilled(ImVec2(X0, Y0), ImVec2(X1, Y0 + Thickness), Color);
    DrawList->AddRectFilled(ImVec2(X0, Y1 - Thickness), ImVec2(X1, Y1), Color);
    DrawList->AddRectFilled(ImVec2(X0, Y0), ImVec2(X0 + Thickness, Y1), Color);
    DrawList->AddRectFilled(ImVec2(X1 - Thickness, Y0), ImVec2(X1, Y1), Color);
}

static void DrawFloatingWindowBorder(ImDrawList* DrawList, const ImVec2& Min, const ImVec2& Max)
{
    constexpr ImU32 OuterColor  = IM_COL32(21, 21, 21, 255);
    constexpr ImU32 MiddleColor = IM_COL32(45, 45, 45, 255);
    constexpr ImU32 InnerColor  = IM_COL32(21, 21, 21, 255);

    DrawFrameBand(DrawList, Min, Max, 0.0f, 2.0f, OuterColor);
    DrawFrameBand(DrawList, Min, Max, 2.0f, 1.0f, MiddleColor);
    DrawFrameBand(DrawList, Min, Max, 3.0f, 2.0f, InnerColor);
}

static void ApplyHoveredRowBg(bool bRowHovered)
{
    if (!bRowHovered)
    {
        return;
    }

    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return;
    }

    const ImU32 HoverBg = IM_COL32(47, 47, 47, 255);

    const int32 ColumnCount = ImGui::TableGetColumnCount();
    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        ImGui::TableSetColumnIndex(ColumnIndex);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, HoverBg);
    }
}

static bool IsRowHovered(float RowHeight)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    ImGui::TableSetColumnIndex(0);

    const ImVec2 Cursor = ImGui::GetCursorScreenPos();
    const ImVec2 RowMin = ImVec2(Table->WorkRect.Min.x, Cursor.y);
    const ImVec2 RowMax = ImVec2(Table->WorkRect.Max.x, Cursor.y + RowHeight);

    constexpr ImGuiHoveredFlags HoverFlags = ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;

    return ImGui::IsWindowHovered(HoverFlags) && ImGui::IsMouseHoveringRect(RowMin, RowMax);
}

static void DrawInputBorderRect(const ImVec2& Min, const ImVec2& Max, bool bActive, bool bHovered, float Rounding = -1.0f, float Thickness = 2.0f)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (Rounding < 0.0f)
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        Rounding = Style.FrameRounding;
    }

    const ImU32 ColorIdle   = IM_COL32(54, 54, 54, 255);
    const ImU32 ColorHover  = IM_COL32(84, 84, 84, 255);
    const ImU32 ColorActive = IM_COL32(0, 112, 224, 255);
    const ImU32 Color       = bActive ? ColorActive : (bHovered ? ColorHover : ColorIdle);

    Window->DrawList->AddRect(Min, Max, Color, Rounding, ImDrawFlags_None, Thickness);
}

static void DrawInputBorderLastItem(float Rounding = -1.0f, float Thickness = 2.0f)
{
    const bool bActive  = ImGui::IsItemActive();
    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    DrawInputBorderRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), bActive, bHovered, Rounding, Thickness);
}

static void DrawAxisLineForLastItem(ImU32 InColor)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    const ImVec2 ItemMin = ImGui::GetItemRectMin();
    const ImVec2 ItemMax = ImGui::GetItemRectMax();

    const float PadX      = 6.0f;
    const float PadY      = 5.0f;
    const float Thickness = 2.0f;
    const float X         = ItemMin.x + PadX;

    const ImVec2 LineMin = ImVec2(X - Thickness * 0.5f, ItemMin.y + PadY);
    const ImVec2 LineMax = ImVec2(X + Thickness * 0.5f, ItemMax.y - PadY);

    Window->DrawList->AddRectFilled(LineMin, LineMax, InColor, 1.0f);
}

static float ResolvePropertyTableContentHeight(float ContentHeight)
{
    if (ContentHeight < 0.0f)
    {
        return ImGui::GetFrameHeight();
    }

    return ContentHeight;
}

static float GetPropertyTableRowHeight(float ContentHeight)
{
    const float ResolvedHeight = ResolvePropertyTableContentHeight(ContentHeight);

    if (GPropertyTableState.Style.VerticalAlign == EPropertyTableVerticalAlign::Center)
    {
        return Math::Max(ResolvedHeight, ImGui::GetFrameHeight());
    }

    return ResolvedHeight;
}

static void BeginPropertyTableRow(float ContentHeight)
{
    const float RowHeight = GetPropertyTableRowHeight(ContentHeight);
    GPropertyTableState.CurrentRowHeight = RowHeight;

    if (GPropertyTableState.Style.VerticalAlign == EPropertyTableVerticalAlign::Center)
    {
        ImGui::TableNextRow(ImGuiTableRowFlags_None, RowHeight);
    }
    else
    {
        ImGui::TableNextRow();
    }
}

static void ApplyPropertyTableVerticalCenter(float ContentHeight)
{
    if (GPropertyTableState.Style.VerticalAlign != EPropertyTableVerticalAlign::Center)
    {
        return;
    }

    const float ResolvedHeight = ResolvePropertyTableContentHeight(ContentHeight);
    const float PadY           = Math::Max((GPropertyTableState.CurrentRowHeight - ResolvedHeight) * 0.5f, 0.0f);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
}

static void BeginPropertyTableLabelCell(float ContentHeight)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + GPropertyTableState.Style.LabelIndentX);

    ApplyPropertyTableVerticalCenter(ContentHeight);
}

static float GetPropertyTableValueWidth()
{
    return Math::Max(1.0f, ImGui::GetContentRegionAvail().x - GPropertyTableState.Style.CellPaddingX);
}

static void SetPropertyTableValueItemWidth()
{
    ImGui::SetNextItemWidth(GetPropertyTableValueWidth());
}

static void BeginPropertyTableValueCell(float ContentHeight)
{
    ImGui::TableSetColumnIndex(1);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + GPropertyTableState.Style.CellPaddingX);

    ApplyPropertyTableVerticalCenter(ContentHeight);
}

static void BeginPropertyTableRevertCell()
{
    ImGui::TableSetColumnIndex(2);
    ApplyPropertyTableVerticalCenter(FixedRevertButtonSize);
}

static bool ResetIconButton(float InSize = 0.0f)
{
    float ButtonSizePx = InSize;
    if (ButtonSizePx <= 0.0f)
    {
        ButtonSizePx = FixedRevertButtonSize;
    }

    ButtonSizePx = Math::Max(1.0f, ButtonSizePx);
    const ImVec2 ButtonSize = ImVec2(ButtonSizePx, ButtonSizePx);

    ImGuiID UniqueSeedId = ImGui::GetItemID();
    if (UniqueSeedId == 0)
    {
        UniqueSeedId = ImGui::GetID("##ResetIconButtonSeed");
    }

    ImGui::PushID(static_cast<int32>(UniqueSeedId));

    const bool bWasPressed = ImGui::InvisibleButton("##Revert", ButtonSize);
    const bool bIsHovered  = ImGui::IsItemHovered();
    const bool bIsHeld     = ImGui::IsItemActive();

    const ImVec2 ButtonRectMin = ImGui::GetItemRectMin();
    const ImVec2 ButtonRectMax = ImGui::GetItemRectMax();

    const ImGuiStyle& ImGuiStyle = ImGui::GetStyle();

    ImDrawList* WindowDrawList = ImGui::GetWindowDrawList();
    if (EditorIcons::UndoIcon)
    {
        const ImVec2 IconRectMin = ButtonRectMin;
        const ImVec2 IconRectMax = ButtonRectMax;

        const int32 IconTintIdle   = 220;
        const int32 IconTintHover  = 160;
        const int32 IconTintActive = 130;
        const int32 IconTintValue  = bIsHeld ? IconTintActive : (bIsHovered ? IconTintHover : IconTintIdle);

        const float Alpha01  = Math::Clamp(ImGuiStyle.Alpha, 0.0f, 1.0f);
        const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);

        const ImU32 IconTintColor = IM_COL32(IconTintValue, IconTintValue, IconTintValue, Alpha255);
        EditorWidgets::DrawIcon(WindowDrawList, EditorIcons::UndoIcon, IconRectMin, IconRectMax, IconTintColor);
    }
    else
    {
        const CHAR* FallbackText = "R";

        const ImVec2 FallbackTextSize     = ImGui::CalcTextSize(FallbackText);
        const ImVec2 FallbackTextPosition = ImVec2((ButtonRectMin.x + ButtonRectMax.x) * 0.5f - FallbackTextSize.x * 0.5f, (ButtonRectMin.y + ButtonRectMax.y) * 0.5f - FallbackTextSize.y * 0.5f);

        const ImU32 TextColor = ImGui::GetColorU32(ImGuiCol_Text);
        WindowDrawList->AddText(FallbackTextPosition, TextColor, FallbackText);
    }

    ImGui::PopID();
    return bWasPressed;
}

static void DrawSuffixAfterTempInputText(ImGuiID InItemId, const CHAR* InSuffix, ImU32 InColor, float InGap = 1.0f)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems || !InSuffix)
    {
        return;
    }

    ImGuiContext& Context = *GImGui;
    if (!ImGui::TempInputIsActive(InItemId))
    {
        return;
    }

    if (Context.InputTextState.ID != InItemId)
    {
        return;
    }

    const CHAR* EditText = Context.InputTextState.TextA.Data;
    if (!EditText)
    {
        return;
    }

    const ImVec2 ItemMin    = ImGui::GetItemRectMin();
    const ImVec2 ItemMax    = ImGui::GetItemRectMax();
    const ImVec2 TextSize   = ImGui::CalcTextSize(EditText);
    const ImVec2 SuffixSize = ImGui::CalcTextSize(InSuffix);

    const float TextStartX = ItemMin.x + ImGui::GetStyle().FramePadding.x;
    const float RightLimit = ItemMax.x - ImGui::GetStyle().FramePadding.x;
    const float DesiredX   = TextStartX + TextSize.x + InGap;

    if (DesiredX + SuffixSize.x > RightLimit)
    {
        return;
    }

    const float Y = ItemMin.y + (ItemMax.y - ItemMin.y - SuffixSize.y) * 0.5f;
    Window->DrawList->AddText(ImVec2(DesiredX, Y), InColor, InSuffix);
}

static FORCEINLINE int32 ClampInt32(int32 V, int32 MinV, int32 MaxV)
{
    return (V < MinV) ? MinV : (V > MaxV ? MaxV : V);
}

static FORCEINLINE void NormalizeSelection(FRichTextSelectionPoint& A, FRichTextSelectionPoint& B)
{
    if (A.Line > B.Line || (A.Line == B.Line && A.Column > B.Column))
    {
        FRichTextSelectionPoint Temp = A;
        A = B;
        B = Temp;
    }
}

static FORCEINLINE bool SelectionIntersectsLine(const FRichTextSelectionPoint& SelA, const FRichTextSelectionPoint& SelB, int32 LineIndex, int32& OutColStart, int32& OutColEnd, int32 LineCharCount)
{
    OutColStart = 0;
    OutColEnd   = 0;

    if (LineIndex < SelA.Line || LineIndex > SelB.Line)
    {
        return false;
    }

    if (SelA.Line == SelB.Line)
    {
        OutColStart = ClampInt32(SelA.Column, 0, LineCharCount);
        OutColEnd   = ClampInt32(SelB.Column, 0, LineCharCount);

        return OutColEnd > OutColStart;
    }

    if (LineIndex == SelA.Line)
    {
        OutColStart = ClampInt32(SelA.Column, 0, LineCharCount);
        OutColEnd   = LineCharCount;

        return OutColEnd > OutColStart;
    }

    if (LineIndex == SelB.Line)
    {
        OutColStart = 0;
        OutColEnd   = ClampInt32(SelB.Column, 0, LineCharCount);

        return OutColEnd > OutColStart;
    }

    OutColStart = 0;
    OutColEnd   = LineCharCount;

    return LineCharCount > 0;
}

static String BuildLineText(const FRichTextViewContext& Ctx, int32 LineIndex)
{
    String Result;

    if (!Ctx.Lines.IsValidIndex(LineIndex))
    {
        return Result;
    }

    const FRichTextLine& Line = Ctx.Lines[LineIndex];
    for (int32 s = 0; s < Line.Spans.Size(); ++s)
    {
        Result.Append(Ctx.GetSpanText(Line.Spans[s]), Line.Spans[s].TextLength);
    }

    return Result;
}

static String BuildSelectedText(const FRichTextViewContext& Ctx)
{
    if (!Ctx.bHasSelection || Ctx.Lines.IsEmpty())
    {
        return String();
    }

    FRichTextSelectionPoint A = Ctx.SelStart;
    FRichTextSelectionPoint B = Ctx.SelEnd;
    NormalizeSelection(A, B);

    String Result;

    const int32 LineMin = ClampInt32(A.Line, 0, Ctx.Lines.Size() - 1);
    const int32 LineMax = ClampInt32(B.Line, 0, Ctx.Lines.Size() - 1);

    for (int32 L = LineMin; L <= LineMax; ++L)
    {
        const String FullLine = BuildLineText(Ctx, L);
        const CHAR*  Full     = *FullLine;
        const int32  FullLen  = static_cast<int32>(CString::Strlen(Full));

        int32 SelColStart = 0;
        int32 SelColEnd   = 0;

        FRichTextSelectionPoint NA = A;
        FRichTextSelectionPoint NB = B;

        if (!SelectionIntersectsLine(NA, NB, L, SelColStart, SelColEnd, FullLen))
        {
            continue;
        }

        SelColStart = ClampInt32(SelColStart, 0, FullLen);
        SelColEnd   = ClampInt32(SelColEnd, 0, FullLen);

        if (SelColEnd > SelColStart)
        {
            const int32 SubLen = SelColEnd - SelColStart;

            String Sub;
            Sub.Reserve(SubLen + 1);

            for (int32 i = 0; i < SubLen; ++i)
            {
                const CHAR C = Full[SelColStart + i];
                Sub += C;
            }

            Result += Sub;

            if (L != LineMax)
            {
                Result += "\n";
            }
        }
    }

    return Result;
}

static String BuildAllText(const FRichTextViewContext& Ctx)
{
    String Result;

    for (int32 L = 0; L < Ctx.Lines.Size(); ++L)
    {
        const FRichTextLine& Line = Ctx.Lines[L];
        for (int32 s = 0; s < Line.Spans.Size(); ++s)
        {
            Result.Append(Ctx.GetSpanText(Line.Spans[s]), Line.Spans[s].TextLength);
        }

        Result += "\n";
    }

    return Result;
}

static int32 AppendToTextArena(FRichTextViewContext& Ctx, const CHAR* Text, int32 Length)
{
    const int32 Offset = Ctx.TextArena.Size();

    Ctx.TextArena.Append(Text, Length);
    Ctx.TextArena.Emplace('\0');

    return Offset;
}

static FORCEINLINE float MeasureRichTextWidth(const CHAR* Text)
{
    const ImFont* Font = ImGui::GetFont();
    if (!Font || !Text)
    {
        return 0.0f;
    }

    return Font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, Text).x;
}

static FRichTextSelectionPoint GetMouseSelectionPoint(const FRichTextViewContext& Ctx, const ImVec2& MousePos)
{
    FRichTextSelectionPoint P{};
    P.Line   = 0;
    P.Column = 0;

    const float StartY = Ctx.ContentStart.y + Ctx.Padding.y;
    const float StartX = Ctx.ContentStart.x + Ctx.Padding.x;
    const float LocalY = (MousePos.y - StartY);
    const float LocalX = (MousePos.x - StartX);

    const int32 LineIndex   = (Ctx.LineHeight > 0.0f) ? static_cast<int32>(LocalY / Ctx.LineHeight) : 0;
    const int32 MaxLine     = Math::Max(0, Ctx.Lines.Size() - 1);
    const int32 ClampedLine = ClampInt32(LineIndex, 0, MaxLine);

    P.Line = ClampedLine;

    const int32 LineChars = Ctx.Lines.IsValidIndex(ClampedLine) ? Ctx.Lines[ClampedLine].TotalChars : 0;
    const int32 Col       = (Ctx.CharWidth > 0.0f) ? static_cast<int32>(LocalX / Ctx.CharWidth) : 0;

    P.Column = ClampInt32(Col, 0, LineChars);
    return P;
}

static FORCEINLINE ERichTextCharClass GetRichTextCharClass(CHAR Char)
{
    const uint8 Byte = static_cast<uint8>(Char);

    if (Byte == ' ' || Byte == '\t')
    {
        return ERichTextCharClass::Whitespace;
    }

    const bool bIsWordChar =
        (Byte >= 'a' && Byte <= 'z') ||
        (Byte >= 'A' && Byte <= 'Z') ||
        (Byte >= '0' && Byte <= '9') ||
        (Byte == '_') ||
        (Byte >= 0x80);

    return bIsWordChar ? ERichTextCharClass::Word : ERichTextCharClass::Symbol;
}

static void FindWordBounds(const CHAR* Text, int32 TextLength, int32 Column, int32& OutStart, int32& OutEnd)
{
    OutStart = 0;
    OutEnd   = 0;

    if (!Text || TextLength <= 0)
    {
        return;
    }

    // Clicking past the last character picks the run that ends the line
    const int32 Index = ClampInt32(Column, 0, TextLength - 1);

    const ERichTextCharClass CharClass = GetRichTextCharClass(Text[Index]);

    int32 Start = Index;
    while (Start > 0 && GetRichTextCharClass(Text[Start - 1]) == CharClass)
    {
        --Start;
    }

    int32 End = Index + 1;
    while (End < TextLength && GetRichTextCharClass(Text[End]) == CharClass)
    {
        ++End;
    }

    OutStart = Start;
    OutEnd   = End;
}

static ImU32 GetButtonBackground(bool bSelected, bool bHovered, bool bHeld)
{
    if (bHovered || bHeld)
    {
        return bSelected ? EditorStyleVars::ButtonBgSelectedHovered : EditorStyleVars::ButtonBgHovered;
    }

    return bSelected ? EditorStyleVars::ButtonBgSelected : EditorStyleVars::ButtonBgIdle;
}

static float GetButtonHeight()
{
    return ImGui::GetFontSize() + EditorStyleVars::ButtonFramePaddingY * 2.0f;
}

static bool MenuFloatRow(EMenuFloatWidget WidgetType, const CHAR* Label, float& InOutValue, float Speed, 
    float MinValue, float MaxValue, const CHAR* Format, float ValueWidth, bool bEnabled)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return false;
    }

    constexpr float MenuIndentX = 20.0f;
    constexpr float PaddingY    = 4.0f;

    const float RowWidth  = ImGui::GetContentRegionAvail().x;
    const float RowHeight = Math::Max(ImGui::GetFontSize() + PaddingY * 2.0f, ImGui::GetFrameHeight());

    ImGui::PushID(Label);

    const ImVec2 RectMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(RowWidth, RowHeight));
    const ImVec2 RectMax = ImVec2(RectMin.x + RowWidth, RectMin.y + RowHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RectMin, RectMax, ImGui::GetColorU32(ImGuiCol_Header), 0.0f);

    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    const float  LabelY    = RectMin.y + (RowHeight - LabelSize.y) * 0.5f + 0.5f;

    const float FieldWidth = Math::Min(ValueWidth, Math::Max(RowWidth - MenuIndentX * 2.0f, 1.0f));
    const float FieldX     = RectMax.x - MenuIndentX - FieldWidth;
    const float FieldY     = RectMin.y + (RowHeight - ImGui::GetFrameHeight()) * 0.5f;

    DrawList->PushClipRect(ImVec2(RectMin.x + MenuIndentX, RectMin.y), ImVec2(Math::Max(FieldX - 8.0f, RectMin.x + MenuIndentX + 1.0f), RectMax.y), true);
    DrawList->AddText(ImVec2(RectMin.x + MenuIndentX, LabelY), ImGui::GetColorU32(bEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), Label);
    DrawList->PopClipRect();

    ImGui::SetCursorScreenPos(ImVec2(FieldX, FieldY));
    ImGui::SetNextItemWidth(FieldWidth);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    bool bChanged = false;
    if (WidgetType == EMenuFloatWidget::Slider)
    {
        bChanged = ImGui::SliderFloat("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bChanged = ImGui::DragFloat("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
    }

    DrawInputBorderLastItem(ImGui::GetStyle().FrameRounding);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::SetCursorScreenPos(ImVec2(RectMin.x, RectMax.y));
    ImGui::PopID();

    return bEnabled && bChanged;
}

static void PushMenuChromeStyle()
{
    // -----------------------------------------------------------------------------------------
    // Styling Vars
    // -----------------------------------------------------------------------------------------

    const ImVec4 PopupBg       = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 PopupBorder   = ImVec4(63.0f / 255.0f, 63.0f / 255.0f, 63.0f / 255.0f, 1.0f);
    const ImVec4 TextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 ShortcutColor = ImVec4(175.0f / 255.0f, 175.0f / 255.0f, 175.0f / 255.0f, 1.0f);
    const ImVec4 HoverBlue     = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    // -----------------------------------------------------------------------------------------
    // Style vars
    // -----------------------------------------------------------------------------------------

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, MenuPopupPadY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // -----------------------------------------------------------------------------------------
    // Style colors
    // -----------------------------------------------------------------------------------------

    ImGui::PushStyleColor(ImGuiCol_PopupBg, PopupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, PopupBorder);
    ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ShortcutColor);
    ImGui::PushStyleColor(ImGuiCol_Header, PopupBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverBlue);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, HoverBlue);

    // -----------------------------------------------------------------------------------------
    // Font
    // -----------------------------------------------------------------------------------------

    ImFont* Font = EditorFonts::SegoeUI_18 ? EditorFonts::SegoeUI_18 : EditorFonts::DefaultFont;
    CHECK(Font != nullptr);

    ImGui::PushFont(Font);
}

static void PopMenuChromeStyle()
{
    ImGui::PopFont();
    ImGui::PopStyleColor(7); // PopupBg, Border, Text, TextDisabled, Header, HeaderHovered, HeaderActive
    ImGui::PopStyleVar(5);   // WindowBorderSize, PopupBorderSize, PopupRounding, WindowPadding, ItemSpacing
}

static void DrawMenuFrame()
{
    const ImVec2 WinPos  = ImGui::GetWindowPos();
    const ImVec2 WinSize = ImGui::GetWindowSize();
    const ImVec2 Min     = ImVec2(WinPos.x + 1.0f, WinPos.y + 1.0f);
    const ImVec2 Max     = ImVec2(WinPos.x + WinSize.x - 1.0f, WinPos.y + WinSize.y - 1.0f);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRect(Min, Max, IM_COL32(50, 50, 50, 255), 0.0f, ImDrawFlags_None, 1.0f);
}

static void PushContextMenuExtras()
{
    const ImVec4 ContextMenuPopupBg = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ContextMenuPopupBg);
}

static void PopContextMenuExtras()
{
    ImGui::PopStyleColor(4); // FrameBg, FrameBgHovered, FrameBgActive, ChildBg
    ImGui::PopStyleVar(1);   // FramePadding
}

static void PushContextMenuStyle()
{
    PushMenuChromeStyle();
    PushContextMenuExtras();
}

static void PopContextMenuStyle()
{
    PopContextMenuExtras();
    PopMenuChromeStyle();
}

static void DrawContextMenuFrame()
{
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    DrawMenuFrame();
}

static void DrawSubMenuArrow(ImDrawList* DrawList, const ImVec2& RectMin, const ImVec2& RectMax, float RowHeight, float LabelY)
{
    const float  ArrowY   = RectMin.y + (RowHeight - SubMenuArrowSize) * 0.5f;
    const float  ArrowX   = RectMax.x - MenuContentIndentX - SubMenuArrowSize;
    const ImVec2 ArrowMin = ImVec2(ArrowX, ArrowY);
    const ImVec2 ArrowMax = ImVec2(ArrowX + SubMenuArrowSize, ArrowY + SubMenuArrowSize);

    if (EditorIcons::RightArrowIcon)
    {
        EditorWidgets::DrawIcon(DrawList, EditorIcons::RightArrowIcon, ArrowMin, ArrowMax, ImGui::GetColorU32(ImGuiCol_Text));
    }
    else
    {
        DrawList->AddText(ImVec2(ArrowX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), ">");
    }
}

static bool LoadEditorIconImage(const CHAR* InRelativePath, FEditorIconImage& OutImage)
{
    String FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += InRelativePath;

    FTextureImporterBase Importer;

    const StringView FullPathView(FullPath);

    TSharedRef<FTexture> Texture   = Importer.ImportFromFile(FullPathView);
    FTexture2D*          Texture2D = Texture ? Texture->GetTexture2D() : nullptr;

    if (!Texture2D)
    {
        LOG_ERROR("[EditorIcons]: Failed to load icon '%s'", *FullPath);
        return false;
    }

    const EFormat Format = Texture2D->GetFormat();
    if (Format != EFormat::R8G8B8A8_Unorm)
    {
        LOG_ERROR("[EditorIcons]: Icon '%s' is '%s', and the atlas only takes 'R8G8B8A8_Unorm'", *FullPath, ToString(Format));
        return false;
    }

    const FTextureResourceData* IconData     = Texture2D->GetTextureResourceData();
    const uint8*                SourcePixels = IconData ? reinterpret_cast<const uint8*>(IconData->GetMipData(0)) : nullptr;

    if (!SourcePixels)
    {
        LOG_ERROR("[EditorIcons]: Icon '%s' has no pixel data", *FullPath);
        return false;
    }

    const int32 Width    = static_cast<int32>(Texture2D->GetWidth());
    const int32 Height   = static_cast<int32>(Texture2D->GetHeight());
    const int64 RowBytes = static_cast<int64>(Width) * 4;
    const int64 RowPitch = IconData->GetMipRowPitch(0);

    OutImage.Width  = Width;
    OutImage.Height = Height;
    OutImage.Pixels.ResizeUninitialized(static_cast<int32>(RowBytes * Height));

    for (int32 Y = 0; Y < Height; ++Y)
    {
        Memory::Memcpy(OutImage.Pixels.Data() + Y * RowBytes, SourcePixels + Y * RowPitch, RowBytes);
    }

    OutImage.bValid = true;
    return true;
}

static ImFont* LoadEditorFont(const CHAR* InRelativePath, float SizePixels, const ImFontConfig* FontCfgTemplate = nullptr, const ImWchar* GlyphRanges = nullptr)
{
    String FullPath = Paths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += InRelativePath;

    ImGuiIO& State = ImGui::GetIO();
    return State.Fonts->AddFontFromFileTTF(*FullPath, SizePixels, FontCfgTemplate, GlyphRanges);
}

#if PLATFORM_WINDOWS

// Loads the four caption code points out of the system icon font as a font of their own, so the 
// window buttons are drawn with the very glyphs the OS would have used rather than an imitation 
// of them. E921 ChromeMinimize, E922 ChromeMaximize, E923 ChromeRestore and E8BB ChromeClose.

static ImFont* LoadSystemIconFont()
{
    // Segoe Fluent Icons ships with Windows 11, and Segoe MDL2 assets carries the same code points on Windows 10.
    const CHAR* IconFontPaths[] =
    {
        "C:\\Windows\\Fonts\\SegoeIcons.ttf",
        "C:\\Windows\\Fonts\\segmdl2.ttf",
    };

    // Kept alive past the call because the atlas only reads the ranges when it is built.
    static const ImWchar CaptionGlyphRanges[] = 
    { 
        0xE8BB, 
        0xE8BB, 
        0xE921, 
        0xE923, 
        0
    };

    ImFontConfig FontConfig;
    FontConfig.PixelSnapH = true;

    ImGuiIO& State = ImGui::GetIO();
    for (const CHAR* IconFontPath : IconFontPaths)
    {
        if (!FPlatformFile::IsFile(IconFontPath))
        {
            continue;
        }

        if (ImFont* IconFont = State.Fonts->AddFontFromFileTTF(IconFontPath, SystemIconSizePixels, &FontConfig, CaptionGlyphRanges))
        {
            return IconFont;
        }
    }

    LOG_WARNING("[EditorFonts]: No system icon font found, so the title bar buttons fall back to drawn shapes");
    return nullptr;
}

#endif

float  EditorStyleVars::MainMenuBarHeight                     = 28.0f;
ImVec2 EditorStyleVars::InputFieldFramePadding                = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness             = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding              = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor                 = IM_COL32(100, 136, 234, 255);
ImVec4 EditorStyleVars::InputFieldSelectionColor              = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
ImVec2 EditorStyleVars::SceneHierarchyItemSpacing             = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::SceneHierarchyWindowPadding           = ImVec2(8.0f, 8.0f);
float  EditorStyleVars::SceneHierarchyTableRowHeight          = 32.0f;
ImVec2 EditorStyleVars::PropertiesItemSpacing                 = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesWindowPadding               = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingHeaderItemSpacing = ImVec2(8.0f, 2.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingFramePadding      = ImVec2(10.0f, 8.0f);
float  EditorStyleVars::PropertiesCollapsingFrameRounding     = 2.0f;
float  EditorStyleVars::CheckboxSizeScale                     = 0.8f;
float  EditorStyleVars::PropertyTableLabelIndentX             = 6.0f;
float  EditorStyleVars::PropertyTableCellPaddingX             = 16.0f;
float  EditorStyleVars::ButtonRounding                        = 6.0f;
float  EditorStyleVars::ButtonPaddingX                        = 12.0f;
float  EditorStyleVars::ButtonFramePaddingY                   = 4.0f;
float  EditorStyleVars::ButtonContentGap                      = 6.0f;
float  EditorStyleVars::ButtonArrowSize                       = 14.0f;
ImU32  EditorStyleVars::ButtonBgIdle                          = IM_COL32(56, 56, 56, 255);
ImU32  EditorStyleVars::ButtonBgHovered                       = IM_COL32(87, 87, 87, 255);
ImU32  EditorStyleVars::ButtonBgSelected                      = IM_COL32(9, 92, 176, 255);
ImU32  EditorStyleVars::ButtonBgSelectedHovered               = IM_COL32(15, 110, 205, 255);

bool EditorWidgets::DrawFloat3Control(const CHAR* Label, Vector3& OutValue, float Speed, const Vector3* InRevertValue, EVector3ControlType InType)
{
    ImGuiTable* CurrentTable = ImGui::GetCurrentTable();
    if (!CurrentTable)
    {
        return false;
    }

    ImGuiStyle& Style = ImGui::GetStyle();

    const float AxisGap     = 2.0f;
    const float FrameHeight = ImGui::GetFontSize() + Style.FramePadding.y * 2.0f;
    const float RowHeight   = FrameHeight;

    BeginPropertyTableRow(RowHeight);

    bool bAnyValueChanged = false;
    bool bRowHovered      = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    const bool bIsRotationDegrees = (InType == EVector3ControlType::RotationDegrees);
    const bool bIsScaleControl    = (InType == EVector3ControlType::Scale);

    const auto GetAxisDragSpeed = [&]() -> float
    {
        if (bIsScaleControl)
        {
            return 0.0025f;
        }

        if (InType == EVector3ControlType::Position || bIsRotationDegrees)
        {
            return 1.0f;
        }

        return Speed;
    };

    const auto GetDynamicFormatString = [](float Value, TStaticArray<CHAR, 8>& OutFormat) -> const CHAR*
    {
        static const TStaticArray<float, 4> Pow10Table = { 10.0f, 100.0f, 1000.0f, 10000.0f };

        const float AbsoluteValue = Math::Abs(Value);

        int32 DecimalsToShow = 1;
        for (int32 DecimalIndex = 1; DecimalIndex <= 4; ++DecimalIndex)
        {
            const float ScaleFactor       = Pow10Table[DecimalIndex - 1];
            const float RoundedToDecimals = Math::Round(AbsoluteValue * ScaleFactor) / ScaleFactor;
            const float Tolerance         = 1e-5f * (AbsoluteValue + 1.0f);

            if (Math::Abs(AbsoluteValue - RoundedToDecimals) <= Tolerance)
            {
                DecimalsToShow = DecimalIndex;
                break;
            }

            DecimalsToShow = 4;
        }

        CString::Snprintf(OutFormat.Data(), static_cast<int32>(OutFormat.Size()), "%%.%df", DecimalsToShow);
        return OutFormat.Data();
    };

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::Text("%s", Label);

    bool bUniformScaleEnabled = false;
    if (bIsScaleControl)
    {
        ImGui::SameLine(0.0f, 12.0f);

        ImGui::PushID(Label);

        const ImGuiID UniformScaleKey = ImGui::GetID("UniformScale");

        ImGuiStorage* StateStorage = ImGui::GetStateStorage();
        bUniformScaleEnabled = StateStorage->GetBool(UniformScaleKey, false);

        const bool bUniformScalePrev = bUniformScaleEnabled;

        const float  IconButtonSizePx = 16.0f;
        const ImVec2 IconButtonSize   = ImVec2(IconButtonSizePx, IconButtonSizePx);

        {
            const ImVec2 CursorScreen = ImGui::GetCursorScreenPos();
            const float  CenteredY    = CursorScreen.y + (GPropertyTableState.CurrentRowHeight - IconButtonSizePx) * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(CursorScreen.x, CenteredY));
        }

        const bool bPressed = ImGui::InvisibleButton("##UniformScale", IconButtonSize);

        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            const ImVec2 RectMin = ImGui::GetItemRectMin();
            const ImVec2 RectMax = ImGui::GetItemRectMax();

            const float Pad = 2.0f;
            ImVec2 IconMin = ImVec2(RectMin.x + Pad, RectMin.y + Pad);
            ImVec2 IconMax = ImVec2(RectMax.x - Pad, RectMax.y - Pad);

            const float Alpha01  = Math::Clamp(Style.Alpha, 0.0f, 1.0f);
            const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);
            const ImU32 Tint     = IM_COL32(255, 255, 255, Alpha255);

            const FEditorIcon& Icon = bUniformScaleEnabled ? EditorIcons::LockedIcon : EditorIcons::UnlockedIcon;
            if (Icon)
            {
                EditorWidgets::DrawIcon(DrawList, Icon, IconMin, IconMax, Tint);
            }
            else
            {
                const CHAR* FallbackText = bUniformScaleEnabled ? "L" : "U";

                const ImVec2 TextSize = ImGui::CalcTextSize(FallbackText);
                const ImVec2 TextPos  = ImVec2((RectMin.x + RectMax.x) * 0.5f - TextSize.x * 0.5f, (RectMin.y + RectMax.y) * 0.5f - TextSize.y * 0.5f);
                
                DrawList->AddText(TextPos, ImGui::GetColorU32(ImGuiCol_Text), FallbackText);
            }
        }

        if (bPressed)
        {
            bUniformScaleEnabled = !bUniformScaleEnabled;
        }

        if (bUniformScaleEnabled != bUniformScalePrev)
        {
            StateStorage->SetBool(UniformScaleKey, bUniformScaleEnabled);
        }

        // Keep row hover highlighting behavior (but icon itself does not change appearance)
        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();

        ImGui::PopID();
    }

    BeginPropertyTableValueCell(FrameHeight);
    ImGui::PushID(Label);

    const float AvailableWidth = GetPropertyTableValueWidth();
    const float TotalAxisGaps  = 2.0f * AxisGap;

    float AxisFieldWidth = (AvailableWidth - TotalAxisGaps) / 3.0f;
    AxisFieldWidth = Math::Max(AxisFieldWidth, 1.0f);

    bool bXChanged = false;
    bool bYChanged = false;
    bool bZChanged = false;

    const auto DrawAxisField = [&](const CHAR* DragWidgetId, float& InOutAxisValue, ImU32 AxisIndicatorColor, float FieldWidth, bool bPlaceOnSameLine) -> bool
    {
        if (bPlaceOnSameLine)
        {
            ImGui::SameLine(0.0f, AxisGap);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(AxisGap, 0.0f));
        ImGui::SetNextItemWidth(FieldWidth);

        bool bAxisValueChanged = false;

        TStaticArray<CHAR, 8> FormatBuffer;
        const CHAR* FormatString = GetDynamicFormatString(InOutAxisValue, FormatBuffer);

        const float AxisDragSpeed = GetAxisDragSpeed();
        bAxisValueChanged |= ImGui::DragFloat(DragWidgetId, &InOutAxisValue, AxisDragSpeed, 0.0f, 0.0f, FormatString, ImGuiSliderFlags_NoRoundToFormat);

        if (bIsRotationDegrees && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            const float RoundedDegrees = Math::Round(InOutAxisValue);
            if (RoundedDegrees != InOutAxisValue)
            {
                InOutAxisValue    = RoundedDegrees;
                bAxisValueChanged = true;
            }
        }

        const bool bIsItemActive  = ImGui::IsItemActive();
        const bool bIsItemHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        const ImGuiID LastItemId = ImGui::GetItemID();

        const bool bIsTempInputActive = ImGui::TempInputIsActive(LastItemId);
        if (!bIsTempInputActive)
        {
            TStaticArray<CHAR, 8> FormatBufferAfter;
            const CHAR* FormatStringAfter = GetDynamicFormatString(InOutAxisValue, FormatBufferAfter);

            TStaticArray<CHAR, 64> ValueTextBuffer;
            CString::Snprintf(ValueTextBuffer.Data(), static_cast<int32>(ValueTextBuffer.Size()), FormatStringAfter, InOutAxisValue);

            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window && !Window->SkipItems)
            {
                const ImVec2 ItemMin         = ImGui::GetItemRectMin();
                const ImVec2 ItemMax         = ImGui::GetItemRectMax();
                const ImU32  BackgroundColor = ImGui::GetColorU32(bIsItemActive ? ImGuiCol_FrameBgActive : (bIsItemHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));

                Window->DrawList->AddRectFilled(ItemMin, ItemMax, BackgroundColor, ImGui::GetStyle().FrameRounding);

                const float  TextStartX    = ItemMin.x + ImGui::GetStyle().FramePadding.x;
                const ImVec2 ValueTextSize = ImGui::CalcTextSize(ValueTextBuffer.Data());
                const float  TextPosY      = ItemMin.y + (ItemMax.y - ItemMin.y - ValueTextSize.y) * 0.5f;

                Window->DrawList->AddText(ImVec2(TextStartX, TextPosY), ImGui::GetColorU32(ImGuiCol_Text), ValueTextBuffer.Data());

                if (bIsRotationDegrees)
                {
                    const float DegreeGap = 1.0f;
                    const float DegreePosX  = TextStartX + ValueTextSize.x + DegreeGap;

                    Window->DrawList->AddText(ImVec2(DegreePosX, TextPosY), ImGui::GetColorU32(ImGuiCol_Text), "\xC2\xB0");
                }
            }
        }

        DrawInputBorderLastItem(ImGui::GetStyle().FrameRounding);
        DrawAxisLineForLastItem(AxisIndicatorColor);

        if (bIsRotationDegrees && bIsTempInputActive)
        {
            DrawSuffixAfterTempInputText(LastItemId, "\xC2\xB0", ImGui::GetColorU32(ImGuiCol_Text));
        }

        bRowHovered |= bIsItemHovered;
        bRowHovered |= bIsItemActive;

        ImGui::PopStyleVar();

        bAnyValueChanged |= bAxisValueChanged;
        return bAxisValueChanged;
    };

    const ImU32 XAxisColor = ImGui::ColorConvertFloat4ToU32(EditorAxisColors::X);
    const ImU32 YAxisColor = ImGui::ColorConvertFloat4ToU32(EditorAxisColors::Y);
    const ImU32 ZAxisColor = ImGui::ColorConvertFloat4ToU32(EditorAxisColors::Z);

    bXChanged = DrawAxisField("##X", OutValue.X, XAxisColor, AxisFieldWidth, false);
    bYChanged = DrawAxisField("##Y", OutValue.Y, YAxisColor, AxisFieldWidth, true);
    bZChanged = DrawAxisField("##Z", OutValue.Z, ZAxisColor, AxisFieldWidth, true);

    if (bIsScaleControl && bUniformScaleEnabled)
    {
        if (bXChanged || bYChanged || bZChanged)
        {
            const float NewUniformScale = bXChanged ? OutValue.X : (bYChanged ? OutValue.Y : OutValue.Z);

            bool bAppliedUniformScale = false;

            if (OutValue.X != NewUniformScale)
            {
                OutValue.X = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (OutValue.Y != NewUniformScale)
            {
                OutValue.Y = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (OutValue.Z != NewUniformScale)
            {
                OutValue.Z = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (bAppliedUniformScale)
            {
                bAnyValueChanged = true;
            }
        }
    }

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bHasRevertValue         = InRevertValue != nullptr;
    const bool bShouldShowRevertButton = bHasRevertValue && (OutValue != *InRevertValue);

    const float RevertSlotSize = FrameHeight;
    if (bShouldShowRevertButton)
    {
        if (ResetIconButton())
        {
            OutValue = *InRevertValue;
            bAnyValueChanged = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(RevertSlotSize, RevertSlotSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bAnyValueChanged;
}

bool EditorWidgets::DrawFloatProperty(const CHAR* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float ContentHeight = ImGui::GetFrameHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bResult     = false;
    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    SetPropertyTableValueItemWidth();
    if (bUseSlider)
    {
        bResult = ImGui::SliderFloat("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bResult = ImGui::DragFloat("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
    }

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawIntProperty(const CHAR* Label, int32& InOutValue, float Speed, int32 MinValue, int32 MaxValue, const CHAR* Format, bool bUseSlider, const int32* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float ContentHeight = ImGui::GetFrameHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bResult     = false;
    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    SetPropertyTableValueItemWidth();
    if (bUseSlider)
    {
        bResult = ImGui::SliderInt("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bResult = ImGui::DragInt("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
    }

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawComboProperty(const CHAR* Label, int32& InOutValue, const CHAR* const* Items, int32 ItemCount, const int32* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float ContentHeight = ImGui::GetFrameHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bResult     = false;
    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    const ImU32 FieldBgU32     = IM_COL32(15, 15, 15, 255);
    const ImU32 FieldTextU32   = IM_COL32(170, 170, 170, 255);
    const ImU32 ArrowU32       = IM_COL32(192, 192, 192, 255);
    const ImU32 PopupBgU32     = IM_COL32(26, 26, 26, 255);
    const ImU32 PopupBorderU32 = IM_COL32(0, 90, 173, 255);
    const ImU32 SelectBgU32    = IM_COL32(0, 112, 224, 255);
    const ImU32 PopupTextU32   = IM_COL32(255, 255, 255, 255);

    const ImGuiStyle& Style = ImGui::GetStyle();
    const float FieldRounding = Style.FrameRounding;
    
    constexpr float PopupRounding = 0.0f;

    const int32  ClampedIndex = Math::Clamp<int32>(InOutValue, 0, Math::Max(0, ItemCount - 1));
    const CHAR*  PreviewText  = (Items && ItemCount > 0) ? Items[ClampedIndex] : "";
    const float  FrameHeight  = ImGui::GetFrameHeight();
    const ImVec2 FieldSize    = ImVec2(GetPropertyTableValueWidth(), FrameHeight);

    const bool bPressed      = ImGui::InvisibleButton("##Value", FieldSize);
    const bool bFieldHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool bFieldActive  = ImGui::IsItemActive();

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    const ImVec2 FieldMin = ImGui::GetItemRectMin();
    const ImVec2 FieldMax = ImGui::GetItemRectMax();

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (Window && !Window->SkipItems)
    {
        Window->DrawList->AddRectFilled(FieldMin, FieldMax, FieldBgU32, FieldRounding);

        const float  ArrowZoneW     = FrameHeight;
        const ImVec2 TextSize       = ImGui::CalcTextSize(PreviewText);
        const float  TextY          = FieldMin.y + (FrameHeight - TextSize.y) * 0.5f;
        const float  ArrowPadX      = 8.0f;
        const float  TextStartX     = FieldMin.x + Style.FramePadding.x;
        const float  TextMaxX       = FieldMax.x - ArrowZoneW - ArrowPadX;
        const ImVec2 ClipMin        = ImVec2(TextStartX, FieldMin.y);
        const ImVec2 ClipMax        = ImVec2(TextMaxX, FieldMax.y);
        const ImU32  PreviewTextU32 = (bFieldHovered || bFieldActive) ? IM_COL32(255, 255, 255, 255) : FieldTextU32;
        const ImU32  ArrowColorU32  = (bFieldHovered || bFieldActive) ? IM_COL32(255, 255, 255, 255) : ArrowU32;

        Window->DrawList->PushClipRect(ClipMin, ClipMax, true);
        Window->DrawList->AddText(ImVec2(TextStartX, TextY), PreviewTextU32, PreviewText);
        Window->DrawList->PopClipRect();

        const float ArrowSize = 6.0f;
        const float CenterX   = FieldMax.x - ArrowZoneW * 0.5f;
        const float CenterY   = (FieldMin.y + FieldMax.y) * 0.5f + 1.0f;
        const ImVec2 P0       = ImVec2(CenterX - ArrowSize, CenterY - ArrowSize * 0.5f);
        const ImVec2 P1       = ImVec2(CenterX + ArrowSize, CenterY - ArrowSize * 0.5f);
        const ImVec2 P2       = ImVec2(CenterX,            CenterY + ArrowSize * 0.6f);
        Window->DrawList->AddTriangleFilled(P0, P1, P2, ArrowColorU32);
    }

    DrawInputBorderLastItem(FieldRounding);

    if (bPressed && bEnabled)
    {
        ImGui::OpenPopup("##ComboPopup");
    }

    constexpr float PopupTextPadX = 6.0f;

    if (ImGui::IsPopupOpen("##ComboPopup"))
    {
        float PopupWidth = FieldSize.x;
        if (Items && ItemCount > 0)
        {
            float MaxItemTextWidth = 0.0f;
            for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
            {
                const ImVec2 TextSize = ImGui::CalcTextSize(Items[ItemIndex] ? Items[ItemIndex] : "");
                MaxItemTextWidth = Math::Max(MaxItemTextWidth, TextSize.x);
            }

            PopupWidth = Math::Max(PopupWidth, MaxItemTextWidth + PopupTextPadX * 2.0f);
        }

        const ImRect Outer = GetPopupExtentRect(ImVec2(FieldMin.x, FieldMax.y));
        PopupWidth = Math::Min(PopupWidth, Math::Max(1.0f, Outer.GetWidth()));

        FPopupAnchor ComboAnchor;
        ComboAnchor.Min = FieldMin;
        ComboAnchor.Max = FieldMax;

        ImGui::SetNextWindowSize(ImVec2(PopupWidth, 0.0f));
        SetNextBeginPopupPos("##ComboPopup", ComboAnchor, EPopupPlacement::BelowAnchor, ImVec2(PopupWidth, 0.0f));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, PopupRounding);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(PopupBgU32));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(PopupTextU32));
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(PopupBgU32));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(SelectBgU32));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(SelectBgU32));

    if (ImGui::BeginPopup("##ComboPopup"))
    {
        ImGuiWindow* PopupWindow   = ImGui::GetCurrentWindow();
        const float  ItemHeight    = ImGui::GetFrameHeight();
        const float  FullWidth     = PopupWindow ? PopupWindow->Size.x : ImGui::GetContentRegionAvail().x;
        ImDrawList*  PopupDrawList = PopupWindow ? PopupWindow->DrawList : ImGui::GetWindowDrawList();
        
        constexpr float SelectedInsetX   = 2.0f;
        constexpr float SelectedInsetY   = 1.0f;
        constexpr float SelectedRounding = 4.0f;

        for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
        {
            const bool bSelected = (ItemIndex == InOutValue);

            ImGui::PushID(ItemIndex);
            ImGui::SetCursorPosX(0.0f);

            const bool bItemPressed = ImGui::InvisibleButton("##Item", ImVec2(FullWidth, ItemHeight));
            const bool bItemHovered = ImGui::IsItemHovered();
            const bool bItemHeld    = ImGui::IsItemActive();

            const ImVec2 ItemMin = ImGui::GetItemRectMin();
            const ImVec2 ItemMax = ImGui::GetItemRectMax();

            if ((bItemHovered || bItemHeld) && PopupDrawList)
            {
                PopupDrawList->AddRectFilled(ItemMin, ItemMax, SelectBgU32, 0.0f);
            }

            const CHAR*  ItemText     = (Items && ItemIndex < ItemCount) ? Items[ItemIndex] : "";
            const ImVec2 ItemTextSize = ImGui::CalcTextSize(ItemText);
            const float  ItemTextY    = ItemMin.y + (ItemHeight - ItemTextSize.y) * 0.5f;
            const float  ItemTextX    = ItemMin.x + PopupTextPadX;

            if (PopupDrawList)
            {
                PopupDrawList->AddText(ImVec2(ItemTextX, ItemTextY), PopupTextU32, ItemText);
            }

            if (bSelected && PopupDrawList)
            {
                const ImVec2 SelMin = ImVec2(ItemMin.x + SelectedInsetX, ItemMin.y + SelectedInsetY);
                const ImVec2 SelMax = ImVec2(ItemMax.x - SelectedInsetX, ItemMax.y - SelectedInsetY);
                PopupDrawList->AddRect(SelMin, SelMax, SelectBgU32, SelectedRounding, 0, 1.5f);
            }

            if (bItemPressed)
            {
                InOutValue = ItemIndex;
                bResult    = true;

                ImGui::CloseCurrentPopup();
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::PopID();
        }

        if (PopupWindow && !PopupWindow->SkipItems)
        {
            const ImVec2 PopupMin = PopupWindow->Pos;
            const ImVec2 PopupMax = ImVec2(PopupWindow->Pos.x + PopupWindow->Size.x, PopupWindow->Pos.y + PopupWindow->Size.y);
            PopupWindow->DrawList->AddRect(PopupMin, PopupMax, PopupBorderU32, PopupRounding, 0, 1.0f);
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }
        
        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawCheckbox(const CHAR* Label, bool& InOutValue, bool bEnabled)
{
    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        if (!bEnabled)
        {
            ImGui::EndDisabled();
        }

        return false;
    }

    ImGuiContext&     Context          = *GImGui;
    const ImGuiStyle& Style            = Context.Style;
    const ImGuiID     Id               = Window->GetID(Label);
    const float       FrameHeight      = ImGui::GetFrameHeight();
    const float       CheckboxSize     = FrameHeight * EditorStyleVars::CheckboxSizeScale;
    const float       OffsetY          = (FrameHeight - CheckboxSize) * 0.5f;
    const bool        bHasVisibleLabel = Label && !(Label[0] == '#' && Label[1] == '#');
    const ImVec2      LabelSize        = bHasVisibleLabel ? ImGui::CalcTextSize(Label, nullptr, true) : ImVec2(0.0f, 0.0f);
    const ImVec2      Pos              = Window->DC.CursorPos;
    const float       TotalWidth       = CheckboxSize + (bHasVisibleLabel ? Style.ItemInnerSpacing.x + LabelSize.x : 0.0f);
    const ImRect      TotalBB          = ImRect(Pos, Pos + ImVec2(TotalWidth, FrameHeight));

    ImGui::ItemSize(TotalBB, Style.FramePadding.y);
    if (!ImGui::ItemAdd(TotalBB, Id))
    {
        if (!bEnabled)
        {
            ImGui::EndDisabled();
        }

        return false;
    }

    bool bHovered = false;
    bool bHeld    = false;

    const bool bPressed = ImGui::ButtonBehavior(TotalBB, Id, &bHovered, &bHeld);

    bool bChanged = false;
    if (bPressed && bEnabled)
    {
        InOutValue = !InOutValue;
        bChanged   = true;

        ImGui::MarkItemEdited(Id);
    }

    const ImVec2 CheckMin = ImVec2(Pos.x, Pos.y + OffsetY);
    const ImVec2 CheckMax = CheckMin + ImVec2(CheckboxSize, CheckboxSize);
    const ImRect CheckBB  = ImRect(CheckMin, CheckMax);

    static const ImVec4 CheckboxBackgroundColor = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
    static const ImVec4 CheckboxCheckMarkColor  = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);

    const ImU32 FrameBg  = ImGui::GetColorU32(CheckboxBackgroundColor);
    const ImU32 CheckCol = ImGui::GetColorU32(CheckboxCheckMarkColor);

    ImGui::RenderNavHighlight(TotalBB, Id);
    ImGui::RenderFrame(CheckBB.Min, CheckBB.Max, FrameBg, true, Style.FrameRounding);

    if (InOutValue)
    {
        const float Pad = Math::Max(1.0f, IM_TRUNC(CheckboxSize / 6.0f));
        EditorWidgets::DrawCheckMark(Window->DrawList, CheckBB.Min + ImVec2(Pad, Pad), CheckCol, CheckboxSize - Pad * 2.0f);
    }

    DrawInputBorderRect(CheckBB.Min, CheckBB.Max, bHeld && bHovered, bHovered, Style.FrameRounding);

    if (bHasVisibleLabel)
    {
        const ImVec2 LabelPos = ImVec2(CheckBB.Max.x + Style.ItemInnerSpacing.x, Pos.y + Style.FramePadding.y);
        ImGui::RenderText(LabelPos, Label);
    }

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    return bEnabled && bChanged;
}

bool EditorWidgets::DrawCheckboxProperty(const CHAR* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float ContentHeight = ImGui::GetFrameHeight();

    BeginPropertyTableRow(ContentHeight);

    bool bResult     = false;
    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);
    ImGui::PushID(Label);

    bResult = EditorWidgets::DrawCheckbox("##Value", InOutValue, bEnabled);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawTextProperty(const CHAR* Label, const CHAR* ValueText)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Text("%s: %s", Label, ValueText ? ValueText : "");
        return false;
    }

    const float ContentHeight = ImGui::GetTextLineHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ContentHeight);
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(ValueText ? ValueText : "");
    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
    return bRowHovered;
}

void EditorWidgets::DrawTextureProperty(const CHAR* Label, ImTextureID Texture, float PreviewSize)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return;
    }

    BeginPropertyTableRow(PreviewSize);

    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    if (Texture)
    {
        BeginPropertyTableValueCell(PreviewSize);

        ImGui::Image(Texture, ImVec2(PreviewSize, PreviewSize));

        ImGuiStyle& Style = ImGui::GetStyle();
        DrawInputBorderLastItem(Style.FrameRounding);

        const bool bPreviewHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= bPreviewHovered;

        if (bPreviewHovered && ImGui::BeginTooltip())
        {
            const float ZoomedSize = 256.0f;
            ImGui::Image(Texture, ImVec2(ZoomedSize, ZoomedSize));
            ImGui::EndTooltip();
        }
    }
    else
    {
        BeginPropertyTableValueCell(ImGui::GetFrameHeight());
        ImGui::TextDisabled("None");
        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
}

void EditorWidgets::DrawReadOnlyFloat3Property(const CHAR* Label, const Vector3& Value)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Text("%s: %.3f %.3f %.3f", Label, Value.X, Value.Y, Value.Z);
        return;
    }

    const float ContentHeight = ImGui::GetFrameHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);

    TStaticArray<float, 3> Temp = { Value.X, Value.Y, Value.Z };
    SetPropertyTableValueItemWidth();
    ImGui::InputFloat3("##Value", Temp.Data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
}

bool EditorWidgets::DrawColor3Property(const CHAR* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float ContentHeight = ImGui::GetFrameHeight();
    BeginPropertyTableRow(ContentHeight);

    bool bResult     = false;
    bool bRowHovered = IsRowHovered(GPropertyTableState.CurrentRowHeight);

    BeginPropertyTableLabelCell(ImGui::GetTextLineHeight());
    ImGui::TextUnformatted(Label);

    BeginPropertyTableValueCell(ContentHeight);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    ImGuiStyle& Style = ImGui::GetStyle();
    const float Gap         = 4.0f;
    const float FrameHeight = ImGui::GetFrameHeight();

    const float Avail       = GetPropertyTableValueWidth();
    const float ButtonWidth = FrameHeight;
    const float TotalGaps   = 3.0f * Gap;

    float FieldWidth = (Avail - ButtonWidth - TotalGaps) / 3.0f;
    FieldWidth = ImMax(FieldWidth, 1.0f);

    {
        const ImVec4 Color = ImVec4(InOutColor[0], InOutColor[1], InOutColor[2], 1.0f);

        const ImGuiColorEditFlags ButtonFlags = Flags | ImGuiColorEditFlags_NoTooltip;
        if (ImGui::ColorButton("##ColorBtn", Color, ButtonFlags, ImVec2(ButtonWidth, FrameHeight)))
        {
            ImGui::OpenPopup("##ColorPicker");
        }

        DrawInputBorderLastItem(Style.FrameRounding);

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();

        if (ImGui::BeginPopup("##ColorPicker"))
        {
            bResult |= ImGui::ColorPicker3("##Picker", InOutColor, Flags);
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine(0.0f, Gap);

    // R / G / B drag floats (0..1)
    const auto DrawComponentDragFloat = [&](const CHAR* Id, float& OutValue, bool bSameLine)
    {
        if (bSameLine)
        {
            ImGui::SameLine(0.0f, Gap);
        }

        ImGui::SetNextItemWidth(FieldWidth);
        bResult |= ImGui::DragFloat(Id, &OutValue, 0.01f, 0.0f, 1.0f, "%.3f");

        ImGuiStyle& Style = ImGui::GetStyle();
        DrawInputBorderLastItem(Style.FrameRounding);

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();
    };

    DrawComponentDragFloat("##R", InOutColor[0], false);
    DrawComponentDragFloat("##G", InOutColor[1], true);
    DrawComponentDragFloat("##B", InOutColor[2], true);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    BeginPropertyTableRevertCell();

    const bool bCanRevert = (InRevertColor != nullptr);
    if (!bCanRevert || !bEnabled)
    {
        ImGui::BeginDisabled();
    }

    if (ResetIconButton() && bCanRevert)
    {
        static constexpr uint64 SizeInBytes = sizeof(TStaticArray<float, 3>);
        Memory::Memcpy(InOutColor, InRevertColor, SizeInBytes);
        bResult = true;
    }

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (!bCanRevert || !bEnabled)
    {
        ImGui::EndDisabled();
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

const CHAR* EditorHelpers::GetTrimmedQuery(const CHAR* InText, CHAR* OutBuf, int32 OutBufSize)
{
    if (!OutBuf || OutBufSize <= 0)
    {
        return nullptr;
    }

    OutBuf[0] = 0;

    if (!InText)
    {
        return nullptr;
    }

    const CHAR* Start = InText;
    while (*Start && CharTraits::IsWhitespace(*Start))
    {
        ++Start;
    }

    const CHAR* End = Start;
    while (*End)
    {
        ++End;
    }

    while (End > Start && CharTraits::IsWhitespace(End[-1]))
    {
        --End;
    }

    const int32 Len = static_cast<int32>(End - Start);
    if (Len <= 0)
    {
        return nullptr;
    }

    if (*End == 0)
    {
        return Start;
    }

    const int32 CopyLen = Math::Min(Len, OutBufSize - 1);
    CString::Strncpy(OutBuf, Start, CopyLen + 1);
    OutBuf[CopyLen] = 0;
    return OutBuf[0] ? OutBuf : nullptr;
}

void EditorHelpers::FormatBytes(int64 Bytes, char* OutBuffer, int32 BufferSize)
{
    const double AbsBytes = static_cast<double>(Bytes < 0 ? -Bytes : Bytes);
    if (AbsBytes >= 1024.0 * 1024.0 * 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f GB", static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0));
    }
    else if (AbsBytes >= 1024.0 * 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f MB", static_cast<double>(Bytes) / (1024.0 * 1024.0));
    }
    else if (AbsBytes >= 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f KB", static_cast<double>(Bytes) / 1024.0);
    }
    else
    {
        snprintf(OutBuffer, BufferSize, "%lld B", static_cast<long long>(Bytes));
    }
}

ImVec2 EditorWidgets::GetDefaultEditorWindowSize()
{
    const ImVec2 ViewportSize = ImGuiExtensions::GetMainViewportSize();

    const float Width  = Math::Clamp(ViewportSize.x * EditorDefaultWindowWidthFraction,
        EditorDefaultWindowMinWidth, EditorDefaultWindowMaxWidth);

    const float Height = Math::Clamp(ViewportSize.y * EditorDefaultWindowHeightFraction,
        EditorDefaultWindowMinHeight, EditorDefaultWindowMaxHeight);

    return ImVec2(Width, Height);
}

ImVec2 EditorWidgets::ScaleEditorWindowSize(const ImVec2& LogicalSize)
{
    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();
    const float  Scale            = FrameBufferScale.x;
    return ImVec2(LogicalSize.x * Scale, LogicalSize.y * Scale);
}

bool EditorWidgets::BeginEditorWindow(const CHAR* Title, bool* pbVisible, ImGuiWindowFlags ExtraFlags)
{
    GEditorWindowBegun               = false;
    GEditorWindowOpen                = false;
    GEditorWindowChildBegun          = false;
    GEditorWindowOuterPaddingPushed  = false;
    GEditorWindowContentStylePushed  = false;
    GEditorWindowNestedChildBgPushed = false;

    const ImGuiCond SizeCondition = (ExtraFlags & ImGuiWindowFlags_NoSavedSettings)
        ? ImGuiCond_Appearing
        : ImGuiCond_FirstUseEver;

    ImGuiContext& g = *GImGui;
    if (!(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize))
    {
        ImGui::SetNextWindowSize(GetDefaultEditorWindowSize(), SizeCondition);
    }

    const ImGuiStyle& Style = ImGui::GetStyle();

    const ImVec2 ContentPadding = Style.WindowPadding;
    const ImVec4 ContentBg      = Style.Colors[ImGuiCol_WindowBg];
    const ImVec4 NestedChildBg  = Style.Colors[ImGuiCol_ChildBg];

    ImGuiWindow* ExistingWindow = ImGui::FindWindowByName(Title);
    const bool bFloating        = IsWindowFloating(ExistingWindow);

    const ImVec2 OuterPadding = bFloating
        ? ImVec2(EditorWindowBorderInset, EditorWindowBorderInset)
        : ImVec2(0.0f, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, OuterPadding);
    GEditorWindowOuterPaddingPushed = true;

    const ImGuiWindowFlags OuterFlags =
        ExtraFlags |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    GEditorWindowOpen  = ImGui::Begin(Title, pbVisible, OuterFlags);
    GEditorWindowBegun = true;
    if (!GEditorWindowOpen)
    {
        return false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ContentPadding);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ContentBg);
    GEditorWindowContentStylePushed = true;

    const ImGuiWindowFlags ScrollFlags =
        ExtraFlags & (ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse |
                      ImGuiWindowFlags_AlwaysVerticalScrollbar |
                      ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    // A borderless child drops its padding unless it is asked to keep it.
    const bool bChildOpen = ImGui::BeginChild("##EditorContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ScrollFlags);
    GEditorWindowChildBegun = true;

    // Only the content child takes the panel background; children the panel creates keep the one they had.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, NestedChildBg);
    GEditorWindowNestedChildBgPushed = true;

    return bChildOpen;
}

void EditorWidgets::EndEditorWindow()
{
    if (GEditorWindowNestedChildBgPushed)
    {
        ImGui::PopStyleColor();
        GEditorWindowNestedChildBgPushed = false;
    }

    if (GEditorWindowChildBegun)
    {
        ImGui::EndChild();
        GEditorWindowChildBegun = false;
    }

    if (GEditorWindowContentStylePushed)
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        GEditorWindowContentStylePushed = false;
    }

    if (GEditorWindowOpen)
    {
        ImGuiWindow* OuterWindow = ImGui::GetCurrentWindow();
        if (IsWindowFloating(OuterWindow))
        {
            const ImRect OuterRect(OuterWindow->Pos, OuterWindow->Pos + OuterWindow->Size);
            ImDrawList* DrawList = OuterWindow->DrawList;

            DrawList->PushClipRect(OuterRect.Min, OuterRect.Max, false);
            DrawFloatingWindowBorder(DrawList, OuterRect.Min, OuterRect.Max);
            DrawList->PopClipRect();
        }

        GEditorWindowOpen = false;
    }

    if (GEditorWindowBegun)
    {
        ImGui::End();
        GEditorWindowBegun = false;
    }

    if (GEditorWindowOuterPaddingPushed)
    {
        ImGui::PopStyleVar();
        GEditorWindowOuterPaddingPushed = false;
    }
}

void EditorWidgets::DrawErrorWindow(FErrorWindowContext& InOutContext)
{
    if (!InOutContext.bVisible)
    {
        return;
    }

    const CHAR* TitleText  = !InOutContext.Title.IsEmpty() ? *InOutContext.Title : "Failed Renames";
    const CHAR* HeaderText = !InOutContext.HeaderText.IsEmpty() ? *InOutContext.HeaderText : "The following files could not be moved";

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        ImGui::SetNextWindowPos(Viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    }

    ImGui::SetNextWindowSize(ImVec2(780.0f, 420.0f), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(44, 44, 44, 255));

    const ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    bool bClosePressed = false;
    const bool bBeginWindow = ImGui::Begin(TitleText, nullptr, WindowFlags);
    if (bBeginWindow)
    {
        ImGui::TextUnformatted(HeaderText);
        ImGui::Spacing();
        ImGui::Spacing();

        const float LineHeight     = ImGui::GetTextLineHeightWithSpacing();
        const float MinHeight      = 140.0f;
        const float EntriesHeight  = LineHeight * InOutContext.Entries.Size();
        const float DesiredHeight  = Math::Clamp(MinHeight + EntriesHeight, 140.0f, 260.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(26, 26, 26, 255));

        const ImGuiChildFlags ChildFlags = ImGuiChildFlags_AlwaysUseWindowPadding;
        ImGui::BeginChild("##FailedMoveList", ImVec2(0.0f, DesiredHeight), ChildFlags, ImGuiWindowFlags_None);

        if (InOutContext.Entries.IsEmpty())
        {
            ImGui::TextUnformatted("No files listed.");
        }
        else
        {
            for (int32 Index = 0; Index < InOutContext.Entries.Size(); ++Index)
            {
                ImGui::TextUnformatted(*InOutContext.Entries[Index]);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        const ImVec2 ListMin = ImGui::GetItemRectMin();
        const ImVec2 ListMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(ListMin, ListMax, IM_COL32(36, 36, 36, 255), 0.0f, ImDrawFlags_None, 3.0f);

        ImGui::Spacing();
        ImGui::Spacing();

        const float ButtonWidth  = 120.0f;
        const float ButtonHeight = ImGui::GetFrameHeight();
        const float AvailableX   = ImGui::GetContentRegionAvail().x;
        const float CursorX      = ImGui::GetCursorPosX();
        const float ButtonPosX   = CursorX + Math::Max(0.0f, AvailableX - ButtonWidth);
        ImGui::SetCursorPosX(ButtonPosX);

        const ImVec2 ButtonSize(ButtonWidth, ButtonHeight);
        bClosePressed = DrawDialogButton("Close", ButtonSize);
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    if (bClosePressed)
    {
        InOutContext.bVisible = false;
        InOutContext.Entries.Clear();
        InOutContext.HeaderText.Clear();
        InOutContext.Title.Clear();
    }
}

bool EditorWidgets::DrawConfirmDialog(FConfirmDialogContext& InOutContext)
{
    if (!InOutContext.bVisible)
    {
        return false;
    }

    const CHAR* TitleText   = !InOutContext.Title.IsEmpty() ? *InOutContext.Title : "Confirm";
    const CHAR* MessageText = !InOutContext.Message.IsEmpty() ? *InOutContext.Message : "Are you sure?";

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        // Use Appearing so the window is centered when opened but can be moved by the user
        ImGui::SetNextWindowPos(Viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    // Set minimum width only; AlwaysAutoResize handles the rest
    ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    // Match error window style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(44, 44, 44, 255));

    const ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;

    bool bConfirmed = false;

    if (ImGui::Begin(TitleText, &InOutContext.bVisible, WindowFlags))
    {
        const float ButtonWidth  = 100.0f;
        const float ButtonHeight = ImGui::GetFrameHeight();
        const float Gap          = 12.0f;

        const float AvailableX = ImGui::GetContentRegionAvail().x;

        // Center the message text on the x-axis
        const ImVec2 MessageTextSize = ImGui::CalcTextSize(MessageText, nullptr, true, AvailableX);
        const float MessageStartX   = ImGui::GetCursorPosX() + Math::Max(0.0f, (AvailableX - MessageTextSize.x) * 0.5f);
        ImGui::SetCursorPosX(MessageStartX);

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + AvailableX);
        ImGui::TextUnformatted(MessageText);
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Spacing();

        const float TwoButtonsWidth = ButtonWidth * 2.0f + Gap;
        const float StartX          = ImGui::GetCursorPosX() + Math::Max(0.0f, (AvailableX - TwoButtonsWidth) * 0.5f);

        ImGui::SetCursorPosX(StartX);
        if (DrawDialogButton("Yes", ImVec2(ButtonWidth, ButtonHeight)))
        {
            bConfirmed = true;
            InOutContext.bVisible = false;
        }
        ImGui::SameLine(0.0f, Gap);
        if (DrawDialogButton("No", ImVec2(ButtonWidth, ButtonHeight)))
        {
            InOutContext.bVisible = false;
        }
    }
    else
    {
        InOutContext.bVisible = false;
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    return bConfirmed;
}

ImVec2 EditorWidgets::GetButtonSize(const CHAR* Label, const ImVec2& Size)
{
    ImVec2 Result = Size;

    if (Result.x <= 0.0f)
    {
        const float LabelWidth = Label ? ImGui::CalcTextSize(Label, nullptr, true).x : 0.0f;
        Result.x = LabelWidth + EditorStyleVars::ButtonPaddingX * 2.0f;
    }

    if (Result.y <= 0.0f)
    {
        Result.y = GetButtonHeight();
    }

    return Result;
}

bool EditorWidgets::DrawButton(const CHAR* Label, const ImVec2& Size, bool bSelected, ImDrawFlags Corners)
{
    if (!Label)
    {
        return false;
    }

    const ImVec2 ButtonSize = GetButtonSize(Label, Size);

    const bool bPressed = ImGui::InvisibleButton(Label, ButtonSize);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bHeld    = ImGui::IsItemActive();

    const ImVec2 Min = ImGui::GetItemRectMin();
    const ImVec2 Max = ImGui::GetItemRectMax();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // Routed through GetColorU32 so the button fades along with its label while it is disabled
    const ImU32 Background = GetButtonBackground(bSelected, bHovered, bHeld);
    DrawList->AddRectFilled(Min, Max, ImGui::GetColorU32(Background), EditorStyleVars::ButtonRounding, Corners);

    const CHAR*  LabelEnd = ImGui::FindRenderedTextEnd(Label);
    const ImVec2 TextSize = ImGui::CalcTextSize(Label, LabelEnd);
    const ImVec2 TextPos  = ImVec2(Min.x + (ButtonSize.x - TextSize.x) * 0.5f, Min.y + (ButtonSize.y - TextSize.y) * 0.5f);

    DrawList->AddText(TextPos, ImGui::GetColorU32(ImGuiCol_Text), Label, LabelEnd);

    return bPressed;
}

bool EditorWidgets::DrawDropdownButton(const CHAR* InId, const CHAR* Label, const ImVec2& Size, bool bPopupOpen, FPopupAnchor& OutAnchor, bool& bOutHovered)
{
    OutAnchor.bRequestPosition = false;
    bOutHovered                = false;

    if (!InId)
    {
        return false;
    }

    const float  ContentGap = EditorStyleVars::ButtonContentGap;
    const float  ArrowSize  = EditorStyleVars::ButtonArrowSize;
    const CHAR*  LabelEnd   = Label ? ImGui::FindRenderedTextEnd(Label) : nullptr;
    const ImVec2 TextSize   = Label ? ImGui::CalcTextSize(Label, LabelEnd) : ImVec2(0.0f, 0.0f);

    ImVec2 ButtonSize = Size;
    if (ButtonSize.x <= 0.0f)
    {
        ButtonSize.x = EditorStyleVars::ButtonPaddingX * 2.0f + TextSize.x + ContentGap + ArrowSize;
    }

    if (ButtonSize.y <= 0.0f)
    {
        ButtonSize.y = GetButtonHeight();
    }

    const bool bPressed = ImGui::InvisibleButton(InId, ButtonSize);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bHeld    = ImGui::IsItemActive();

    const ImVec2 Min = ImGui::GetItemRectMin();
    const ImVec2 Max = ImGui::GetItemRectMax();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImU32 Background = (bPopupOpen || bHovered || bHeld) ? EditorStyleVars::ButtonBgHovered : EditorStyleVars::ButtonBgIdle;
    DrawList->AddRectFilled(Min, Max, ImGui::GetColorU32(Background), EditorStyleVars::ButtonRounding);

    const ImU32 ContentColor = ImGui::GetColorU32(ImGuiCol_Text);

    if (Label)
    {
        const float TextX = Min.x + EditorStyleVars::ButtonPaddingX;
        const float TextY = Min.y + (ButtonSize.y - TextSize.y) * 0.5f;

        DrawList->AddText(ImVec2(TextX, TextY), ContentColor, Label, LabelEnd);
    }

    if (EditorIcons::DownArrowIcon)
    {
        const float  ArrowX   = Max.x - EditorStyleVars::ButtonPaddingX - ArrowSize;
        const float  ArrowY   = Min.y + (ButtonSize.y - ArrowSize) * 0.5f;
        const ImVec2 ArrowMin = ImVec2(ArrowX, ArrowY);
        const ImVec2 ArrowMax = ImVec2(ArrowX + ArrowSize, ArrowY + ArrowSize);

        EditorWidgets::DrawIcon(DrawList, EditorIcons::DownArrowIcon, ArrowMin, ArrowMax, ContentColor);
    }

    OutAnchor.Min = Min;
    OutAnchor.Max = Max;
    bOutHovered   = bHovered;

    return bPressed;
}

bool EditorWidgets::DrawDialogButton(const CHAR* Label, const ImVec2& Size)
{
    ImGui::PushFont(EditorFonts::SegoeUI_22);
    const bool bPressed = DrawButton(Label, Size);
    ImGui::PopFont();

    return bPressed;
}

bool EditorWidgets::DrawSearchField(const CHAR* InId, const CHAR* InHint, CHAR* InOutBuffer, int32 InBufferSize, float InWidth, bool bDrawBorder)
{
    if (!InId || !InHint || !InOutBuffer || InBufferSize <= 0)
    {
        return false;
    }

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return false;
    }

    // -------------------------------------------------------------------------------------
    // Layout constants
    // -------------------------------------------------------------------------------------

    const ImVec2 BasePadding = EditorStyleVars::InputFieldFramePadding;

    const float IconGap     = 6.0f;
    const float IconSize    = 16.0f;
    const float Rounding    = EditorStyleVars::InputFieldBorderRounding;
    const float BorderThick = EditorStyleVars::InputFieldBorderThickness;
    const float TotalWidth  = (InWidth <= 0.0f) ? ImGui::GetContentRegionAvail().x : InWidth;
    const float TotalHeight = ImGui::GetFontSize() + BasePadding.y * 2.0f;

    if (TotalWidth <= 1.0f)
    {
        ImGui::Dummy(ImVec2(1.0f, TotalHeight));
        return false;
    }

    const ImVec2 Start         = ImGui::GetCursorScreenPos();
    const ImVec2 End           = ImVec2(Start.x + TotalWidth, Start.y + TotalHeight);
    const ImRect FullRect      = ImRect(Start, End);
    const float  IconAreaWidth = BasePadding.x + IconSize + IconGap;

    // -------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImU32  BgColor            = IM_COL32(15, 15, 15, 255);
    const ImU32  BorderColorNormal  = IM_COL32(51, 51, 51, 255);
    const ImU32  BorderColorHovered = IM_COL32(74, 74, 74, 255);
    const ImU32  BorderColorActive  = IM_COL32(9, 92, 176, 255);
    const ImVec4 HintTextInactive   = ImVec4(76.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f);
    const ImVec4 HintTextActive     = ImVec4(97.0f / 255.0f, 97.0f / 255.0f, 97.0f / 255.0f, 1.0f);
    const ImVec4 InputTextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImU32  IconInactive       = IM_COL32(192, 192, 192, 255);
    const ImU32  IconActive         = IM_COL32(255, 255, 255, 255);

    const ImGuiID PendingInputId  = ImGui::GetID(InId);
    const bool    bInputWasActive = ImGui::GetActiveID() == PendingInputId;
    const bool    bInputClicked   = ImGui::IsMouseHoveringRect(FullRect.Min, FullRect.Max, true) && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool    bUseActiveHint  = bInputWasActive || bInputClicked;
    const ImVec4  HintTextColor   = bUseActiveHint ? HintTextActive : HintTextInactive;
    DrawList->AddRectFilled(Start, End, BgColor, Rounding);

    // -------------------------------------------------------------------------------------
    // Input
    // -------------------------------------------------------------------------------------

    ImGui::SetNextItemWidth(TotalWidth);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(IconAreaWidth, BasePadding.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, InputTextColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, HintTextColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, EditorStyleVars::InputFieldSelectionColor);

    bool bChanged = ImGui::InputTextWithHint(InId, InHint, InOutBuffer, static_cast<size_t>(InBufferSize));

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);

    const ImGuiID InputId = ImGui::GetItemID();

    // -------------------------------------------------------------------------------------
    // Icon rect
    // -------------------------------------------------------------------------------------

    const float  IconY    = Start.y + (TotalHeight - IconSize) * 0.5f;
    const ImVec2 IconMin  = ImVec2(Start.x + BasePadding.x, IconY);
    const ImVec2 IconMax  = ImVec2(IconMin.x + IconSize, IconMin.y + IconSize);
    const ImRect IconRect = ImRect(IconMin, IconMax);

    const bool bHasText = InOutBuffer[0] != '\0';

    // -------------------------------------------------------------------------------------
    // Clearing behavior
    // -------------------------------------------------------------------------------------

    bool bClearedThisFrame = false;

    if (bHasText && EditorIcons::CloseIcon)
    {
        const bool bIconHovered = ImGui::IsMouseHoveringRect(IconRect.Min, IconRect.Max, true);
        const bool bIconPressed = bIconHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (bIconHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 Tint = bIconHovered ? IconActive : IconInactive;
        EditorWidgets::DrawIcon(DrawList, EditorIcons::CloseIcon, IconMin, IconMax, Tint);

        if (bIconPressed)
        {
            InOutBuffer[0]    = '\0';
            bClearedThisFrame = true;
            bChanged          = true;

            if (ImGuiInputTextState* State = ImGui::GetInputTextState(InputId))
            {
                State->ClearText();
                State->CursorClamp();
            }

            ImGui::SetActiveID(InputId, Window);
            ImGui::SetFocusID(InputId, Window);
            ImGui::FocusWindow(Window);
        }
    }
    else if (EditorIcons::SearchIcon)
    {
        const bool bActive = ImGui::GetActiveID() == InputId;
        const ImU32 Tint = bActive ? IconActive : IconInactive;
        EditorWidgets::DrawIcon(DrawList, EditorIcons::SearchIcon, IconMin, IconMax, Tint);
    }

    // -------------------------------------------------------------------------------------
    // Border
    // -------------------------------------------------------------------------------------

    if (bDrawBorder)
    {
        const bool bActive = ImGui::GetActiveID() == InputId;
        const bool bHover  = ImGui::IsMouseHoveringRect(FullRect.Min, FullRect.Max, true);

        const ImU32 BorderColor = bActive ? BorderColorActive : (bHover ? BorderColorHovered : BorderColorNormal);
        DrawList->AddRect(Start, End, BorderColor, Rounding, ImDrawFlags_None, BorderThick);
    }

    ImGui::SetCursorScreenPos(ImVec2(Start.x, Start.y + TotalHeight));
    return bChanged || bClearedThisFrame;
}

void EditorWidgets::DrawTextWithSearchHighlight(ImDrawList* DrawList, const ImVec2& TextPos, const CHAR* Text, const CHAR* FilterText, ImU32 BaseTextU32, float HighlightPadX, float HighlightPadY, const ImVec2* ClampMin, const ImVec2* ClampMax)
{
    if (!DrawList || !Text)
    {
        return;
    }

    const CHAR* Query = (FilterText && *FilterText != 0) ? FilterText : nullptr;

    int32 MatchStart = -1;
    int32 MatchLen   = 0;

    if (Query)
    {
        if (const CHAR* MatchPtr = CString::Stristr(Text, Query))
        {
            MatchStart = static_cast<int32>(MatchPtr - Text);
            MatchLen   = static_cast<int32>(CString::Strlen(Query));
        }
    }

    if (MatchStart >= 0 && MatchLen > 0)
    {
        const float Scale = ImGui::GetIO().DisplayFramebufferScale.x;
        const float PadX  = HighlightPadX * Scale;
        const float PadY  = HighlightPadY * Scale;

        const ImU32 HighlightBgU32   = IM_COL32(139, 194, 74, 255);
        const ImU32 HighlightTextU32 = IM_COL32(0, 0, 0, 255);

        const ImVec2 PrefixSize = ImGui::CalcTextSize(Text, Text + MatchStart);
        const ImVec2 MatchSize  = ImGui::CalcTextSize(Text + MatchStart, Text + MatchStart + MatchLen);

        const ImVec2 PrefixPos = TextPos;
        const ImVec2 MatchPos  = ImVec2(TextPos.x + PrefixSize.x, TextPos.y);
        const ImVec2 SuffixPos = ImVec2(MatchPos.x + MatchSize.x, TextPos.y);

        if (MatchStart > 0)
        {
            DrawList->AddText(PrefixPos, BaseTextU32, Text, Text + MatchStart);
        }

        ImVec2 HighlightMin = ImVec2(MatchPos.x - PadX, MatchPos.y - PadY);
        ImVec2 HighlightMax = ImVec2(MatchPos.x + MatchSize.x + PadX, MatchPos.y + MatchSize.y + PadY);

        if (ClampMin && ClampMax)
        {
            HighlightMin.x = ImMax(HighlightMin.x, ClampMin->x);
            HighlightMin.y = ImMax(HighlightMin.y, ClampMin->y);
            HighlightMax.x = ImMin(HighlightMax.x, ClampMax->x);
            HighlightMax.y = ImMin(HighlightMax.y, ClampMax->y);
        }

        DrawList->AddRectFilled(HighlightMin, HighlightMax, HighlightBgU32, 0.0f);
        DrawList->AddText(MatchPos, HighlightTextU32, Text + MatchStart, Text + MatchStart + MatchLen);

        const CHAR* Suffix = Text + MatchStart + MatchLen;
        if (Suffix && *Suffix != 0)
        {
            DrawList->AddText(SuffixPos, BaseTextU32, Suffix);
        }
    }
    else
    {
        DrawList->AddText(TextPos, BaseTextU32, Text);
    }
}

void EditorWidgets::MenuSeparator(float Thickness, float PaddingY)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }

    const ImVec2 CursorMin = ImGui::GetCursorScreenPos();
    const float  Width     = ImGui::GetContentRegionAvail().x;

    constexpr float InsetX = 20.0f;

    if (Width > (InsetX * 2.0f + 1.0f))
    {
        const ImU32 Color = IM_COL32(106, 106, 106, 255);

        const float Y  = CursorMin.y + ((Thickness <= 1.0f) ? 0.5f : 0.0f);
        const ImVec2 A = ImVec2(CursorMin.x + InsetX, Y);
        const ImVec2 B = ImVec2(CursorMin.x + Width - InsetX, Y);

        ImGui::GetWindowDrawList()->AddLine(A, B, Color, Thickness);
    }

    ImGui::Dummy(ImVec2(0.0f, Thickness));

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }
}

void EditorWidgets::MenuLabeledSeparator(const CHAR* Label, float Thickness, float PaddingY)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 CursorMin = ImGui::GetCursorScreenPos();
    const float  Width     = ImGui::GetContentRegionAvail().x;

    // Use same background as menu popup (rgb 56,56,56) so separator has no different background
    const ImU32 MenuSeparatorBg = IM_COL32(56, 56, 56, 255);

    constexpr float InsetX = 20.0f;

    const ImU32 LineColor  = IM_COL32(106, 106, 106, 255);
    const ImU32 LabelColor = IM_COL32(160, 160, 160, 255);

    const CHAR* LabelToDraw = "";
    TStaticArray<CHAR, 128> UpperLabel{};

    if (Label && Label[0] != '\0')
    {
        int32 i = 0;
        const int32 UpperLabelMax = static_cast<int32>(UpperLabel.Size()) - 1;
        for (; Label[i] != '\0' && i < UpperLabelMax; ++i)
        {
            const CHAR Ch = static_cast<CHAR>(Label[i]);
            UpperLabel[i] = static_cast<CHAR>(CharTraits::ToUpper(Ch));
        }

        UpperLabel[i] = '\0';
        LabelToDraw   = UpperLabel.Data();
    }

    const ImVec2 LabelSize = ImGui::CalcTextSize(LabelToDraw);
    const float  RowHeight = Math::Max(LabelSize.y, Thickness);
    const float  YLine     = CursorMin.y + (RowHeight * 0.5f) + ((Thickness <= 1.0f) ? 0.5f : 0.0f);
    const float  LabelX    = CursorMin.x + InsetX;
    const float  LabelY    = CursorMin.y + (RowHeight - LabelSize.y) * 0.5f;

    DrawList->AddRectFilled(CursorMin, ImVec2(CursorMin.x + Width, CursorMin.y + RowHeight), MenuSeparatorBg, 0.0f);

    if (LabelSize.x > 0.0f)
    {
        DrawList->AddText(ImVec2(LabelX, LabelY), LabelColor, LabelToDraw);
    }

    const float GapAfterLabel = 12.0f;
    const float LineStartX    = LabelX + LabelSize.x + GapAfterLabel;
    const float LineEndX      = CursorMin.x + Width - InsetX;

    if (LineEndX > LineStartX + 1.0f)
    {
        DrawList->AddLine(ImVec2(LineStartX, YLine), ImVec2(LineEndX, YLine), LineColor, Thickness);
    }

    ImGui::Dummy(ImVec2(0.0f, RowHeight));

    if (PaddingY > 0.0f)
    {
        const ImVec2 PadMin = ImVec2(CursorMin.x, CursorMin.y + RowHeight);
        const ImVec2 PadMax = ImVec2(CursorMin.x + Width, CursorMin.y + RowHeight + PaddingY);
        DrawList->AddRectFilled(PadMin, PadMax, MenuSeparatorBg, 0.0f);
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }
}

bool EditorWidgets::MenuItem(const CHAR* Label, const CHAR* Shortcut, bool bSelected, bool bEnabled, bool bDrawBorder)
{
    const float PaddingX    = 8.0f;
    const float PaddingY    = MenuRowPaddingY;
    const float RowHeight   = ImGui::GetFontSize() + PaddingY * 2.0f;
    const float RowWidth    = ImGui::GetContentRegionAvail().x;
    const float CheckSize   = ImGui::GetFontSize() * 0.85f;
    const float GapRight    = 8.0f;
    const float ClipGap     = 4.0f;
    const float MenuIndentX = MenuRowIndentX;
    const float IconGutterX = 0.0f;

    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    const ImGuiSelectableFlags Flags =
        ImGuiSelectableFlags_SpanAvailWidth |
        ImGuiSelectableFlags_NoPadWithHalfSpacing;

    const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive  = ImGui::IsItemActive();

    const ImVec2 RectMin = ImGui::GetItemRectMin();
    const ImVec2 RectMax = ImGui::GetItemRectMax();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // -----------------------------------------------------------------------------------------
    // Background
    // -----------------------------------------------------------------------------------------

    ImU32 Background = ImGui::GetColorU32(ImGuiCol_Header);
    if (bActive)
    {
        Background = ImGui::GetColorU32(ImGuiCol_HeaderActive);
    }
    else if (bHovered)
    {
        Background = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
    }

    DrawList->AddRectFilled(RectMin, RectMax, Background, 0.0f);

    if (bDrawBorder && bHovered)
    {
        const ImVec4 HoveredColor = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
        const ImU32  BorderColor  = EditorHelpers::CreateBrighterColorU32(HoveredColor, 0.20f);
        DrawList->AddRect(RectMin, RectMax, BorderColor, 0.0f, ImDrawFlags_None, 1.0f);
    }

    // -----------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------
    
    const float LeftInnerX  = RectMin.x + MenuIndentX;
    const float RightInnerX = RectMax.x - MenuIndentX;

    const bool bHasShortcut = (Shortcut && Shortcut[0] != '\0');

    // -----------------------------------------------------------------------------------------
    // Upper-case shortcut
    // -----------------------------------------------------------------------------------------

    const CHAR* ShortcutToDraw = Shortcut;

    TStaticArray<CHAR, 128> UpperShortcut{};
    if (bHasShortcut)
    {
        int32 i = 0;
        const int32 UpperShortcutMax = static_cast<int32>(UpperShortcut.Size()) - 1;
        for (; Shortcut[i] != '\0' && i < UpperShortcutMax; ++i)
        {
            const CHAR Ch    = static_cast<CHAR>(Shortcut[i]);
            UpperShortcut[i] = static_cast<CHAR>(CharTraits::ToUpper(Ch));
        }
        UpperShortcut[i] = '\0';
        ShortcutToDraw   = UpperShortcut.Data();
    }

    const ImVec2 ShortcutSize = bHasShortcut ? ImGui::CalcTextSize(ShortcutToDraw) : ImVec2(0.0f, 0.0f);
    const ImVec2 LabelSize    = ImGui::CalcTextSize(Label);

    const float OpticalBiasY = 0.5f;
    const float LabelY       = RectMin.y + (RowHeight - LabelSize.y) * 0.5f + OpticalBiasY;

    const bool bDrawCheckMark = bSelected;
    const bool bDrawShortcut  = bHasShortcut;

    ImVec2 CheckPos         = ImVec2(0.0f, 0.0f);
    float  ShortcutX        = 0.0f;
    float  RightContentMinX = RightInnerX;

    if (bDrawCheckMark)
    {
        const float CheckY    = RectMin.y + (RowHeight - CheckSize) * 0.5f + OpticalBiasY;
        const float CheckMinX = RightInnerX - CheckSize;

        CheckPos         = ImVec2(CheckMinX, CheckY);
        RightContentMinX = CheckMinX;

        if (bDrawShortcut)
        {
            ShortcutX        = CheckMinX - GapRight - ShortcutSize.x;
            RightContentMinX = ShortcutX;
        }
    }
    else if (bDrawShortcut)
    {
        ShortcutX        = RightInnerX - ShortcutSize.x;
        RightContentMinX = ShortcutX;
    }

    // -----------------------------------------------------------------------------------------
    // Draw shortcut text
    // -----------------------------------------------------------------------------------------

    if (bDrawShortcut)
    {
        const float ShortcutY = RectMin.y + (RowHeight - ShortcutSize.y) * 0.5f + OpticalBiasY;
        DrawList->AddText(ImVec2(ShortcutX, ShortcutY), ImGui::GetColorU32(ImGuiCol_TextDisabled), ShortcutToDraw);
    }

    // -----------------------------------------------------------------------------------------
    // Draw checkmark
    // -----------------------------------------------------------------------------------------

    if (bDrawCheckMark)
    {
        if (EditorIcons::CheckmarkIcon)
        {
            const ImVec2 IconMin = CheckPos;
            const ImVec2 IconMax = ImVec2(CheckPos.x + CheckSize, CheckPos.y + CheckSize);

            const ImU32 Tint = ImGui::GetColorU32(ImGuiCol_Text);
            EditorWidgets::DrawIcon(DrawList, EditorIcons::CheckmarkIcon, IconMin, IconMax, Tint);
        }
        else
        {
            // Fallback if icon not loaded
            DrawCheckMark(DrawList, CheckPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
        }
    }

    // -----------------------------------------------------------------------------------------
    // Draw label
    // -----------------------------------------------------------------------------------------

    const float LabelX = LeftInnerX + PaddingX + IconGutterX;

    float ClipMaxX = RightInnerX - PaddingX;
    if (bDrawShortcut || bDrawCheckMark)
    {
        ClipMaxX = RightContentMinX - ClipGap;
    }

    ClipMaxX = Math::Max(ClipMaxX, LabelX + 1.0f);

    DrawList->PushClipRect(ImVec2(LabelX, RectMin.y), ImVec2(ClipMaxX, RectMax.y), true);
    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);
    DrawList->PopClipRect();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();
    return bEnabled && bPressed;
}

bool EditorWidgets::MenuSliderFloat(const CHAR* Label, float& InOutValue, float MinValue, float MaxValue, const CHAR* Format, float ValueWidth, bool bEnabled)
{
    return MenuFloatRow(EMenuFloatWidget::Slider, Label, InOutValue, 0.0f, MinValue, MaxValue, Format, ValueWidth, bEnabled);
}

bool EditorWidgets::MenuDragFloat(const CHAR* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR* Format, float ValueWidth, bool bEnabled)
{
    return MenuFloatRow(EMenuFloatWidget::Drag, Label, InOutValue, Speed, MinValue, MaxValue, Format, ValueWidth, bEnabled);
}

void EditorWidgets::MenuButton(const CHAR* Label, const CHAR* PopupId, bool bAnyPopupOpen, float ButtonHeight, FPopupAnchor& OutAnchor, bool bDrawBorder)
{
    const bool bThisPopupOpen = ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None);
    OutAnchor.bRequestPosition = false;

    if (bThisPopupOpen)
    {
        const ImU32 PressedBlue = EditorStyleVars::ButtonBgSelected;
        ImGui::PushStyleColor(ImGuiCol_Button, PressedBlue);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PressedBlue);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, PressedBlue);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(EditorStyleVars::ButtonPaddingX, 0.0f));
    const bool bPressed = ImGui::Button(Label, ImVec2(0.0f, ButtonHeight));
    ImGui::PopStyleVar();

    if (bPressed)
    {
        ImGui::OpenPopup(PopupId);
        OutAnchor.bRequestPosition = true;
    }

    if (bAnyPopupOpen && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && !bThisPopupOpen)
    {
        ImGui::OpenPopup(PopupId);
        OutAnchor.bRequestPosition = true;
    }

    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    OutAnchor.Min = ImGui::GetItemRectMin();
    OutAnchor.Max = ImGui::GetItemRectMax();

    if (bDrawBorder && bHovered)
    {
        const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRect(OutAnchor.Min, OutAnchor.Max, BorderColor, 0.0f, ImDrawFlags_None, 1.0f);
    }

    if (bThisPopupOpen)
    {
        ImGui::PopStyleColor(3);
    }
}

ImRect EditorWidgets::GetPopupExtentRect(const ImVec2& RefPos)
{
    ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
    for (int32 Index = 0; Index < PlatformIO.Monitors.Size; ++Index)
    {
        const ImGuiPlatformMonitor& Monitor = PlatformIO.Monitors[Index];

        const ImRect MonitorRect(Monitor.WorkPos, Monitor.WorkPos + Monitor.WorkSize);
        if (MonitorRect.Contains(RefPos))
        {
            return MonitorRect;
        }
    }

    ImGuiContext*        Context       = ImGui::GetCurrentContext();
    ImGuiWindow*         CurrentWindow = Context ? Context->CurrentWindow : nullptr;
    const ImGuiViewport* Viewport      = CurrentWindow ? CurrentWindow->Viewport : ImGui::GetMainViewport();

    if (Viewport)
    {
        return ImRect(Viewport->WorkPos, Viewport->WorkPos + Viewport->WorkSize);
    }

    return ImRect(ImVec2(0.0f, 0.0f), ImGui::GetIO().DisplaySize);
}

bool EditorWidgets::SetNextPopupPos(const CHAR* WindowId, const FPopupAnchor& Anchor, EPopupPlacement Placement, const ImVec2& FixedSize)
{
    ImVec2 Size = FixedSize;
    if (Size.x <= 0.0f || Size.y <= 0.0f)
    {
        ImGuiWindow* Window = ImGui::FindWindowByName(WindowId);
        if (!Window)
        {
            return false;
        }

        ImVec2 Measured = ImGui::CalcWindowNextAutoFitSize(Window);
        if (Measured.x <= 0.0f || Measured.y <= 0.0f)
        {
            Measured = Window->SizeFull;
        }

        Size.x = (Size.x > 0.0f) ? Size.x : Measured.x;
        Size.y = (Size.y > 0.0f) ? Size.y : Measured.y;

        if (Size.x <= 0.0f || Size.y <= 0.0f)
        {
            return false;
        }
    }

    ImVec2 RefPos;
    ImRect Avoid;
    switch (Placement)
    {
    case EPopupPlacement::BelowAnchor:
        RefPos = ImVec2(Anchor.Min.x, Anchor.Max.y);
        Avoid  = ImRect(Anchor.Min, Anchor.Max);
        break;

    case EPopupPlacement::RightOfAnchor:
        RefPos = Anchor.Min;
        Avoid  = ImRect(Anchor.Min.x, -FLT_MAX, Anchor.Max.x, FLT_MAX);
        break;

    default:
        RefPos = Anchor.Min;
        Avoid  = ImRect(Anchor.Min, Anchor.Max);
        break;
    }

    const ImRect  Outer        = GetPopupExtentRect(RefPos);
    const ImGuiID StateId      = ImHashStr(WindowId);
    const int32   CurrentFrame = ImGui::GetFrameCount();
    const int32   LastFrame    = GPopupLastPlacedFrame.GetInt(StateId, -1);

    GPopupLastPlacedFrame.SetInt(StateId, CurrentFrame);

    ImGuiDir* LastDir = GPopupLastDirections.GetIntRef(StateId, ImGuiDir_None);
    if (LastFrame < CurrentFrame - 1)
    {
        *LastDir = ImGuiDir_None;
    }

    ImVec2 Position;
    switch (Placement)
    {
    case EPopupPlacement::BelowAnchor:
        Position = ImGui::FindBestWindowPosForPopupEx(RefPos, Size, LastDir, Outer, Avoid, ImGuiPopupPositionPolicy_ComboBox);
        break;

    case EPopupPlacement::RightOfAnchor:
        Position = ImGui::FindBestWindowPosForPopupEx(RefPos, Size, LastDir, Outer, Avoid, ImGuiPopupPositionPolicy_Default);
        break;

    default:
        Position = ImClamp(RefPos, Outer.Min, ImMax(Outer.Min, Outer.Max - Size));
        break;
    }

    ImGui::SetNextWindowPos(Position, ImGuiCond_Always);
    return true;
}

bool EditorWidgets::SetNextBeginPopupPos(const CHAR* PopupId, const FPopupAnchor& Anchor, EPopupPlacement Placement, const ImVec2& FixedSize)
{
    TStaticArray<CHAR, 24> WindowName;
    CString::Snprintf(WindowName.Data(), static_cast<int32>(WindowName.Size()), "##Popup_%08x", ImGui::GetID(PopupId));

    return SetNextPopupPos(WindowName.Data(), Anchor, Placement, FixedSize);
}

bool EditorWidgets::BeginMenuPopup(const CHAR* PopupId, const FPopupAnchor& Anchor, float MinWidth, EPopupPlacement Placement)
{
    if (!Anchor.bRequestPosition && !ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None))
    {
        return false;
    }

    SetNextBeginPopupPos(PopupId, Anchor, Placement);

    PushMenuChromeStyle();

    const float FinalMinWidth = MinWidth + (MenuContentIndentX * 2.0f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(FinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopup(PopupId);
    if (bOpen)
    {
        DrawMenuFrame();
    }
    else
    {
        PopMenuChromeStyle();
    }

    return bOpen;
}

void EditorWidgets::EndMenuPopup()
{
    PopMenuChromeStyle();
    ImGui::EndPopup();
}

bool EditorWidgets::BeginPopupContextWindow(const CHAR* PopupId, ImGuiPopupFlags Flags)
{
    PushContextMenuStyle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(MenuFinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopupContextWindow(PopupId, Flags);
    if (bOpen)
    {
        DrawContextMenuFrame();
    }
    else
    {
        PopContextMenuStyle();
    }

    return bOpen;
}

bool EditorWidgets::BeginPopupContextItem(const CHAR* PopupId)
{
    PushContextMenuStyle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(MenuFinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopupContextItem(PopupId);
    if (bOpen)
    {
        DrawContextMenuFrame();
    }
    else
    {
        PopContextMenuStyle();
    }

    return bOpen;
}

bool EditorWidgets::BeginPopupContext(const CHAR* PopupId)
{
    if (!ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None))
    {
        return false;
    }

    PushContextMenuStyle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(MenuFinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopup(PopupId);
    if (bOpen)
    {
        DrawContextMenuFrame();
    }
    else
    {
        PopContextMenuStyle();
    }

    return bOpen;
}

bool EditorWidgets::MenuSubMenuRow(const CHAR* Label, FPopupAnchor& OutAnchor, bool& bOutHovered, bool bForceActive, float LabelIndentX)
{
    const float PaddingY  = 4.0f;
    const float RowHeight = ImGui::GetFontSize() + PaddingY * 2.0f;
    const float RowWidth  = ImGui::GetContentRegionAvail().x;

    ImGui::PushID(Label);

    const ImGuiSelectableFlags Flags =
        ImGuiSelectableFlags_SpanAvailWidth |
        ImGuiSelectableFlags_NoPadWithHalfSpacing |
        ImGuiSelectableFlags_DontClosePopups;

    const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool bActive  = ImGui::IsItemActive();

    const ImVec2 RectMin = ImGui::GetItemRectMin();
    const ImVec2 RectMax = ImGui::GetItemRectMax();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    ImU32 Background = ImGui::GetColorU32(ImGuiCol_Header);
    if (bActive || bForceActive)
    {
        Background = ImGui::GetColorU32(ImGuiCol_HeaderActive);
    }
    else if (bHovered)
    {
        Background = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
    }

    DrawList->AddRectFilled(RectMin, RectMax, Background, 0.0f);

    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    const float  LabelX    = RectMin.x + LabelIndentX;
    const float  LabelY    = RectMin.y + (RowHeight - LabelSize.y) * 0.5f;

    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);

    DrawSubMenuArrow(DrawList, RectMin, RectMax, RowHeight, LabelY);

    OutAnchor.Min              = RectMin;
    OutAnchor.Max              = RectMax;
    OutAnchor.bRequestPosition = false;

    bOutHovered = bHovered;

    ImGui::PopID();

    return bPressed;
}

void EditorWidgets::MenuSubMenuOverlay(const CHAR* Label, const FPopupAnchor& Anchor, float LabelIndentX)
{
    if (Anchor.Max.x <= Anchor.Min.x || Anchor.Max.y <= Anchor.Min.y)
    {
        return;
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(Anchor.Min, Anchor.Max, ImGui::GetColorU32(ImGuiCol_HeaderActive), 0.0f);

    const float  RowHeight = Anchor.Max.y - Anchor.Min.y;
    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    const float  LabelX    = Anchor.Min.x + LabelIndentX;
    const float  LabelY    = Anchor.Min.y + (RowHeight - LabelSize.y) * 0.5f;

    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);

    DrawSubMenuArrow(DrawList, Anchor.Min, Anchor.Max, RowHeight, LabelY);
}

bool EditorWidgets::BeginSubMenu(FSubMenuState& InOutState, const CHAR* PopupId, const CHAR* Label, bool bEnabled)
{
    InOutState.Label   = Label;
    InOutState.PopupId = PopupId;

    const bool bPopupOpen = ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    const bool bPressed = MenuSubMenuRow(Label, InOutState.Anchor, InOutState.bRowHovered, bPopupOpen, InOutState.LabelIndentX);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
        return false;
    }

    if (bPressed || InOutState.bRowHovered)
    {
        ImGui::OpenPopup(PopupId);
        InOutState.Anchor.bRequestPosition = true;
    }

    const ImGuiWindow* ParentWindow = ImGui::GetCurrentWindow();
    const float        ScrollbarX   = ParentWindow ? ParentWindow->ScrollbarSizes.x : 0.0f;

    const ImVec2 ParentMin = ImGui::GetWindowPos();
    const ImVec2 ParentMax = ParentMin + ImGui::GetWindowSize();

    FPopupAnchor FlyoutAnchor;
    FlyoutAnchor.Min              = ImVec2(ParentMin.x + MenuSubMenuOverlapX, InOutState.Anchor.Min.y);
    FlyoutAnchor.Max              = ImVec2(ParentMax.x - MenuSubMenuOverlapX - ScrollbarX, InOutState.Anchor.Min.y);
    FlyoutAnchor.bRequestPosition = InOutState.Anchor.bRequestPosition;

    const bool bOpen = BeginMenuPopup(PopupId, FlyoutAnchor, InOutState.MinWidth, EPopupPlacement::RightOfAnchor);
    if (bOpen)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        if (!GSubMenuStack.IsEmpty())
        {
            GSubMenuStack.Last().bChildOpen = true;
        }

        GSubMenuStack.Add(FSubMenuFrame());
    }

    return bOpen;
}

void EditorWidgets::EndSubMenu(FSubMenuState& InOutState)
{
    bool bChildOpen = false;
    if (!GSubMenuStack.IsEmpty())
    {
        bChildOpen = GSubMenuStack.Last().bChildOpen;
        GSubMenuStack.RemoveAt(GSubMenuStack.Size() - 1);
    }

    const bool  bFlyoutHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const float BridgeMaxX     = InOutState.Anchor.Max.x + 4.0f;
    const bool  bBridgeHovered = ImGui::IsMouseHoveringRect(InOutState.Anchor.Min, ImVec2(BridgeMaxX, InOutState.Anchor.Max.y), false);

    if (!(bFlyoutHovered || InOutState.bRowHovered || bBridgeHovered || bChildOpen))
    {
        ImGui::CloseCurrentPopup();
    }

    EndMenuPopup();
    MenuSubMenuOverlay(InOutState.Label, InOutState.Anchor, InOutState.LabelIndentX);
}

void EditorWidgets::EndPopupContext()
{
    PopContextMenuStyle();
    ImGui::EndPopup();
}

bool EditorWidgets::BeginPropertyTable(const CHAR* TableId, float LabelColumnWidth, float RevertColumnWidth, const FPropertyTableStyle& Style)
{
    // -----------------------------------------------------------------------------------------
    // Colors
    // -----------------------------------------------------------------------------------------

    const ImVec4 RowBg       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 TableBorder = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 FrameBg     = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 4.0f));

    ImGui::PushStyleColor(ImGuiCol_TableRowBg, RowBg);
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, RowBg);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, TableBorder);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, TableBorder);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, FrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, FrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, FrameBg);

    const ImGuiTableFlags Flags =
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoPadOuterX |
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_BordersInnerV;

    ImVec2 HeaderMin = ImGui::GetItemRectMin();
    ImVec2 HeaderMax = ImGui::GetItemRectMax();

    if (HeaderMax.x <= HeaderMin.x)
    {
        const ImVec2 CursorScreenPos = ImGui::GetCursorScreenPos();
        HeaderMin = CursorScreenPos;
        HeaderMax = ImVec2(CursorScreenPos.x + ImGui::GetContentRegionAvail().x, CursorScreenPos.y);
    }

    ImGui::SetCursorScreenPos(ImVec2(HeaderMin.x, HeaderMax.y));

    const float TableWidth = HeaderMax.x - HeaderMin.x;
    const ImVec2 OuterSize = ImVec2(TableWidth, 0.0f);

    GPropertyTableState.BorderMin   = ImGui::GetCursorScreenPos();
    GPropertyTableState.BorderWidth = TableWidth;

    if (!ImGui::BeginTable(TableId, 3, Flags, OuterSize))
    {
        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar();
        return false;
    }

    GPropertyTableState.Style            = Style;
    GPropertyTableState.CurrentRowHeight = 0.0f;
    GPropertyTableState.bActive          = true;

    ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
    ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Revert", ImGuiTableColumnFlags_WidthFixed, RevertColumnWidth);
    return true;
}

void EditorWidgets::EndPropertyTable()
{
    GPropertyTableState.bActive = false;

    ImGui::EndTable();

    const float X0         = GPropertyTableState.BorderMin.x;
    const float Y0         = GPropertyTableState.BorderMin.y;
    const float TableWidth = GPropertyTableState.BorderWidth;

    if (TableWidth > 0.0f)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const ImVec2 CursorAfterTable = ImGui::GetCursorScreenPos();
        float BottomY = CursorAfterTable.y - Style.ItemSpacing.y;

        const ImU32 Color     = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);
        const float Thickness = 2.0f;

        const float X1      = X0 + TableWidth;
        const float YTop    = Math::Floor(Y0) + 0.5f;
        const float YBottom = Math::Ceil(BottomY) - 0.5f;

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddLine(ImVec2(X0, YTop),    ImVec2(X1, YTop),    Color, Thickness);
        DrawList->AddLine(ImVec2(X0, YBottom), ImVec2(X1, YBottom), Color, Thickness);

        ImGui::SetCursorScreenPos(ImVec2(CursorAfterTable.x, BottomY + Thickness * 0.5f));
    }

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar();
}

void EditorWidgets::PropertyRowLabel(const CHAR* Label)
{
    const float ContentHeight = ImGui::GetTextLineHeight();
    BeginPropertyTableRow(ContentHeight);

    BeginPropertyTableLabelCell(ContentHeight);
    ImGui::TextUnformatted(Label);
}

void EditorWidgets::PropertyTableBeginValueCell(float ContentHeight)
{
    BeginPropertyTableValueCell(ContentHeight);
}

void EditorWidgets::PropertySeparatorRow(float PaddingY)
{
    UNREFERENCED_VARIABLE(PaddingY);
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Separator();
        return;
    }

    ImGui::TableNextRow();

    const int32 ColumnCount = ImGui::TableGetColumnCount();
    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        ImGui::TableSetColumnIndex(ColumnIndex);
        ImGui::Separator();
    }
}

bool EditorWidgets::BeginRichTextView(const CHAR* InId, const ImVec2& InSize, FRichTextViewContext& InOutContext, ImGuiWindowFlags InFlags, bool bWithContextMenu)
{
    InOutContext.bActive = false;

    const ImGuiWindowFlags Flags = InFlags | ImGuiWindowFlags_HorizontalScrollbar;

    InOutContext.ViewId = ImGui::GetID(InId);

    const bool bOpen = ImGui::BeginChild(InId, InSize, ImGuiChildFlags_Border, Flags);
    if (!bOpen)
    {
        return false;
    }

    InOutContext.bActive      = true;
    InOutContext.LineHeight   = ImGui::GetTextLineHeight();
    InOutContext.CharWidth    = MeasureRichTextWidth("A");
    InOutContext.ContentStart = ImGui::GetCursorScreenPos();

    if (InOutContext.MeasuredFont != ImGui::GetFont() || InOutContext.MeasuredFontSize != ImGui::GetFontSize())
    {
        for (FRichTextLine& Line : InOutContext.Lines)
        {
            for (FRichTextSpan& Span : Line.Spans)
            {
                Span.Width = MeasureRichTextWidth(InOutContext.GetSpanText(Span));
            }
        }

        InOutContext.MeasuredFont     = ImGui::GetFont();
        InOutContext.MeasuredFontSize = ImGui::GetFontSize();
    }

    if (bWithContextMenu && EditorWidgets::BeginPopupContextWindow("##RichTextViewContext", ImGuiPopupFlags_MouseButtonRight))
    {
        EditorWidgets::MenuLabeledSeparator("Log");

        const String Selected = BuildSelectedText(InOutContext);
        if (EditorWidgets::MenuItem("Copy Selection", nullptr, false, InOutContext.bHasSelection && !Selected.IsEmpty()))
        {
            ImGui::SetClipboardText(*Selected);
        }

        if (EditorWidgets::MenuItem("Copy All"))
        {
            const String All = BuildAllText(InOutContext);
            ImGui::SetClipboardText(*All);
        }

        if (EditorWidgets::MenuItem("Clear Selection", nullptr, false, InOutContext.bHasSelection))
        {
            InOutContext.bHasSelection = false;
            InOutContext.bSelecting = false;
        }

        EditorWidgets::EndPopupContext();
    }

    return true;
}

void EditorWidgets::RichTextSelectAll(FRichTextViewContext& InOutContext)
{
    if (InOutContext.Lines.IsEmpty())
    {
        InOutContext.bHasSelection = false;
        InOutContext.bSelecting    = false;
        return;
    }

    const int32 LastLineIndex = InOutContext.Lines.Size() - 1;
    const int32 LastCol       = InOutContext.Lines[LastLineIndex].TotalChars;

    InOutContext.bHasSelection   = true;
    InOutContext.bSelecting      = false;
    InOutContext.SelStart.Line   = 0;
    InOutContext.SelStart.Column = 0;
    InOutContext.SelEnd.Line     = LastLineIndex;
    InOutContext.SelEnd.Column   = LastCol;
}

String EditorWidgets::GetSelectedRichText(const FRichTextViewContext& InContext)
{
    return BuildSelectedText(InContext);
}

String EditorWidgets::GetAllRichText(const FRichTextViewContext& InContext)
{
    return BuildAllText(InContext);
}

void EditorWidgets::RichTextNewLine(FRichTextViewContext& InOutContext)
{
    if (!InOutContext.bActive)
    {
        return;
    }

    FRichTextLine Line;
    Line.TotalChars = 0;
    InOutContext.Lines.Add(Line);
}

void EditorWidgets::RichTextAddText(FRichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor)
{
    RichTextAddText(InOutContext, InText, InText ? static_cast<int32>(CString::Strlen(InText)) : 0, InTextColor);
}

void EditorWidgets::RichTextAddText(FRichTextViewContext& InOutContext, const CHAR* InText, int32 InLength, ImU32 InTextColor)
{
    if (!InOutContext.bActive || InOutContext.Lines.IsEmpty() || !InText || InLength <= 0)
    {
        return;
    }

    FRichTextLine& Line = InOutContext.Lines[InOutContext.Lines.Size() - 1];

    FRichTextSpan& Span = Line.Spans.Emplace();
    Span.TextOffset     = AppendToTextArena(InOutContext, InText, InLength);
    Span.TextLength     = InLength;
    Span.Width          = MeasureRichTextWidth(InOutContext.GetSpanText(Span));
    Span.TextColor      = InTextColor;
    Span.bHasBackground = false;

    Line.TotalChars += InLength;

    InOutContext.MaxLineChars = Math::Max(InOutContext.MaxLineChars, Line.TotalChars);
}

void EditorWidgets::RichTextAddTextBg(FRichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor, ImU32 InBackgroundColor)
{
    RichTextAddTextBg(InOutContext, InText, InText ? static_cast<int32>(CString::Strlen(InText)) : 0, InTextColor, InBackgroundColor);
}

void EditorWidgets::RichTextAddTextBg(FRichTextViewContext& InOutContext, const CHAR* InText, int32 InLength, ImU32 InTextColor, ImU32 InBackgroundColor)
{
    if (!InOutContext.bActive || InOutContext.Lines.IsEmpty() || !InText || InLength <= 0)
    {
        return;
    }

    FRichTextLine& Line = InOutContext.Lines[InOutContext.Lines.Size() - 1];

    FRichTextSpan& Span = Line.Spans.Emplace();
    Span.TextOffset      = AppendToTextArena(InOutContext, InText, InLength);
    Span.TextLength      = InLength;
    Span.Width           = MeasureRichTextWidth(InOutContext.GetSpanText(Span));
    Span.TextColor       = InTextColor;
    Span.bHasBackground  = true;
    Span.BackgroundColor = InBackgroundColor;

    Line.TotalChars += InLength;

    InOutContext.MaxLineChars = Math::Max(InOutContext.MaxLineChars, Line.TotalChars);
}

void EditorWidgets::EndRichTextView(FRichTextViewContext& InOutContext)
{
    if (!InOutContext.bActive)
    {
        ImGui::EndChild();
        return;
    }

    ImGuiIO& State = ImGui::GetIO();

    ImDrawList*  DrawList      = ImGui::GetWindowDrawList();
    ImGuiWindow* CurrentWindow = ImGui::GetCurrentWindow();

    const float FullLineHeight = InOutContext.LineHeight;
    const float TotalHeight    = InOutContext.Padding.y + static_cast<float>(InOutContext.Lines.Size()) * FullLineHeight + InOutContext.Padding.y;
    const float TotalWidth     = InOutContext.Padding.x + static_cast<float>(InOutContext.MaxLineChars) * InOutContext.CharWidth + InOutContext.Padding.x;

    {
        const ImVec2 SavedCursorPos = ImGui::GetCursorPos();
        ImGui::Dummy(ImVec2(TotalWidth, TotalHeight));
        ImGui::SetCursorPos(SavedCursorPos);
    }

    const auto MarkKeyboardCaptureActive = [&]()
    {
        ImGui::SetWindowFocus();
        ImGui::SetNextFrameWantCaptureKeyboard(true);
    };

    // -----------------------------------------------------------------------------------------
    // Selection begin
    // -----------------------------------------------------------------------------------------

    const bool bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy);

    bool bHoveredContent = false;

    if (bHovered)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const bool bHasVScroll = ImGui::GetScrollMaxY() > 0.0f;
        const bool bHasHScroll = ImGui::GetScrollMaxX() > 0.0f;

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Mouse   = State.MousePos;

        const float MaxX = WinPos.x + WinSize.x - (bHasVScroll ? Style.ScrollbarSize : 0.0f);
        const float MaxY = WinPos.y + WinSize.y - (bHasHScroll ? Style.ScrollbarSize : 0.0f);

        bHoveredContent = (Mouse.x < MaxX) && (Mouse.y < MaxY);

        if (bHoveredContent)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
        }
    }

    if (bHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
    {
        MarkKeyboardCaptureActive();
    }

    if (bHovered && (State.MouseWheel != 0.0f || State.MouseWheelH != 0.0f))
    {
        MarkKeyboardCaptureActive();
    }

    if (bHoveredContent && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const FRichTextSelectionPoint Point = GetMouseSelectionPoint(InOutContext, State.MousePos);
        const int32                   Count = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left);

        if (Count >= 3)
        {
            InOutContext.SelStart.Line   = Point.Line;
            InOutContext.SelStart.Column = 0;
            InOutContext.SelEnd.Line     = Point.Line;
            InOutContext.SelEnd.Column   = InOutContext.Lines.IsValidIndex(Point.Line) ? InOutContext.Lines[Point.Line].TotalChars : 0;

            InOutContext.bHasSelection = true;

            InOutContext.bSelecting = false;
        }
        else if (Count == 2)
        {
            const String LineText = BuildLineText(InOutContext, Point.Line);

            int32 WordStart = 0;
            int32 WordEnd   = 0;
            FindWordBounds(*LineText, static_cast<int32>(CString::Strlen(*LineText)), Point.Column, WordStart, WordEnd);

            InOutContext.SelStart.Line   = Point.Line;
            InOutContext.SelStart.Column = WordStart;
            InOutContext.SelEnd.Line     = Point.Line;
            InOutContext.SelEnd.Column   = WordEnd;

            InOutContext.bHasSelection = WordEnd > WordStart;
            InOutContext.bSelecting    = false;
        }
        else if (State.KeyShift && InOutContext.bHasSelection)
        {
            InOutContext.SelEnd     = Point;
            InOutContext.bSelecting = true;
        }
        else
        {
            InOutContext.SelStart = Point;
            InOutContext.SelEnd   = Point;

            InOutContext.bHasSelection = true;
            InOutContext.bSelecting    = true;
        }

        MarkKeyboardCaptureActive();
    }

    // -----------------------------------------------------------------------------------------
    // Select all (Ctrl/Cmd + A)
    // -----------------------------------------------------------------------------------------

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);
        EditorWidgets::RichTextSelectAll(InOutContext);
    }

    // -----------------------------------------------------------------------------------------
    // Selection update
    // -----------------------------------------------------------------------------------------

    if (InOutContext.bSelecting && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        InOutContext.SelEnd = GetMouseSelectionPoint(InOutContext, State.MousePos);

        {
            const float TopY              = CurrentWindow->InnerRect.Min.y;
            const float BottomY           = CurrentWindow->InnerRect.Max.y;
            const float EdgeInside        = 10.0f;
            const float RampDistance      = 100.0f;
            const float BaseSpeedPxPerSec = 20.0f;
            const float MaxSpeedPxPerSec  = 2000.0f;

            float ScrollDir       = 0.0f;
            float DistPastTrigger = 0.0f;

            if (State.MousePos.y <= TopY + EdgeInside)
            {
                ScrollDir = -1.0f;

                const float TriggerY = TopY + EdgeInside;
                DistPastTrigger = TriggerY - State.MousePos.y;
            }
            else if (State.MousePos.y >= BottomY - EdgeInside)
            {
                ScrollDir = 1.0f;

                const float TriggerY = BottomY - EdgeInside;
                DistPastTrigger = State.MousePos.y - TriggerY;
            }

            DistPastTrigger = Math::Max(0.0f, DistPastTrigger);

            const float ScrollMaxY = ImGui::GetScrollMaxY();
            if (ScrollDir != 0.0f && ScrollMaxY > 0.0f)
            {
                const float DeltaTime = (State.DeltaTime > 0.0f) ? State.DeltaTime : (1.0f / 60.0f);

                float T = DistPastTrigger / RampDistance;
                T = Math::Clamp(T, 0.0f, 1.0f);

                T = T * T * (3.0f - 2.0f * T);

                const float Speed = BaseSpeedPxPerSec + (MaxSpeedPxPerSec - BaseSpeedPxPerSec) * T;
                float Delta = ScrollDir * Speed * DeltaTime;

                const float MinDelta = 1.0f;
                if (Delta > -MinDelta && Delta < MinDelta)
                {
                    Delta = (ScrollDir < 0.0f) ? -MinDelta : MinDelta;
                }

                float ScrollY = ImGui::GetScrollY();
                ScrollY = Math::Clamp(ScrollY + Delta, 0.0f, ScrollMaxY);
                ImGui::SetScrollY(ScrollY);

                InOutContext.SelEnd = GetMouseSelectionPoint(InOutContext, State.MousePos);
            }
        }
    }

    // -----------------------------------------------------------------------------------------
    // Selection end
    // -----------------------------------------------------------------------------------------

    if (InOutContext.bSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        InOutContext.bSelecting = false;

        if (InOutContext.SelStart.Line == InOutContext.SelEnd.Line && InOutContext.SelStart.Column == InOutContext.SelEnd.Column)
        {
            InOutContext.bHasSelection = false;
        }
    }

    // -----------------------------------------------------------------------------------------
    // Copy (Ctrl/Cmd + C)
    // -----------------------------------------------------------------------------------------

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);

        if (InOutContext.bHasSelection)
        {
            const String Selected = BuildSelectedText(InOutContext);
            if (!Selected.IsEmpty())
            {
                ImGui::SetClipboardText(*Selected);
            }
        }
        else if (bHoveredContent)
        {
            const FRichTextSelectionPoint Point    = GetMouseSelectionPoint(InOutContext, State.MousePos);
            const String                  LineText = BuildLineText(InOutContext, Point.Line);

            if (!LineText.IsEmpty())
            {
                ImGui::SetClipboardText(*LineText);
            }
        }
    }

    // -----------------------------------------------------------------------------------------
    // Draw
    // -----------------------------------------------------------------------------------------

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + InOutContext.Padding.y);

    ImGuiListClipper Clipper;
    Clipper.Begin(InOutContext.Lines.Size(), FullLineHeight);

    FRichTextSelectionPoint SelA = InOutContext.SelStart;
    FRichTextSelectionPoint SelB = InOutContext.SelEnd;
    NormalizeSelection(SelA, SelB);

    const ImU32 SelectionBg = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);

    const ImRect& InnerClip = CurrentWindow->InnerClipRect;
    const ImVec2  ClipMin   = ImVec2(InnerClip.Min.x + InOutContext.Padding.x, InnerClip.Min.y + InOutContext.Padding.y);
    const ImVec2  ClipMax   = ImVec2(
        Math::Max(ClipMin.x, InnerClip.Max.x - InOutContext.Padding.x),
        Math::Max(ClipMin.y, InnerClip.Max.y - InOutContext.Padding.y));

    DrawList->PushClipRect(ClipMin, ClipMax, true);

    while (Clipper.Step())
    {
        for (int32 LineIndex = Clipper.DisplayStart; LineIndex < Clipper.DisplayEnd; ++LineIndex)
        {
            if (!InOutContext.Lines.IsValidIndex(LineIndex))
            {
                continue;
            }

            const FRichTextLine& Line = InOutContext.Lines[LineIndex];

            const float Y = InOutContext.ContentStart.y + InOutContext.Padding.y + LineIndex * FullLineHeight;
            const float X = InOutContext.ContentStart.x + InOutContext.Padding.x;

            const ImVec2 LineMin = ImVec2(CurrentWindow->WorkRect.Min.x, Y);
            const ImVec2 LineMax = ImVec2(CurrentWindow->WorkRect.Max.x, Y + FullLineHeight);

            if (InOutContext.bHasSelection)
            {
                int32 ColStart = 0;
                int32 ColEnd   = 0;

                if (SelectionIntersectsLine(SelA, SelB, LineIndex, ColStart, ColEnd, Line.TotalChars))
                {
                    const float SelX0 = X + ColStart * InOutContext.CharWidth;
                    const float SelX1 = X + ColEnd   * InOutContext.CharWidth;

                    DrawList->AddRectFilled(ImVec2(SelX0, LineMin.y), ImVec2(SelX1, LineMax.y), SelectionBg, 0.0f);
                }
            }

            float CursorX = X;
            for (int32 s = 0; s < Line.Spans.Size(); ++s)
            {
                const FRichTextSpan& Span = Line.Spans[s];

                const CHAR* Text = InOutContext.GetSpanText(Span);
                if (!Text || Text[0] == 0)
                {
                    continue;
                }

                const float SpanWidth = Span.Width;
                if (Span.bHasBackground)
                {
                    const float HighlightPadY    = 2.0f;
                    const float HighlightMarginY = 2.0f;
                    const float HighlightTopY    = LineMin.y + HighlightPadY - HighlightMarginY;
                    const float HighlightBotY    = LineMax.y - HighlightPadY + HighlightMarginY;

                    DrawList->AddRectFilled(ImVec2(CursorX, HighlightTopY), ImVec2(CursorX + SpanWidth, HighlightBotY), Span.BackgroundColor, 0.0f);
                }

                DrawList->AddText(ImVec2(CursorX, LineMin.y), Span.TextColor, Text);
                CursorX += SpanWidth;
            }
        }
    }

    DrawList->PopClipRect();

    // -----------------------------------------------------------------------------------------
    // Scroll to bottom
    // -----------------------------------------------------------------------------------------

    const float CurrentScrollMaxY = ImGui::GetScrollMaxY();
    InOutContext.bIsScrolledToBottom = (CurrentScrollMaxY <= 0.0f) || (ImGui::GetScrollY() >= CurrentScrollMaxY - 1.0f);

    const bool bStayPinned = InOutContext.bAutoScroll && InOutContext.bIsScrolledToBottom;

    if ((InOutContext.bScrollToBottom || bStayPinned) && !InOutContext.bSelecting)
    {
        ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + TotalHeight, 1.0f);

        InOutContext.bScrollToBottom     = false;
        InOutContext.bIsScrolledToBottom = true;
    }

    ImGui::EndChild();
}

bool EditorWidgets::DrawButtonCenteredOnLine(const CHAR* Label, float Alignment)
{
    const ImVec2 ButtonSize = GetButtonSize(Label, ImVec2(0.0f, 0.0f));
    const float  Offset     = (ImGui::GetContentRegionAvail().x - ButtonSize.x) * Alignment;

    if (Offset > 0.0f)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
    }

    return DrawButton(Label, ButtonSize);
}

void EditorWidgets::DrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize)
{
    const float Thickness = ImMax(1.0f, CheckMarkSize / 6.0f);

    const ImVec2 PointA = ImVec2(Position.x + CheckMarkSize * 0.15f, Position.y + CheckMarkSize * 0.55f);
    const ImVec2 PointB = ImVec2(Position.x + CheckMarkSize * 0.40f, Position.y + CheckMarkSize * 0.80f);
    const ImVec2 PointC = ImVec2(Position.x + CheckMarkSize * 0.85f, Position.y + CheckMarkSize * 0.20f);

    DrawList->AddLine(PointA, PointB, Color, Thickness);
    DrawList->AddLine(PointB, PointC, Color, Thickness);
}

void EditorWidgets::DrawIcon(ImDrawList* DrawList, const FEditorIcon& Icon, const ImVec2& Min, const ImVec2& Max, ImU32 Tint)
{
    if (DrawList && Icon.IsValid())
    {
        DrawList->AddImage(Icon.Texture, Min, Max, Icon.UVMin, Icon.UVMax, Tint);
    }
}

FEditorIcon EditorIcons::UndoIcon             = FEditorIcon();
FEditorIcon EditorIcons::SearchIcon           = FEditorIcon();
FEditorIcon EditorIcons::LockedIcon           = FEditorIcon();
FEditorIcon EditorIcons::UnlockedIcon         = FEditorIcon();
FEditorIcon EditorIcons::FolderIcon           = FEditorIcon();
FEditorIcon EditorIcons::FolderSmallIcon      = FEditorIcon();
FEditorIcon EditorIcons::FolderSmall2Icon     = FEditorIcon();
FEditorIcon EditorIcons::FolderOpenSmallIcon  = FEditorIcon();
FEditorIcon EditorIcons::DocumentIcon         = FEditorIcon();
FEditorIcon EditorIcons::DocumentSmallIcon    = FEditorIcon();
FEditorIcon EditorIcons::CheckmarkIcon        = FEditorIcon();
FEditorIcon EditorIcons::ForbiddenIcon        = FEditorIcon();
FEditorIcon EditorIcons::CircledCheckmarkIcon = FEditorIcon();
FEditorIcon EditorIcons::NextIcon             = FEditorIcon();
FEditorIcon EditorIcons::PreviousIcon         = FEditorIcon();
FEditorIcon EditorIcons::CloseIcon            = FEditorIcon();
FEditorIcon EditorIcons::FilterIcon           = FEditorIcon();
FEditorIcon EditorIcons::RightArrowIcon       = FEditorIcon();
FEditorIcon EditorIcons::DownArrowIcon        = FEditorIcon();
FEditorIcon EditorIcons::CollapseArrowDown    = FEditorIcon();
FEditorIcon EditorIcons::CollapseArrowRight   = FEditorIcon();

bool EditorIcons::Initialize()
{
    Release();

    constexpr int32 IconCount = static_cast<int32>(ARRAY_COUNT(GIconRequests));

    TArray<FEditorIconImage> Images;
    Images.Resize(IconCount);

    bool bResult = true;

    int32 ShelfX      = 0;
    int32 ShelfY      = 0;
    int32 ShelfHeight = 0;
    int32 AtlasHeight = 0;

    for (int32 Index = 0; Index < IconCount; ++Index)
    {
        FEditorIconImage& Image = Images[Index];
        if (!LoadEditorIconImage(GIconRequests[Index].RelativePath, Image))
        {
            bResult = false;
            continue;
        }

        if (Image.Width > IconAtlasWidth)
        {
            LOG_ERROR("[EditorIcons]: Icon '%s' is %d wide, the atlas is only %d", GIconRequests[Index].RelativePath, Image.Width, IconAtlasWidth);

            Image.bValid = false;
            bResult      = false;
            continue;
        }

        if (ShelfX + Image.Width > IconAtlasWidth)
        {
            ShelfY     += ShelfHeight;
            ShelfX      = 0;
            ShelfHeight = 0;
        }

        Image.X = ShelfX;
        Image.Y = ShelfY;

        ShelfX      += Image.Width + IconAtlasPadding;
        ShelfHeight  = Math::Max(ShelfHeight, Image.Height + IconAtlasPadding);
        AtlasHeight  = Math::Max(AtlasHeight, ShelfY + Image.Height);
    }

    if (AtlasHeight <= 0)
    {
        LOG_ERROR("[EditorIcons]: No icon could be loaded, leaving the atlas empty");
        return false;
    }

    TArray<uint8> AtlasPixels;
    AtlasPixels.ResizeUninitialized(IconAtlasWidth * AtlasHeight * 4);
    Memory::Memzero(AtlasPixels.Data(), static_cast<uint64>(AtlasPixels.Size()));

    for (const FEditorIconImage& Image : Images)
    {
        if (!Image.bValid)
        {
            continue;
        }

        const int64 RowBytes = static_cast<int64>(Image.Width) * 4;
        for (int32 Y = 0; Y < Image.Height; ++Y)
        {
            uint8* Destination = AtlasPixels.Data() + ((static_cast<int64>(Image.Y + Y) * IconAtlasWidth) + Image.X) * 4;
            Memory::Memcpy(Destination, Image.Pixels.Data() + static_cast<int64>(Y) * RowBytes, RowBytes);
        }
    }

    GIconAtlas.Texture = FTextureFactory::Get().LoadFromMemory(AtlasPixels.Data(), IconAtlasWidth, AtlasHeight, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!GIconAtlas.Texture)
    {
        LOG_ERROR("[EditorIcons]: Failed to create the icon atlas texture");
        return false;
    }

    GIconAtlas.Texture->SetDebugName("Editor IconAtlas");

    GIconAtlas.ImGuiTexture = MakeUniquePtr<FImGuiTexture>(GIconAtlas.Texture);
    if (!GIconAtlas.ImGuiTexture)
    {
        LOG_ERROR("[EditorIcons]: Failed to create the ImGui texture wrapper for the icon atlas");
        return false;
    }

    GIconAtlas.ImGuiTexture->bEnableBlending      = true;
    GIconAtlas.ImGuiTexture->bEnableLinearSampler = true;

    const ImTextureID AtlasID      = reinterpret_cast<ImTextureID>(GIconAtlas.ImGuiTexture.Get());
    const float       AtlasWidthF  = static_cast<float>(IconAtlasWidth);
    const float       AtlasHeightF = static_cast<float>(AtlasHeight);

    for (int32 Index = 0; Index < IconCount; ++Index)
    {
        const FEditorIconImage& Image = Images[Index];
        if (!Image.bValid)
        {
            continue;
        }

        FEditorIcon& Icon = *GIconRequests[Index].Icon;

        Icon.Texture = AtlasID;
        Icon.UVMin   = ImVec2(static_cast<float>(Image.X) / AtlasWidthF, static_cast<float>(Image.Y) / AtlasHeightF);
        Icon.UVMax   = ImVec2(static_cast<float>(Image.X + Image.Width) / AtlasWidthF, static_cast<float>(Image.Y + Image.Height) / AtlasHeightF);
    }

    return bResult;
}

void EditorIcons::Release()
{
    for (const FEditorIconRequest& Request : GIconRequests)
    {
        *Request.Icon = FEditorIcon();
    }

    GIconAtlas.ImGuiTexture.Reset();
    GIconAtlas.Texture.Reset();
}

ImFont* EditorFonts::DefaultFont = nullptr;
ImFont* EditorFonts::SystemIcons = nullptr;
ImFont* EditorFonts::SegoeUI_18  = nullptr;
ImFont* EditorFonts::SegoeUI_22  = nullptr;
ImFont* EditorFonts::Consola_16  = nullptr;

bool EditorFonts::Initialize()
{
    if (!IImguiPlugin::IsEnabled())
    {
        return false;
    }

    ImGuiIO& State = ImGui::GetIO();
    State.Fonts->Clear();

    // Default font
    DefaultFont = State.Fonts->AddFontDefault();

    // Load fonts from file
    SegoeUI_18 = LoadEditorFont("Editor/Fonts/segoeui.ttf", 18.0f);
    SegoeUI_22 = LoadEditorFont("Editor/Fonts/segoeui.ttf", 22.0f);
    Consola_16 = LoadEditorFont("Editor/Fonts/consola.ttf", 16.0f);

#if PLATFORM_WINDOWS
    SystemIcons = LoadSystemIconFont();
#endif

    IImguiPlugin& ImGuiPlugin = IImguiPlugin::Get();
    if (!ImGuiPlugin.UpdateFontAtlas())
    {
        return false;
    }

    if (SegoeUI_18)
    {
        State.FontDefault = SegoeUI_18;
    }

    return true;
}

void EditorFonts::Release()
{
    DefaultFont = nullptr;
    SegoeUI_18  = nullptr;
    SegoeUI_22  = nullptr;
    Consola_16  = nullptr;
    SystemIcons = nullptr;
}
