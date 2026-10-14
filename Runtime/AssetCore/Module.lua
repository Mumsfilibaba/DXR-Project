include "BuildTool.lua"

-- AssetCore Module

local AssetCoreModule = ModuleBuildRules("AssetCore")

AssetCoreModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("stb_image"),
})

AssetCoreModule.AddModules({
    "Core",
    "OpenFBX",
    "tinyobjloader",
})
