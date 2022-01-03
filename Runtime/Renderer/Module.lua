include '../../BuildScripts/Scripts/enginebuild.lua'

local RendererModule = CreateModule( 'Renderer' )
RendererModule.SysIncludes = 
{
    '%{wks.location}/Dependencies/imgui',
}

RendererModule.ModuleDependencies = 
{
    'Core',
    'CoreApplication',
    'Interface',
    'RHI',
    'Engine',
}

RendererModule.LinkLibraries = 
{
    'ImGui',
}

if BuildWithXcode() then
    RendererModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

RendererModule:Generate()
