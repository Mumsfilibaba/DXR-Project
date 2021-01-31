#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"

#include <string>

enum EWindowStyleFlag : UInt32
{
    WindowStyleFlag_None        = 0x0,
    WindowStyleFlag_Titled      = FLAG(1),
    WindowStyleFlag_Closable    = FLAG(2),
    WindowStyleFlag_Minimizable = FLAG(3),
    WindowStyleFlag_Maximizable = FLAG(4),
    WindowStyleFlag_Resizeable  = FLAG(5),
};

struct WindowCreateInfo
{
public:
    WindowCreateInfo() = default;

    WindowCreateInfo(const std::string& InTitle, UInt32 InWidth, UInt32 InHeight, UInt32 InStyle)
        : Title(InTitle)
        , Width(InWidth)
        , Height(InHeight)
        , Style(InStyle)
    {
    }

    FORCEINLINE Bool IsTitled() const
    {
        return Style & WindowStyleFlag_Titled;
    }

    FORCEINLINE Bool IsClosable() const
    {
        return Style & WindowStyleFlag_Closable;
    }

    FORCEINLINE Bool IsMinimizable() const
    {
        return Style & WindowStyleFlag_Minimizable;
    }

    FORCEINLINE Bool IsMaximizable() const
    {
        return Style & WindowStyleFlag_Maximizable;
    }

    FORCEINLINE Bool IsResizeable() const
    {
        return Style & WindowStyleFlag_Resizeable;
    }

    std::string Title;
    UInt32 Width  = 0;
    UInt32 Height = 0;
    UInt32 Style  = 0;
};

struct WindowShape
{
    WindowShape() = default;

    WindowShape(UInt32 InWidth, UInt32 InHeight, Int32 x, Int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    {
    }

    UInt32 Width  = 0;
    UInt32 Height = 0;
    struct
    {
        Int32 x = 0;
        Int32 y = 0;
    } Position;
};

class GenericWindow : public RefCountedObject
{
public:
    virtual Bool Init(const WindowCreateInfo& InCreateInfo) = 0;

    virtual void Show(Bool Maximized) = 0;
    virtual void Minimize() = 0;
    virtual void Maximize() = 0;
    virtual void Close()    = 0;
    virtual void Restore()  = 0;
    virtual void ToggleFullscreen() = 0;

    virtual Bool IsValid() const        = 0;
    virtual Bool IsActiveWindow() const = 0;

    virtual void SetTitle(const std::string& Title) = 0;

    virtual void SetWindowShape(const WindowShape& Shape, Bool Move) = 0;
    virtual void GetWindowShape(WindowShape& OutWindowShape) const   = 0;

    virtual UInt32 GetWidth()  const = 0;
    virtual UInt32 GetHeight() const = 0;

    virtual Void* GetNativeHandle() const { return nullptr; }

    const WindowCreateInfo& GetCreateInfo() const { return CreateInfo; }

protected:
    WindowCreateInfo CreateInfo;
};