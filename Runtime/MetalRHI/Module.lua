include "BuildTool.lua"

-- MetalRHI Module

if IsPlatformMac() then
    local MetalRHI = ModuleBuildRules("MetalRHI")
    MetalRHI.bRuntimeLinking        = true
    MetalRHI.bUsePrecompiledHeaders = true
    
    MetalRHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
        "ShaderCore",
    })

    MetalRHI.AddFrameworks({
        "CoreGraphics",
        "Metal",
        "QuartzCore",
    })

    -- The internal shaders are compiled into headers in Generated/ before the module builds. The ShaderCompiler tool is
    -- built in the tools workspace first, and only rewrites a header when something it reads has changed.
    local ShaderCommands =
    {
        GetShaderCompilerToolBuildCommand(),
    }

    local function AddClearShader(Source, Name, Defines)
        table.insert(ShaderCommands, GetShaderHeaderCommand(MetalRHI, {
            Source  = "Shaders/Internal/" .. Source .. ".hlsl",
            Entry   = "Main",
            Stage   = "Compute",
            Model   = "SM_6_2",
            RHI     = "Metal",
            Defines = Defines,
            Header  = "Generated/" .. Source .. "_" .. Name .. ".h",
            Symbol  = "GMetal" .. Source .. "_" .. Name,
        }))
    end

    local ElementTypes =
    {
        { Name = "Float", Defines = { "CLEAR_ELEMENT_UINT=0", "CLEAR_ELEMENT_SINT=0" } },
        { Name = "Uint",  Defines = { "CLEAR_ELEMENT_UINT=1", "CLEAR_ELEMENT_SINT=0" } },
        { Name = "Sint",  Defines = { "CLEAR_ELEMENT_UINT=0", "CLEAR_ELEMENT_SINT=1" } },
    }

    for _, ElementType in ipairs(ElementTypes) do
        AddClearShader("ClearBufferUAV", ElementType.Name, ElementType.Defines)
    end

    AddClearShader("ClearBufferUAV", "Untyped", { "CLEAR_ELEMENT_UNTYPED=1" })

    -- CLEAR_DIMENSION is the index into this list, in the order of the CLEAR_DIMENSION_* values in ClearTextureUAV.hlsl
    local TextureDimensions = { "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture3D" }
    for DimensionIndex, Dimension in ipairs(TextureDimensions) do
        for _, ElementType in ipairs(ElementTypes) do
            local Defines = { ("CLEAR_DIMENSION=%d"):format(DimensionIndex - 1) }
            for _, Define in ipairs(ElementType.Defines) do
                table.insert(Defines, Define)
            end

            AddClearShader("ClearTextureUAV", Dimension .. "_" .. ElementType.Name, Defines)
        end
    end

    -- One command, a build event only reports the exit code of its last command
    MetalRHI.AddPreBuildCommands({
        table.concat(ShaderCommands, " && "),
    })
end
