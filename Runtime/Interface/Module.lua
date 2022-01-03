include '../../BuildScripts/Scripts/enginebuild.lua'

local InterfaceModule = CreateModule( 'Interface' )
InterfaceModule.SysIncludes = 
{
    '%{wks.location}/Dependencies/imgui',
}

InterfaceModule.ModuleDependencies = 
{
    'Core',
    'CoreApplication',
}

InterfaceModule.LinkLibraries = 
{
    'ImGui',
}

if BuildWithXcode() then
    InterfaceModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

InterfaceModule:Generate()
