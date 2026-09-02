#pragma once
#include "Core/Math/Color.h"
#include "Application/Layout/LayoutTypes.h"

struct IFontFace;

enum class EInteractionState : uint8
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
};

struct FUIStyleColors
{
    FUIStyleColors()
        : WindowBackground(0.09f, 0.09f, 0.11f, 1.0f)
        , PanelBackground(0.13f, 0.13f, 0.16f, 1.0f)
        , ControlNormal(0.20f, 0.21f, 0.25f, 1.0f)
        , ControlHovered(0.27f, 0.29f, 0.34f, 1.0f)
        , ControlPressed(0.16f, 0.17f, 0.21f, 1.0f)
        , ControlDisabled(0.16f, 0.16f, 0.18f, 1.0f)
        , ButtonNormal(0.22f, 0.22f, 0.22f, 1.0f)
        , ButtonHovered(0.34f, 0.34f, 0.34f, 1.0f)
        , ButtonPressed(0.34f, 0.34f, 0.34f, 1.0f)
        , MenuBarItemHovered(0.27f, 0.29f, 0.34f, 1.0f)
        , MenuBarItemActive(0.25f, 0.55f, 0.95f, 1.0f)
        , MenuBackground(0.13f, 0.13f, 0.16f, 1.0f)
        , MenuBorder(0.32f, 0.33f, 0.38f, 1.0f)
        , MenuInnerBorder(0.20f, 0.21f, 0.24f, 1.0f)
        , MenuItemHovered(0.25f, 0.55f, 0.95f, 1.0f)
        , MenuItemShortcut(0.45f, 0.46f, 0.50f, 1.0f)
        , MenuSeparator(106.0f / 255.0f, 106.0f / 255.0f, 106.0f / 255.0f, 1.0f)
        , MenuSectionText(160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f)
        , Border(0.32f, 0.33f, 0.38f, 1.0f)
        , Text(0.90f, 0.91f, 0.94f, 1.0f)
        , TextDisabled(0.45f, 0.46f, 0.50f, 1.0f)
        , TextSelectionBackground(0.20f, 0.42f, 0.78f, 1.0f)
        , Accent(0.25f, 0.55f, 0.95f, 1.0f)
        , AccentHovered(0.35f, 0.63f, 0.98f, 1.0f)
    {
    }

    FFloatColor WindowBackground;
    FFloatColor PanelBackground;
    FFloatColor ControlNormal;
    FFloatColor ControlHovered;
    FFloatColor ControlPressed;
    FFloatColor ControlDisabled;
    FFloatColor ButtonNormal;
    FFloatColor ButtonHovered;
    FFloatColor ButtonPressed;
    FFloatColor MenuBarItemHovered;
    FFloatColor MenuBarItemActive;
    FFloatColor MenuBackground;
    FFloatColor MenuBorder;
    FFloatColor MenuInnerBorder;
    FFloatColor MenuItemHovered;
    FFloatColor MenuItemShortcut;
    FFloatColor MenuSeparator;
    FFloatColor MenuSectionText;
    FFloatColor Border;
    FFloatColor Text;
    FFloatColor TextDisabled;
    FFloatColor TextSelectionBackground;
    FFloatColor Accent;
    FFloatColor AccentHovered;
};

struct FUIStyleMetrics
{
    FUIStyleMetrics()
        : ControlPadding(8, 4)
        , ButtonPadding(12, 4)
        , CornerRadius(3.0f)
        , ButtonCornerRadius(3.0f)
        , BorderThickness(1.0f)
        , RowHeight(24)
        , ButtonHeight(24)
        , ScrollBarThickness(12)
        , SeparatorThickness(1)
        , MenuSeparatorThickness(2)
    {
    }

    FMargin ControlPadding;

    /** @brief The space between a button's bounds and its label, which runs wider than a control's. */
    FMargin ButtonPadding;

    float CornerRadius;

    /** @brief How far a button's corners are rounded, which is rounder than a control's. */
    float ButtonCornerRadius;

    float BorderThickness;
    int32 RowHeight;

    /** @brief How tall a button is, which is taller than a row so a strip of them does not read as a list. */
    int32 ButtonHeight;

    int32 ScrollBarThickness;
    int32 SeparatorThickness;
    int32 MenuSeparatorThickness;
};

struct APPLICATION_API FUIStyle
{
    /**
     * @brief Gets the style every widget reads when it was not handed one of its own.
     *
     * @return The process-wide default, which is the shipped theme until SetDefault replaces it.
     */
    NODISCARD static const FUIStyle& GetDefault();

    /**
     * @brief Replaces the process-wide default style.
     *
     * @param InStyle The style to install.
     */
    static void SetDefault(const FUIStyle& InStyle);

    /** @brief Puts the process-wide default back to the shipped theme. */
    static void ResetDefault();

    FUIStyle();

    /**
     * @brief Picks the control fill matching an interaction state.
     *
     * @param State The state the control is in.
     * @return The fill color for that state.
     */
    NODISCARD const FFloatColor& GetControlColor(EInteractionState State) const;

    /**
     * @brief Picks the button fill matching an interaction state, which runs lighter than a control's and
     * switches to the accent pair for a button that is showing itself as the chosen one of a set.
     *
     * @param State       The state the button is in.
     * @param bIsSelected True for the lit fill, which a toggle that is on and an open dropdown both want.
     * @return The fill color for that state.
     */
    NODISCARD const FFloatColor& GetButtonColor(EInteractionState State, bool bIsSelected) const;

    /**
     * @brief Picks the text color matching an interaction state, which only dims when disabled.
     *
     * @param State The state the control is in.
     * @return The text color for that state.
     */
    NODISCARD const FFloatColor& GetTextColor(EInteractionState State) const;

    FUIStyleColors   Colors;
    FUIStyleMetrics  Metrics;
    const IFontFace* NormalFont;
    const IFontFace* MonospaceFont;
};
