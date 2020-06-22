#pragma once
#include "EventHandler.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsCursor.h"
#include "Windows/WindowsApplication.h"

#include <memory>

class Application : public EventHandler
{
public:
	~Application();

	bool Tick();

	void SetCursor(std::shared_ptr<WindowsCursor> InCursor);
	void SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow> InCapture);

	ModifierKeyState				GetModifierKeyState()	const;
	std::shared_ptr<WindowsWindow>	GetWindow()				const;
	std::shared_ptr<WindowsWindow>	GetActiveWindow()		const;
	std::shared_ptr<WindowsWindow>	GetCapture()			const;

	void SetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32 InX, Int32 InY);
	void GetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32& OutX, Int32& OutY) const;
	
	static Application* Create();
	static Application* Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)		override;
	virtual void OnKeyReleased(EKey InKeyCode, const ModifierKeyState& InModierKeyState)						override;
	virtual void OnKeyPressed(EKey InKeyCode, const ModifierKeyState& InModierKeyState)							override;
	virtual void OnMouseMove(Int32 InX, Int32 InY)																override;
	virtual void OnMouseButtonReleased(EMouseButton InButton, const ModifierKeyState& InModierKeyState)			override;
	virtual void OnMouseButtonPressed(EMouseButton InButton, const ModifierKeyState& InModierKeyState)			override;
	virtual void OnMouseScrolled(Float32 InHorizontalDelta, Float32 InVerticalDelta)							override;
	virtual void OnCharacterInput(Uint32 Character)																override;

private:
	Application();

	bool Initialize();

protected:
	std::shared_ptr<WindowsWindow>	Window				= nullptr;
	WindowsApplication*				PlatformApplication = nullptr;

	static std::shared_ptr<Application> Instance;
};