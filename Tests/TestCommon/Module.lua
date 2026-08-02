include "BuildTool.lua"

-- TestCommon Module
local TestCommon = ModuleBuildRules("TestCommon")
TestCommon.bIsLibrary             = true
TestCommon.bIsDynamic             = false
TestCommon.bUsePrecompiledHeaders = false

TestCommon.SetGroup("Tests")

TestCommon.AddModules({
    "Core",
})
