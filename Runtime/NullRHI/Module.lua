include '../../BuildScripts/Scripts/enginebuild.lua'

local NullRHIModule = CreateModule( 'NullRHI' )
NullRHIModule.ModuleDependencies = 
{
    'Core',
    'RHI',
}

if BuildWithXcode() then
    NullRHIModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

NullRHIModule:Generate()
