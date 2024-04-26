#pragma once
#include "Core/RefCounted.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

enum class EWindowStyleFlag : uint16
{
    None          = 0,
    Titled        = FLAG(1),
    Closable      = FLAG(2),
    Minimizable   = FLAG(3),
    Maximizable   = FLAG(4),
    Resizeable    = FLAG(5),
    NoTaskBarIcon = FLAG(6),
    TopMost       = FLAG(7)
};

ENUM_CLASS_OPERATORS(EWindowStyleFlag);

// TODO: Change to use WindowMode instead of specifying styles separately
enum class EWindowMode : uint8
{
    None       = 0,
    Windowed   = 1,
    Borderless = 2,
    Fullscreen = 3,
};

struct FWindowStyle
{
    static FWindowStyle Default()
    {
        return FWindowStyle(EWindowStyleFlag::Titled | EWindowStyleFlag::Maximizable | EWindowStyleFlag::Minimizable | EWindowStyleFlag::Resizeable | EWindowStyleFlag::Closable);
    }

    constexpr FWindowStyle() = default;

    constexpr FWindowStyle(EWindowStyleFlag InStyle)
        : Style(InStyle)
    {
    }

    constexpr bool IsTitled() const
    {
        return (Style & EWindowStyleFlag::Titled) != EWindowStyleFlag::None;
    }

    constexpr bool IsClosable() const
    {
        return (Style & EWindowStyleFlag::Closable) != EWindowStyleFlag::None;
    }

    constexpr bool IsMinimizable() const
    {
        return (Style & EWindowStyleFlag::Minimizable) != EWindowStyleFlag::None;
    }

    constexpr bool IsMaximizable() const
    {
        return (Style & EWindowStyleFlag::Maximizable) != EWindowStyleFlag::None;
    }

    constexpr bool IsResizeable() const
    {
        return (Style & EWindowStyleFlag::Resizeable) != EWindowStyleFlag::None;
    }

    constexpr bool HasTaskBarIcon() const
    {
        return (Style & EWindowStyleFlag::NoTaskBarIcon) == EWindowStyleFlag::None;
    }

    constexpr bool IsTopMost() const
    {
        return (Style & EWindowStyleFlag::TopMost) != EWindowStyleFlag::None;
    }

    constexpr bool operator==(FWindowStyle Other) const
    {
        return Style == Other.Style;
    }

    constexpr bool operator!=(FWindowStyle Other) const
    {
        return Style != Other.Style;
    }
    
    constexpr bool operator==(EWindowStyleFlag Other) const
    {
        return Style == Other;
    }

    constexpr bool operator!=(EWindowStyleFlag Other) const
    {
        return Style != Other;
    }

    EWindowStyleFlag Style = EWindowStyleFlag::None;
};

struct FWindowShape
{
    FWindowShape() = default;

    FWindowShape(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ 0, 0 })
    {
    }

    FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    {
    }

    bool operator==(FWindowShape Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position.x == Other.Position.x && Position.y == Other.Position.y;
    }

    bool operator!=(FWindowShape Other) const
    {
        return !(*this == Other);
    }

    uint32 Width  = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};

struct FGenericWindowInitializer
{
    FGenericWindowInitializer()
        : Title()
        , Width(1280)
        , Height(720)
        , Position(0, 0)
        , Style(FWindowStyle::Default())
        , ParentWindow(nullptr)
    {
    }
    
    bool operator==(const FGenericWindowInitializer& Other) const
    {
        return Title == Other.Title && Width == Other.Width && Height == Other.Height && Position == Other.Position && Style == Other.Style && ParentWindow == Other.ParentWindow;
    }

    bool operator!=(const FGenericWindowInitializer& Other) const
    {
        return !(*this == Other);
    }

    FString         Title;
    uint32          Width;
    uint32          Height;
    FIntVector2     Position;
    FWindowStyle    Style;
    FGenericWindow* ParentWindow;
};

class FGenericWindow : public FRefCounted
{
public:
    virtual ~FGenericWindow() = default;

    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) { return true; }
    virtual void Show(bool bFocusOnActivate = true) { }
    virtual void Minimize() { }
    virtual void Maximize() { }
    virtual void Destroy() { }
    virtual void Restore() { }
    virtual void ToggleFullscreen() { }
    virtual bool IsActiveWindow() const { return false; }
    virtual void SetWindowPos(int32 x, int32 y) { }
    virtual bool IsValid() const { return false; }
    virtual bool IsMinimized() const { return false; }
    virtual bool IsMaximized() const { return false; }
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ParentWindow) const { return false; }
    virtual void SetWindowFocus() { }
    virtual void SetTitle(const FString& Title) { }
    virtual void GetTitle(FString& OutTitle) const { }
    virtual void SetWindowOpacity(float Alpha) { }
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) { }
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const { }
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }
    virtual float GetWindowDpiScale() const { return 0.0f; }
    virtual uint32 GetWidth() const { return 0; }
    virtual uint32 GetHeight() const { return 0; }
    virtual void SetPlatformHandle(void* InPlatformHandle) { }  
    virtual void* GetPlatformHandle() const { return nullptr; }
    virtual void SetStyle(FWindowStyle Style) { }

    FWindowStyle GetStyle() const 
    { 
        return StyleParams;
    }

protected:
    FWindowStyle StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
