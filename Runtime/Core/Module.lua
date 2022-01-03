include '../../BuildScripts/Scripts/enginebuild.lua'

local CoreModule = CreateModule( 'Core' )
CoreModule.bUsePrecompiledHeaders = true

CoreModule:Generate()