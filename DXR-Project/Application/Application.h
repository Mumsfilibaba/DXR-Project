#pragma once
#include "Clock.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsCursor.h"
#include "Windows/WindowsApplication.h"

#include "Scene/Scene.h"

#include "Application/Events/ApplicationEventHandler.h"

#include "Application/Generic/GenericApplication.h"
#include "Application/Generic/GenericCursor.h"
#include "Application/Generic/GenericWindow.h"

/*
* Application
*/
class Application : public ApplicationEventHandler
{
public:
	~Application();

	bool Initialize();
	void Release();

	void Tick();

	void SetCursor(TSharedPtr<GenericCursor> Cursor);
	void SetActiveWindow(TSharedPtr<GenericWindow> Window);
	void SetCapture(TSharedPtr<GenericWindow> Window);

	ModifierKeyState GetModifierKeyState() const;
	TSharedPtr<GenericWindow> GetWindow() const;
	TSharedPtr<GenericWindow> GetActiveWindow() const;
	TSharedPtr<GenericWindow> GetCapture() const;

	void SetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32 x, Int32 y);
	void GetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const;
	
	static Application* Make();
	static Application& Get();

public:
	// EventHandler Interface
	virtual void OnWindowResized(TSharedPtr<GenericWindow> InWindow, Uint16 Width, Uint16 Height) override;
	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseMove(Int32 x, Int32 y) override;
	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override;
	virtual void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta) override;
	virtual void OnCharacterInput(Uint32 Character) override;

private:
	Application();

protected:
	TSharedPtr<GenericWindow> Window;
	GenericApplication*	PlatformApplication = nullptr;

	Scene*	CurrentScene	= nullptr;
	Camera* CurrentCamera	= nullptr;

	static TSharedPtr<Application> Instance;
};