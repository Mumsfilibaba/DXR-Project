include '../../BuildScripts/Scripts/enginebuild.lua'

local RHIModule = CreateModule( 'RHI' )
RHIModule.ModuleDependencies = 
{
    'Core',
}

if BuildWithXcode() then
    RHIModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

RHIModule:Generate()
