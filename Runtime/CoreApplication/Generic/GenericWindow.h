#pragma once
#include "Core/RefCounted.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

enum class EWindowStyleFlags : uint16
{
    None          = 0,
    Titled        = FLAG(1),
    Closable      = FLAG(2),
    Minimizable   = FLAG(3),
    Maximizable   = FLAG(4),
    Resizeable    = FLAG(5),
    NoTaskBarIcon = FLAG(6),
    TopMost       = FLAG(7),
    Opaque        = FLAG(8),

    Default = Titled | Maximizable | Minimizable | Resizeable | Closable | Opaque
};

ENUM_CLASS_OPERATORS(EWindowStyleFlags);

struct FWindowShape
{
    FWindowShape()
        : Width(0)
        , Height(0)
        , Position(0, 0)
    {
    }

    FWindowShape(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
        , Position(0, 0)
    {
    }

    FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position(x, y)
    {
    }

    bool operator==(const FWindowShape& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    bool operator!=(const FWindowShape& Other) const
    {
        return !(*this == Other);
    }

    uint32      Width;
    uint32      Height;
    FIntVector2 Position;
};

struct FGenericWindowInitializer
{
    FGenericWindowInitializer()
        : Title()
        , Width(1280)
        , Height(720)
        , Position(0, 0)
        , Style(EWindowStyleFlags::Default)
        , ParentWindow(nullptr)
    {
    }

    FString           Title;
    uint32            Width;
    uint32            Height;
    FIntVector2       Position;
    EWindowStyleFlags Style;
    FGenericWindow*   ParentWindow;
};

class FGenericWindow : public FRefCounted
{
public:
    virtual ~FGenericWindow() = default;

    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) { return true; }
    virtual void Show(bool bFocus = true) { }
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
    virtual void SetStyle(EWindowStyleFlags Style) { }

    EWindowStyleFlags GetStyle() const 
    { 
        return StyleParams;
    }

protected:
    EWindowStyleFlags StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
