include "BuildTool.lua"

-- Application Module

local ApplicationModule = ModuleBuildRules("Application")
ApplicationModule.bUsePrecompiledHeaders = true

ApplicationModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("stb_truetype"),
})

ApplicationModule.AddModules({
    "Core",
    "CoreApplication",
    "RHI",
})