#pragma once
#include "Clock.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsCursor.h"
#include "Windows/WindowsApplication.h"

#include "Scene/Scene.h"

#include "Application/Events/ApplicationEventHandler.h"
#include "Application/Generic/GenericApplication.h"

/*
* Application
*/
class Application : public ApplicationEventHandler
{
public:
	~Application();

	void Release();

	void Tick();

	void SetCursor(TSharedPtr<WindowsCursor> Cursor);
	void SetActiveWindow(TSharedPtr<WindowsWindow>& ActiveWindow);
	void SetCapture(TSharedPtr<WindowsWindow> Capture);

	ModifierKeyState			GetModifierKeyState()	const;
	TSharedPtr<WindowsWindow>	GetWindow()				const;
	TSharedPtr<WindowsWindow>	GetActiveWindow()		const;
	TSharedPtr<WindowsWindow>	GetCapture()			const;

	void SetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y);
	void GetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const;
	
	static Application* Make();
	static Application* Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(TSharedPtr<WindowsWindow>& InWindow, Uint16 Width, Uint16 Height)		override;
	virtual void OnKeyReleased(EKey InKeyCode, const ModifierKeyState& ModierKeyState)					override;
	virtual void OnKeyPressed(EKey InKeyCode, const ModifierKeyState& ModierKeyState)					override;
	virtual void OnMouseMove(Int32 X, Int32 Y)															override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)		override;
	virtual void OnMouseButtonPressed(EMouseButton InButton, const ModifierKeyState& ModierKeyState)	override;
	virtual void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)						override;
	virtual void OnCharacterInput(Uint32 Character)														override;

private:
	Application();

	bool Initialize();

protected:
	TSharedPtr<WindowsWindow> Window		= nullptr;
	WindowsApplication*	PlatformApplication = nullptr;

	Scene*	CurrentScene	= nullptr;
	Camera* CurrentCamera	= nullptr;

	static TSharedPtr<Application> Instance;
};