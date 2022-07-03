#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Templates/EnumUtilities.h"

#include "CoreApplication/CoreApplication.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EWindowStyleFlag - Window style flags

enum EWindowStyleFlag : uint32
{
    WindowStyleFlag_None        = 0x0,
    WindowStyleFlag_Titled      = FLAG(1),
    WindowStyleFlag_Closable    = FLAG(2),
    WindowStyleFlag_Minimizable = FLAG(3),
    WindowStyleFlag_Maximizable = FLAG(4),
    WindowStyleFlag_Resizeable  = FLAG(5),
};

ENUM_CLASS_OPERATORS(EWindowStyleFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowStyle

struct SWindowStyle
{
    SWindowStyle() = default;

    FORCEINLINE SWindowStyle(uint32 InStyle)
        : Style(InStyle)
    { }

    FORCEINLINE bool IsTitled() const
    {
        return Style & WindowStyleFlag_Titled;
    }

    FORCEINLINE bool IsClosable() const
    {
        return Style & WindowStyleFlag_Closable;
    }

    FORCEINLINE bool IsMinimizable() const
    {
        return Style & WindowStyleFlag_Minimizable;
    }

    FORCEINLINE bool IsMaximizable() const
    {
        return Style & WindowStyleFlag_Maximizable;
    }

    FORCEINLINE bool IsResizeable() const
    {
        return Style & WindowStyleFlag_Resizeable;
    }

    uint32 Style = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowShape

struct SWindowShape
{
    SWindowShape() = default;

    FORCEINLINE SWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    { }

    uint32 Width = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericWindow

class FGenericWindow : public FRefCounted
{
protected:

    FGenericWindow()  = default;
    ~FGenericWindow() = default;

public:

    virtual bool Initialize(const FString& Title, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) { return true; }

    virtual void Show(bool bMaximized) { }

    virtual void Minimize() { }

    virtual void Maximize() { }

    virtual void Close() { }

    virtual void Restore() { }

    virtual void ToggleFullscreen() { }

    virtual bool IsValid() const { return false; }

    virtual bool IsActiveWindow() const { return false; }

    virtual void SetTitle(const FString& Title) { }

    virtual void GetTitle(FString& OutTitle) { }

    virtual void MoveTo(int32 x, int32 y) { }

    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) { }

    virtual void GetWindowShape(SWindowShape& OutWindowShape) const { }

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    virtual uint32 GetWidth() const { return 0; }

    virtual uint32 GetHeight() const { return 0; }

    virtual void SetPlatformHandle(void* InPlatformHandle) { }

    virtual void* GetPlatformHandle() const { return nullptr; }

    FORCEINLINE SWindowStyle GetStyle() const { return StyleParams; }

protected:
    SWindowStyle StyleParams;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
