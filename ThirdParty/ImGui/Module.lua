include "BuildTool_Module.lua"

-- ThirdParty ImGui Module
local ImGuiModule = ModuleBuildRules("ImGui")
ImGuiModule.bIsLibrary             = true
ImGuiModule.bIsDynamic             = false
ImGuiModule.bEnableRuntimeTypeInfo = false
ImGuiModule.bEnableEditAndContinue = false
ImGuiModule.bEnableIntrinsics      = true
ImGuiModule.bUsePrecompiledHeaders = false
ImGuiModule.bOptimizeDebugBuild    = true
ImGuiModule.bSilenceWarnings       = true
ImGuiModule.ExceptionHandling      = "Off"
ImGuiModule.FloatingPoint          = "Fast"
ImGuiModule.VectorExtensions       = "Default"
ImGuiModule.Language               = "C++"
ImGuiModule.CppVersion             = "C++20"
ImGuiModule.SystemVersion          = "latest"
ImGuiModule.CharacterSet           = "Ascii"

ImGuiModule.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Add the correct folder as an include dir to make includes simpler (#include <imgui.h> instead of #include <imgui/imgui.h>)
ImGuiModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("ImGui/imgui/"),
})

-- Source files
ImGuiModule.AddFiles({
    CreateExternalThirdpartyPath("ImGui/imgui/imconfig.h"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui.h"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui.cpp"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui_demo.cpp"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui_draw.cpp"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui_internal.h"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui_tables.cpp"),
    CreateExternalThirdpartyPath("ImGui/imgui/imgui_widgets.cpp"),
    CreateExternalThirdpartyPath("ImGui/imgui/imstb_rectpack.h"),
    CreateExternalThirdpartyPath("ImGui/imgui/imstb_textedit.h"),
    CreateExternalThirdpartyPath("ImGui/imgui/imstb_truetype.h"),
})
