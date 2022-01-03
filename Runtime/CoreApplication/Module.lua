include '../../BuildScripts/Scripts/enginebuild.lua'

local CoreApplicationModule = CreateModule( 'CoreApplication' )
CoreApplicationModule.ModuleDependencies = 
{
    'Core'
}

if BuildWithXcode() then
    CoreApplicationModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

CoreApplicationModule:Generate()
