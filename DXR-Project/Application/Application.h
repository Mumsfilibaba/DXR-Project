#pragma once
#include "Time/Clock.h"

#include "Application/Generic/GenericApplication.h"
#include "Application/Generic/GenericCursor.h"
#include "Application/Generic/GenericWindow.h"
#include "Application/Events/ApplicationEventHandler.h"

/*
* Application
*/

class Application : public ApplicationEventHandler
{
	Application(GenericApplication* InPlatformApplication);

public:
	FORCEINLINE TSharedRef<GenericWindow> MakeWindow()
	{
		return PlatformApplication->MakeWindow();
	}

	FORCEINLINE TSharedRef<GenericCursor> MakeCursor()
	{
		return PlatformApplication->MakeCursor();
	}

	bool Initialize();
	void Tick();

	FORCEINLINE void SetCursor(TSharedRef<GenericCursor> Cursor)
	{
		PlatformApplication->SetCursor(Cursor);
	}

	FORCEINLINE void SetActiveWindow(TSharedRef<GenericWindow> Window)
	{
		PlatformApplication->SetActiveWindow(Window);
	}

	FORCEINLINE void SetCapture(TSharedRef<GenericWindow> Window)
	{
		PlatformApplication->SetCapture(Window);
	}

	FORCEINLINE TSharedRef<GenericWindow> GetMainWindow()	const
	{
		return MainWindow;
	}

	FORCEINLINE TSharedRef<GenericWindow> GetActiveWindow() const
	{
		return PlatformApplication->GetActiveWindow();
	}

	FORCEINLINE ModifierKeyState GetModifierKeyState() const
	{
		return PlatformApplication->GetModifierKeyState();
	}

	FORCEINLINE TSharedRef<GenericWindow> GetCapture() const
	{
		return PlatformApplication->GetCapture();
	}

	FORCEINLINE void SetCursorPos(TSharedRef<GenericWindow> RelativeWindow, Int32 x, Int32 y)
	{
		PlatformApplication->SetCursorPos(RelativeWindow, x, y);
	}

	FORCEINLINE void GetCursorPos(TSharedRef<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const
	{
		PlatformApplication->GetCursorPos(RelativeWindow, OutX, OutY);
	}

	void SetPlatformApplication(GenericApplication* InPlatformApplication);

	FORCEINLINE GenericApplication* GetPlatformApplication() const
	{
		return PlatformApplication;
	}

	static Application* Make(GenericApplication* InPlatformApplication);

	static FORCEINLINE Application& Get()
	{
		VALIDATE(Instance != nullptr);
		return *Instance;
	}

public:
	// EventHandler Interface
	virtual void OnWindowResized(TSharedRef<GenericWindow> InWindow, UInt16 Width, UInt16 Height) override;
	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)		override;
	virtual void OnMouseMove(Int32 x, Int32 y) override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta) override;
	virtual void OnCharacterInput(UInt32 Character) override;

protected:
	TSharedRef<GenericWindow> MainWindow;
	GenericApplication* PlatformApplication = nullptr;

	static TSharedPtr<Application> Instance;
};