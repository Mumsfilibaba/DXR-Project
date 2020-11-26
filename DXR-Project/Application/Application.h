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
	~Application();

	TSharedRef<GenericWindow> MakeWindow();
	TSharedRef<GenericCursor> MakeCursor();

	bool Initialize(TSharedPtr<GenericApplication> InPlatformApplication);
	void Tick();
	void Release();

	void SetCursor(TSharedRef<GenericCursor> Cursor);
	void SetActiveWindow(TSharedRef<GenericWindow> Window);
	void SetCapture(TSharedRef<GenericWindow> Window);

	ModifierKeyState GetModifierKeyState() const;
	TSharedRef<GenericWindow> GetMainWindow() const;
	TSharedRef<GenericWindow> GetActiveWindow() const;
	TSharedRef<GenericWindow> GetCapture() const;

	void SetCursorPos(TSharedRef<GenericWindow> RelativeWindow, int32 x, int32 y);
	void GetCursorPos(TSharedRef<GenericWindow> RelativeWindow, int32& OutX, int32& OutY) const;
	
	void SetPlatformApplication(TSharedPtr<GenericApplication> InPlatformApplication);

	FORCEINLINE TSharedPtr<GenericApplication> GetPlatformApplication() const
	{
		return PlatformApplication;
	}

	static Application* Make();
	static Application& Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(TSharedRef<GenericWindow> InWindow, uint16 Width, uint16 Height) override;
	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseMove(int32 x, int32 y) override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) override;
	virtual void OnCharacterInput(uint32 Character) override;

private:
	Application();

protected:
	TSharedRef<GenericWindow> MainWindow;
	TSharedPtr<GenericApplication> PlatformApplication = nullptr;

	static TSharedPtr<Application> CurrentApplication;
};