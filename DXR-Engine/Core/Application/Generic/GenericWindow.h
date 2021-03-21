#pragma once
#include "Core/RefCountedObject.h"

#include <string>

enum EWindowStyleFlag : uint32
{
    WindowStyleFlag_None        = 0x0,
    WindowStyleFlag_Titled      = FLAG(1),
    WindowStyleFlag_Closable    = FLAG(2),
    WindowStyleFlag_Minimizable = FLAG(3),
    WindowStyleFlag_Maximizable = FLAG(4),
    WindowStyleFlag_Resizeable  = FLAG(5),
};

struct WindowStyle
{
    WindowStyle() = default;

    WindowStyle(uint32 InStyle)
        : Style(InStyle)
    {
    }

    bool IsTitled() const { return Style & WindowStyleFlag_Titled; }
    bool IsClosable() const { return Style & WindowStyleFlag_Closable; }
    bool IsMinimizable() const { return Style & WindowStyleFlag_Minimizable; }
    bool IsMaximizable() const { return Style & WindowStyleFlag_Maximizable; }
    bool IsResizeable() const { return Style & WindowStyleFlag_Resizeable; }

    uint32 Style = 0;
};

struct WindowShape
{
    WindowShape() = default;

    WindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    {
    }

    uint32 Width  = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};

class GenericWindow : public RefCountedObject
{
public:
    virtual void Show(bool Maximized) = 0;
    virtual void Minimize() = 0;
    virtual void Maximize() = 0;
    virtual void Close() = 0;
    virtual void Restore() = 0;
    virtual void ToggleFullscreen() = 0;

    virtual bool IsValid() const = 0;
    virtual bool IsActiveWindow() const = 0;

    virtual void SetTitle(const std::string& Title) = 0;
    virtual void GetTitle(std::string& OutTitle) = 0;

    virtual void SetWindowShape(const WindowShape& Shape, bool Move) = 0;
    virtual void GetWindowShape(WindowShape& OutWindowShape) const = 0;

    virtual uint32 GetWidth()  const = 0;
    virtual uint32 GetHeight() const = 0;

    virtual void* GetNativeHandle() const { return nullptr; }

    WindowStyle GetStyle() const { return WndStyle; }

protected:
    WindowStyle WndStyle;
};