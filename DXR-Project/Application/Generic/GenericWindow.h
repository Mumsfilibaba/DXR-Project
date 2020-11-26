#pragma once
#include "Defines.h"
#include "Types.h"

#include <string>

/*
* EWindowStyleFlag
*/
enum EWindowStyleFlag : uint32
{
	WINDOW_STYLE_FLAG_NONE			= 0x00,
	WINDOW_STYLE_FLAG_TITLED		= FLAG(1),
	WINDOW_STYLE_FLAG_CLOSABLE		= FLAG(2),
	WINDOW_STYLE_FLAG_MINIMIZABLE	= FLAG(3),
	WINDOW_STYLE_FLAG_MAXIMIZABLE	= FLAG(4),
	WINDOW_STYLE_FLAG_RESIZEABLE	= FLAG(5),
};

/*
* WindowInitializer
*/
struct WindowInitializer
{
public:
	inline WindowInitializer()
		: Title()
		, Width(0)
		, Height(0)
		, Style(0)
	{
	}

	inline WindowInitializer(const std::string& InTitle, uint32 InWidth, uint32 InHeight, uint32 InStyle)
		: Title(InTitle)
		, Width(InWidth)
		, Height(InHeight)
		, Style(InStyle)
	{
	}

	FORCEINLINE bool IsTitled() const
	{
		return Style & WINDOW_STYLE_FLAG_TITLED;
	}

	FORCEINLINE bool IsClosable() const
	{
		return Style & WINDOW_STYLE_FLAG_CLOSABLE;
	}

	FORCEINLINE bool IsMaximizable() const
	{
		return Style & WINDOW_STYLE_FLAG_MAXIMIZABLE;
	}

	FORCEINLINE bool IsMinimizable() const
	{
		return Style & WINDOW_STYLE_FLAG_MINIMIZABLE;
	}

	FORCEINLINE bool IsResizeable() const
	{
		return Style & WINDOW_STYLE_FLAG_RESIZEABLE;
	}

	std::string Title;
	uint32 Width;
	uint32 Height;
	uint32 Style;
};

/*
* WindowShape
*/
struct WindowShape
{
	inline WindowShape()
		: Width(0)
		, Height(0)
		, Position({ 0, 0 })
	{
	}

	inline WindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
		: Width(InWidth)
		, Height(InHeight)
		, Position({ x, y })
	{
	}

	uint32 Width;
	uint32 Height;
	struct
	{
		int32 x;
		int32 y;
	} Position;
};

/*
* GenericWindow
*/
class GenericWindow : public RefCountedObject
{
public:
	GenericWindow()		= default;
	~GenericWindow()	= default;

	virtual bool Initialize(const WindowInitializer& InInitializer) = 0;

	virtual void Show(bool Maximized) = 0;
	virtual void Minimize() = 0;
	virtual void Maximize() = 0;
	virtual void Close() = 0;
	virtual void Restore() = 0;
	virtual void ToggleFullscreen() = 0;

	virtual bool IsValid() const = 0;
	virtual bool IsActiveWindow() const = 0;

	virtual void SetTitle(const std::string& Title) = 0;

	virtual void SetWindowShape(const WindowShape& Shape, bool Move)	= 0;
	virtual void GetWindowShape(WindowShape& OutWindowShape) const		= 0;

	virtual uint32 GetWidth()	const = 0;
	virtual uint32 GetHeight()	const = 0;

	virtual void* GetNativeHandle() const
	{
		return nullptr;
	}

	FORCEINLINE const WindowInitializer& GetInitializer() const
	{
		return Initializer;
	}

protected:
	WindowInitializer Initializer;
};