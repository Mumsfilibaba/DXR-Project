#pragma once
#include "Application/Elements/TitleBar.h"
#include "Application/Elements/Window.h"

class FVerticalBox;

class APPLICATION_API FFloatingWindow
{
public:

    /** @brief The width a window gets unless the desc asks for another. */
    static constexpr int32 DefaultWidth = 480;

    /** @brief The height a window gets unless the desc asks for another. */
    static constexpr int32 DefaultHeight = 320;

public:
    struct FDesc
    {
        FDesc()
            : Title()
            , ParentWindow(nullptr)
            , Position()
            , Size(DefaultWidth, DefaultHeight)
            , Font(nullptr)
            , Icon()
            , TitleBarContent(nullptr)
            , Content(nullptr)
            , bShowOnCreate(true)
        {
        }

        /**
         * @brief Sets the caption text, which is also the platform window's title.
         *
         * @param InTitle The title to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetTitle(const String& InTitle)
        {
            Title = InTitle;
            return *this;
        }

        /**
         * @brief Sets the size and position of the window.
         *
         * @param InPosition The top-left corner, in screen coordinates.
         * @param InSize     The width and height.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetBounds(const IntVector2& InPosition, const IntVector2& InSize)
        {
            Position = InPosition;
            Size     = InSize;
            return *this;
        }

        /**
         * @brief Sets what fills the window under its caption.
         *
         * @param InContent The element to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetContent(const TSharedPtr<FVisualElement>& InContent)
        {
            Content = InContent;
            return *this;
        }

        String                     Title;
        TSharedPtr<FWindow>        ParentWindow;
        IntVector2                 Position;
        IntVector2                 Size;
        TSharedPtr<IFontFace>      Font;
        FUIBrush                   Icon;
        TSharedPtr<FVisualElement> TitleBarContent;
        TSharedPtr<FVisualElement> Content;
        bool                       bShowOnCreate : 1;
    };

public:

    /**
     * @brief Creates the window, its caption and its body, and shows it unless told not to.
     *
     * @param Desc Initialization parameters.
     * @return The helper, or null when there is no application to create the window in.
     */
    NODISCARD static TSharedPtr<FFloatingWindow> Create(const FDesc& Desc);

public:
    FFloatingWindow();
    ~FFloatingWindow();

    /**
     * @brief Replaces what fills the window under its caption.
     *
     * @param InContent The element to show.
     */
    void SetContent(const TSharedPtr<FVisualElement>& InContent);

    /** @brief Brings the window up, laying it out first. */
    void Show();

    /** @brief Takes the window down, after which the helper holds nothing. */
    void Close();

    /** @return True while the window is up, until Close() leaves the helper holding nothing. */
    NODISCARD bool IsOpen() const;

    /** @return The top-level window the helper created, which is null once it has been closed. */
    NODISCARD FORCEINLINE const TSharedPtr<FWindow>& GetWindow() const
    {
        return Window;
    }

    /**
     * @brief Gets the caption, so a caller can retitle it or reach its regions.
     *
     * @return The caption, which is null once the window has been closed.
     */
    NODISCARD FORCEINLINE const TSharedPtr<FTitleBar>& GetTitleBar() const
    {
        return TitleBar;
    }

private:
    void Initialize(const FDesc& Desc);

    TSharedPtr<FWindow>        Window;
    TSharedPtr<FTitleBar>      TitleBar;
    TSharedPtr<FVerticalBox>   Panel;
    TSharedPtr<FVisualElement> Content;
};
