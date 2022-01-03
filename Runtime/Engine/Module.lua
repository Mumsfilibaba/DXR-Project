include '../../BuildScripts/Scripts/enginebuild.lua'

local EngineModule = CreateModule( 'Engine' )
EngineModule.SysIncludes = 
{
    '%{wks.location}/Dependencies/imgui',
    '%{wks.location}/Dependencies/stb_image',
    '%{wks.location}/Dependencies/tinyobjloader',
    '%{wks.location}/Dependencies/OpenFBX/src',
}

EngineModule.ModuleDependencies = 
{
    'Core',
    'CoreApplication',
    'Interface',
    'RHI',
}

EngineModule.LinkLibraries = 
{
    'ImGui',
    'tinyobjloader',
    'OpenFBX',
}

if BuildWithXcode() then
    EngineModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

EngineModule:Generate()
