include "BuildTool_Common.lua"
include "BuildTool_Log.lua"
include "BuildTool_Module.lua"
include "BuildTool_Rule.lua"
include "BuildTool_Shader.lua"
include "BuildTool_Target.lua"
include "BuildTool_Version.lua"
include "BuildTool_Workspace.lua"

-- Default roots for Module.lua files: Runtime and ThirdParty
AddModuleSearchRoot(GetRuntimeFolderPath())
AddModuleSearchRoot(GetExternalThirdPartyFolderPath())
