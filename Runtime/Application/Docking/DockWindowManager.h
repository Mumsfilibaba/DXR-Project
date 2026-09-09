#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockLayoutFile.h"
#include "Application/Docking/DockingArea.h"
#include "RHI/RHIResources.h"

class FTitleBar;
class FWindow;

class APPLICATION_API FDockWindowManager
{
public:

    /** @brief The size a host gets when nothing names one. */
    static constexpr int32 DefaultHostWidth  = 520;
    static constexpr int32 DefaultHostHeight = 400;

    /** @brief How far above and left of the cursor a new host is placed, so the dropped tab lands under it. */
    static constexpr int32 SpawnCursorInset = 24;

    /**
     * @brief The size the decorator follows the cursor at, which is the host width in a band short enough
     * to leave the drop zones under it visible.
     */
    static constexpr int32 DecoratorWidth  = 520;
    static constexpr int32 DecoratorHeight = 160;

    /** @brief How see-through the decorator is, so what a drop would land on stays readable underneath it. */
    static constexpr float DecoratorOpacity = 0.65f;

public:
    struct FDesc
    {
        /** @brief The area a closing host hands its panels back to closed, which is the editor's own. */
        TSharedPtr<FDockingArea> MainArea = nullptr;

        /** @brief The description every host's area is created with, so a host behaves like the main one. */
        FDockingArea::FDesc AreaDesc;

        /**
         * @brief What a host's caption holds beside its title, which is the slot the editor shell puts its
         * menu bar into on the main window. Null leaves a host with a caption carrying nothing but the title.
         */
        TSharedPtr<FVisualElement> TitleBarContent = nullptr;

        /** @brief The size a host gets when the layout being restored does not carry one. */
        IntVector2 DefaultSize = IntVector2(DefaultHostWidth, DefaultHostHeight);
    };

public:

    /**
     * @brief Gets the process-wide manager, creating it on first use.
     *
     * @return The manager, which lives until Shutdown takes it down.
     */
    NODISCARD static FDockWindowManager& Get();

    /**
     * @brief Gets whether the manager exists, so a teardown path can avoid resurrecting it.
     *
     * @return True once it has been created and before Shutdown takes it down.
     */
    NODISCARD static bool IsInitialized();

    /** @brief Closes every host, returning its panels to the main area, and takes the manager down. */
    static void Shutdown();

public:
    FDockWindowManager();
    ~FDockWindowManager();

    /**
     * @brief Initializes the manager and binds it to the drop-outside path of the drag.
     *
     * @param InDesc Initialization parameters.
     */
    void Initialize(const FDesc& InDesc);

    /**
     * @brief Closes whatever emptied and retitles the rest. Both are deferred to here rather than done as
     * the drop happens, because a drop is handled from inside the tab strip that started it and closing its
     * window there would free the element still on the stack.
     */
    void Tick();

    /**
     * @brief Creates a host window holding one panel, which is what a tab dropped clear of every area gets.
     *
     * @param PanelId        The panel to put in it.
     * @param Label          The text its tab shows, which is also the window's caption.
     * @param Content        The panel's content.
     * @param ScreenPosition Where the window's top-left goes.
     * @param Size           The client size for the window.
     * @return The host's area, or null when the window could not be created.
     */
    TSharedPtr<FDockingArea> SpawnHost(const String& PanelId, const String& Label, const TSharedPtr<FVisualElement>& Content, const IntVector2& ScreenPosition, const IntVector2& Size);

    /** @brief Closes every host, returning its panels to the main area. */
    void CloseAllHosts();

    /**
     * @brief Follows the cursor with the decorator, keeping the grip it had on the tab it was torn out by.
     * It never resizes, so what a drop would land on is shown by the drop zones the area under the cursor
     * draws rather than by the decorator taking that shape.
     *
     * @param ScreenPosition Where the cursor is now.
     */
    void MoveDecorator(const IntVector2& ScreenPosition);

    /**
     * @brief Renders the panel in flight at the size the drop it is aimed at would give it, so the area
     * under the cursor can show it where it would land rather than a rectangle standing in for it. Every
     * zone the drag has already visited is kept, and a drag aimed at nothing keeps them all rather than
     * starting over, because the chips are islands with gaps between them and a cursor crossing a gap
     * would otherwise pay for the zone it is going back to.
     */
    void UpdateDropPreview();

    /** @brief Takes the decorator down, which every exit from a drag does. */
    void DestroyDecorator();

    /** @return The window the drag is dragging, or null when no tear-out is in flight. */
    NODISCARD FORCEINLINE TSharedPtr<FWindow> GetDecoratorWindow() const
    {
        return DecoratorWindow;
    }

    /** @return The area inside the decorator, or null when no tear-out is in flight. */
    NODISCARD FORCEINLINE TSharedPtr<FDockingArea> GetDecoratorArea() const
    {
        return DecoratorArea;
    }

    /** @return The picture the decorator is drawing under Docking.DecoratorSnapshot, null when it is live. */
    NODISCARD FORCEINLINE FRHITexture* GetDecoratorSnapshot() const
    {
        return DecoratorSnapshot.Get();
    }

    /**
     * @return The panel in flight drawn at the size the drop it is aimed at would give it, which the target
     * area covers that rectangle with. Null when nothing is aimed at or nothing has been rendered for the
     * zone that is, so a picture is never stretched into a rectangle it was not made for, and null
     * throughout when there is no RHI to render into, which leaves the target drawing a plain rectangle.
     */
    NODISCARD FRHITexture* GetDropPreviewTexture() const;

    /**
     * @brief Gathers what every host would need to be built again, which is what a save writes out.
     *
     * @return One entry per host, in creation order, none of them the main window.
     */
    NODISCARD TArray<FDockWindowLayout> SaveHostLayouts() const;

    /**
     * @brief Builds a host per entry, taking each panel's registration out of the main area as it goes. An
     * entry naming no panel the main area knows about is skipped, so a saved host of panels that no longer
     * exist does not leave an empty window behind.
     *
     * @param Layouts The hosts to build, which a load hands over without the main window.
     */
    void RestoreHostLayouts(const TArray<FDockWindowLayout>& Layouts);

    /**
     * @brief Finds which area holds a panel, the main one included.
     *
     * @param PanelId The panel to look for.
     * @return The area whose tree holds it, or null when nothing does.
     */
    NODISCARD TSharedPtr<FDockingArea> FindAreaForPanel(const String& PanelId) const;

    /**
     * @brief Gets whether any area holds a panel, which is what tells a torn-out panel from a closed one.
     *
     * @param PanelId The panel to look for.
     * @return True when the main area or a host holds it.
     */
    NODISCARD bool IsPanelDockedAnywhere(const String& PanelId) const;

    /**
     * @brief Gets whether a panel's content is on screen in any window.
     *
     * @param PanelId The panel to look for.
     * @return True when the area holding it shows it rather than another tab of its strip.
     */
    NODISCARD bool IsPanelVisibleAnywhere(const String& PanelId) const;

    /**
     * @brief Brings the tab of a panel torn out into a host forward, and the host's window with it, which is
     * what a menu naming an already-open panel should do rather than docking it a second time.
     *
     * @param PanelId The panel to show.
     * @return True when a host holds it, false when it is in the main area or nowhere.
     */
    bool FocusPanelInHost(const String& PanelId);

    /** @return The number of host windows open, which does not count the main window. */
    NODISCARD FORCEINLINE int32 GetNumHosts() const
    {
        return Hosts.Size();
    }

    /**
     * @brief Gets a host's window, so a caller can focus it or read where it sits.
     *
     * @param HostIndex Which host, in creation order.
     * @return The window, or null when the index is out of range.
     */
    NODISCARD TSharedPtr<FWindow> GetHostWindow(int32 HostIndex) const;

    /**
     * @brief Gets a host's area.
     *
     * @param HostIndex Which host, in creation order.
     * @return The area, or null when the index is out of range.
     */
    NODISCARD TSharedPtr<FDockingArea> GetHostArea(int32 HostIndex) const;

private:

    // A leaf offers five zones, and a drag rarely aims across more than one leaf before it lands
    static constexpr int32 MaxDropPreviews = 5;

    struct FDropPreview
    {
        FRHITextureRef      Texture;
        const FDockingArea* Area;
        String              PanelId;
        EDockDirection      Direction;
        IntVector2          Size;
    };

    struct FHost
    {
        FHost()
            : Window(nullptr)
            , Area(nullptr)
            , TitleBar(nullptr)
        {
        }

        TSharedPtr<FWindow>      Window;
        TSharedPtr<FDockingArea> Area;
        TSharedPtr<FTitleBar>    TitleBar;
    };

    NODISCARD static IntVector2 ResolveHostPosition(const IntVector2& ScreenPosition);
    NODISCARD static String ResolveHostTitle(const TSharedPtr<FDockingArea>& Area);

    static void ReleaseTextureDeferred(FRHITextureRef& Texture);

    NODISCARD int32 FindHostByArea(const FDockingArea* Area) const;
    NODISCARD int32 FindHostByWindow(const FWindow* Window) const;
    NODISCARD int32 FindDropPreview(const FDockingArea* Area, const String& PanelId, EDockDirection Direction, const IntVector2& Size) const;

    void OnDragBegan(const FDockDragPanel& Panel, const IntVector2& ScreenPosition);
    void OnDropOutside(const FDockDragPanel& Panel, const IntVector2& ScreenPosition);
    void OnHostWindowClosed(FWindow* Window);
    void SnapshotDecorator();
    void CloseHost(int32 HostIndex);
    void ClearDropPreview();
    void ReturnPanelRegistrationsToMainArea(const TSharedPtr<FDockingArea>& Area);

    FDesc                    Desc;
    TArray<FHost>            Hosts;
    TSharedPtr<FWindow>      DecoratorWindow;
    TSharedPtr<FDockingArea> DecoratorArea;
    FRHITextureRef           DecoratorSnapshot;
    IntVector2               DecoratorGrabOffset;
    TArray<FDropPreview>     DropPreviews;

    static TUniquePtr<FDockWindowManager> DockWindowManager;
};
