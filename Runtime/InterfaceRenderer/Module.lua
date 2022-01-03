include '../../BuildScripts/Scripts/enginebuild.lua'

local InterfaceRendererModule = CreateModule( 'InterfaceRenderer' )
InterfaceRendererModule.SysIncludes = 
{
    '%{wks.location}/Dependencies/imgui',
}

InterfaceRendererModule.ModuleDependencies = 
{
    'Core',
    'CoreApplication',
    'Interface',
    'Engine',
    'RHI',
}

InterfaceRendererModule.LinkLibraries = 
{
    'ImGui',
}

if BuildWithXcode() then
    InterfaceRendererModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

InterfaceRendererModule:Generate()
