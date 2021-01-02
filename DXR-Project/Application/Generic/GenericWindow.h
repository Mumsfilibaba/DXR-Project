#pragma once
#include "Defines.h"

#include <string>

/*
* EWindowStyleFlag
*/

enum EWindowStyleFlag : UInt32
{
	WindowStyleFlag_None		= 0x0,
	WindowStyleFlag_Titled		= FLAG(1),
	WindowStyleFlag_Closable	= FLAG(2),
	WindowStyleFlag_Minimizable	= FLAG(3),
	WindowStyleFlag_Maximizable	= FLAG(4),
	WindowStyleFlag_Resizeable	= FLAG(5),
};

/*
* WindowInitializer
*/

struct WindowInitializer
{
public:
	WindowInitializer() = default;

	inline WindowInitializer(const std::string& InTitle, UInt32 InWidth, UInt32 InHeight, UInt32 InStyle)
		: Title(InTitle)
		, Width(InWidth)
		, Height(InHeight)
		, Style(InStyle)
	{
	}

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

	std::string Title;
	UInt32 Width	= 0;
	UInt32 Height	= 0;
	UInt32 Style	= 0;
};

/*
* WindowShape
*/

struct WindowShape
{
	WindowShape() = default;

	inline WindowShape(UInt32 InWidth, UInt32 InHeight, Int32 x, Int32 y)
		: Width(InWidth)
		, Height(InHeight)
		, Position({ x, y })
	{
	}

	UInt32 Width	= 0;
	UInt32 Height	= 0;
	struct
	{
		Int32 x = 0;
		Int32 y = 0;
	} Position;
};

/*
* GenericWindow
*/

class GenericWindow : public RefCountedObject
{
public:
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

	virtual UInt32 GetWidth()	const = 0;
	virtual UInt32 GetHeight()	const = 0;

	virtual Void* GetNativeHandle() const
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