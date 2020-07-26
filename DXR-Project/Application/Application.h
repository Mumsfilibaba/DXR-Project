#pragma once
#include "EventHandler.h"
#include "Clock.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsCursor.h"
#include "Windows/WindowsApplication.h"

#include "Scene/Scene.h"

#include <memory>

class Application : public EventHandler
{
public:
	~Application();

	void Release();

	void Run();
	bool Tick();

	void DrawDebugData();
	void DrawRenderSettings();

	void SetCursor(std::shared_ptr<WindowsCursor> Cursor);
	void SetActiveWindow(std::shared_ptr<WindowsWindow>& ActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow> Capture);

	ModifierKeyState				GetModifierKeyState()	const;
	std::shared_ptr<WindowsWindow>	GetWindow()				const;
	std::shared_ptr<WindowsWindow>	GetActiveWindow()		const;
	std::shared_ptr<WindowsWindow>	GetCapture()			const;

	void SetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y);
	void GetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const;
	
	static Application* Make();
	static Application* Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(std::shared_ptr<WindowsWindow>& InWindow, Uint16 Width, Uint16 Height)		override;
	virtual void OnKeyReleased(EKey InKeyCode, const ModifierKeyState& ModierKeyState)						override;
	virtual void OnKeyPressed(EKey InKeyCode, const ModifierKeyState& ModierKeyState)						override;
	virtual void OnMouseMove(Int32 X, Int32 Y)																override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)			override;
	virtual void OnMouseButtonPressed(EMouseButton InButton, const ModifierKeyState& ModierKeyState)		override;
	virtual void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)							override;
	virtual void OnCharacterInput(Uint32 Character)															override;

private:
	Application();

	bool Initialize();

protected:
	std::shared_ptr<WindowsWindow> Window = nullptr;
	WindowsApplication*	PlatformApplication = nullptr;

	Clock Timer;
	Scene* CurrentScene = nullptr;

	static std::shared_ptr<Application> Instance;
};