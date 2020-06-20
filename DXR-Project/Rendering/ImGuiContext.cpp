#include "ImGuiContext.h"

std::unique_ptr<GuiContext> GuiContext::Instance = nullptr;

GuiContext::GuiContext()
{
}

GuiContext::~GuiContext()
{
}

GuiContext* GuiContext::Create()
{
	Instance = std::unique_ptr<GuiContext>(new GuiContext());
	if (Instance->Initialize())
	{
		return Instance.get();
	}
	else
	{
		return nullptr;
	}
}

GuiContext* GuiContext::Get()
{
	return Instance.get();
}

void GuiContext::BeginFrame()
{
}

void GuiContext::EndFrame()
{
}

bool GuiContext::Initialize()
{
	IMGUI_CHECKVERSION();
	Context = ImGui::CreateContext();
	if (!Context)
	{
		return false;
	}

	return true;
}
