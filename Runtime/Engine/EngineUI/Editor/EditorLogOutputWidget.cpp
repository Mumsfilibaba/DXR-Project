#include "Engine/EngineUI/Editor/EditorLogOutputWidget.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include <imgui.h>

FEditorLogOutputWidget::FEditorLogOutputWidget()
    : IOutputDevice()
    , bVisible(true)
    , bAutoScroll(true)
    , bScrollToBottom(false)
    , bFilterInfo(true)
    , bFilterWarning(true)
    , bFilterError(true)
    , SearchFilterBuf()
    , Messages()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->RegisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorLogOutputWidget::Draw));
    }

    SearchFilterBuf.Fill(0);
}

FEditorLogOutputWidget::~FEditorLogOutputWidget()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->UnregisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorLogOutputWidget::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FEditorLogOutputWidget::Log(ELogSeverity Severity, const FString& Message)
{
    SCOPED_LOCK(MessagesCS);

    constexpr int32 MaxMessages = 5000;
    Messages.Emplace(FLogMessage{ Message, Severity });
    
    if (Messages.Size() > MaxMessages)
    {
        const int32 Overflow = Messages.Size() - MaxMessages;
        Messages.RemoveAt(0, Overflow);
    }
    
    if (bAutoScroll)
    {
        bScrollToBottom = true;
    }
}

void FEditorLogOutputWidget::DrawToolbar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Auto Scroll", nullptr, &bAutoScroll);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Filter"))
        {
            ImGui::Checkbox("Info", &bFilterInfo);
            ImGui::SameLine();
            ImGui::Checkbox("Warning", &bFilterWarning);
            ImGui::SameLine();
            ImGui::Checkbox("Error", &bFilterError);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::MenuItem("Copy All"))
            {
                ImGui::LogToClipboard();
                
                SCOPED_LOCK(MessagesCS);
                
                for (const FLogMessage& Message : Messages)
                {
                    ImGui::LogText("%s\n", *Message.Message);
                }

                ImGui::LogFinish();
            }
            
            if (ImGui::MenuItem("Clear"))
            {
                SCOPED_LOCK(MessagesCS);
                Messages.Clear();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void FEditorLogOutputWidget::DrawFilterBar()
{
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##LogSearch", "Search...", SearchFilterBuf.Data(), SearchFilterBuf.Size());
}

void FEditorLogOutputWidget::DrawLogList()
{
    const auto MatchesSeverity = [&](ELogSeverity Severity)
    {
		if (Severity == ELogSeverity::Info)
		{
            return bFilterInfo;
        }
        else if (Severity == ELogSeverity::Warning)
        {
            return bFilterWarning;
        }
        else if (Severity == ELogSeverity::Error)
        {
            return bFilterError;
        }

        return true;
    };
        
    // Copy under lock once per frame
    TArray<FLogMessage> Local;
    {
        SCOPED_LOCK(MessagesCS);
        Local = Messages;
    }
    
    ImGuiListClipper Clipper;
    Clipper.Begin(Local.Size());
        
    const bool bHasSearch = (SearchFilterBuf[0] != 0);
    const char* Search = SearchFilterBuf.Data();
    while (Clipper.Step())
    {
        for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
        {
            const FLogMessage& Message = Local[i];
            if (!MatchesSeverity(Message.Severity))
            {
                continue;
            }

            if (bHasSearch && FCString::Strstr(*Message.Message, Search) == nullptr)
            {
                continue;
            }

            ImVec4 TextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            switch (Message.Severity)
            {
                case ELogSeverity::Warning: 
                    TextColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    break;

                case ELogSeverity::Error:
                    TextColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    break;

                default: 
                    break;
            }

            ImGui::TextColored(TextColor, "%s", *Message.Message);
        }
    }

    if (bScrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        bScrollToBottom = false;
    }
}

void FEditorLogOutputWidget::Draw()
{
    if (!bVisible)
    {
        return;
    } 

    ImGuiWindowFlags Flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Output Log", &bVisible, Flags))
    {
        DrawToolbar();
        DrawFilterBar();

        const float FooterReserve = 0.0f; // pure output (no input line here)
        if (ImGui::BeginChild("##OutputLogScroll", ImVec2(0, -FooterReserve), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            DrawLogList();
        }
    
        ImGui::EndChild();
    }

    ImGui::End();
}
