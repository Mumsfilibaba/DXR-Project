#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FGenericWindow> FGenericWindowRef;

enum class EWindowStyleFlag : uint32
{
    None        = 0x0,
    Titled      = FLAG(1),
    Closable    = FLAG(2),
    Minimizable = FLAG(3),
    Maximizable = FLAG(4),
    Resizeable  = FLAG(5),
};

ENUM_CLASS_OPERATORS(EWindowStyleFlag);


struct FWindowStyle
{
    FWindowStyle() = default;

    FORCEINLINE FWindowStyle(EWindowStyleFlag InStyle)
        : Style(InStyle)
    { }

    FORCEINLINE bool IsTitled() const
    {
        return (Style & EWindowStyleFlag::Titled) != EWindowStyleFlag::None;
    }

    FORCEINLINE bool IsClosable() const
    {
        return (Style & EWindowStyleFlag::Closable) != EWindowStyleFlag::None;
    }

    FORCEINLINE bool IsMinimizable() const
    {
        return (Style & EWindowStyleFlag::Minimizable) != EWindowStyleFlag::None;
    }

    FORCEINLINE bool IsMaximizable() const
    {
        return (Style & EWindowStyleFlag::Maximizable) != EWindowStyleFlag::None;
    }

    FORCEINLINE bool IsResizeable() const
    {
        return (Style & EWindowStyleFlag::Resizeable) != EWindowStyleFlag::None;
    }

    EWindowStyleFlag Style = EWindowStyleFlag::None;
};


struct FWindowShape
{
    FWindowShape() = default;

    FORCEINLINE FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    { }

    uint32 Width  = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};


class FGenericWindow 
    : public FRefCounted
{
public:
    virtual ~FGenericWindow() = default;

    virtual bool Initialize(
        const FString& Title,
        uint32 InWidth,
        uint32 InHeight,
        int32 x,
        int32 y,
        FWindowStyle Style) { return true; }

    virtual void Show(bool bMaximized) { }

    virtual void Minimize() { }

    virtual void Maximize() { }

    virtual void Close() { }

    virtual void Restore() { }

    virtual void ToggleFullscreen() { }

    virtual bool IsActiveWindow() const { return false; }
    
    virtual bool IsValid() const { return false; }

    virtual void SetTitle(const FString& Title) { }
    
    virtual void GetTitle(FString& OutTitle) { }

    virtual void MoveTo(int32 x, int32 y) { }

    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) { }

    virtual void GetWindowShape(FWindowShape& OutWindowShape) const { }

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    virtual uint32 GetWidth() const { return 0; }

    virtual uint32 GetHeight() const { return 0; }

    virtual void  SetPlatformHandle(void* InPlatformHandle) { }
    
    virtual void* GetPlatformHandle() const { return nullptr; }

    FORCEINLINE FWindowStyle GetStyle() const { return StyleParams; }

protected:
    FWindowStyle StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
