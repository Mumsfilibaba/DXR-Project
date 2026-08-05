include "BuildTool.lua"

local BlueNoiseGen = TargetBuildRules("BlueNoiseGen")
BlueNoiseGen.TargetType = ETargetType.Program
BlueNoiseGen.Kind       = "ConsoleApp"

BlueNoiseGen.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("tinyddsloader"),
})

BlueNoiseGen.AddModules({
    "Core",
})
