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
public:
	TSharedRef<GenericWindow> MakeWindow();
	TSharedRef<GenericCursor> MakeCursor();

	bool Initialize(GenericApplication* InPlatformApplication);
	void Tick();

	void SetCursor(TSharedRef<GenericCursor> Cursor);
	void SetActiveWindow(TSharedRef<GenericWindow> Window);
	void SetCapture(TSharedRef<GenericWindow> Window);

	ModifierKeyState GetModifierKeyState() const;
	TSharedRef<GenericWindow> GetMainWindow() const;
	TSharedRef<GenericWindow> GetActiveWindow() const;
	TSharedRef<GenericWindow> GetCapture() const;

	void SetCursorPos(TSharedRef<GenericWindow> RelativeWindow, Int32 x, Int32 y);
	void GetCursorPos(TSharedRef<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const;
	
	void SetPlatformApplication(GenericApplication* InPlatformApplication);

	FORCEINLINE GenericApplication* GetPlatformApplication() const
	{
		return PlatformApplication;
	}

	static Application* Make();
	static Application& Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(TSharedRef<GenericWindow> InWindow, UInt16 Width, UInt16 Height) override;
	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseMove(Int32 x, Int32 y) override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta) override;
	virtual void OnCharacterInput(UInt32 Character) override;

protected:
	TSharedRef<GenericWindow> MainWindow;
	GenericApplication* PlatformApplication = nullptr;

	static TSharedPtr<Application> CurrentApplication;
};