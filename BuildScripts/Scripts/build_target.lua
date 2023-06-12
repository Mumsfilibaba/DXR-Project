include "Build_Module.lua"

-- Target types
ETargetType = 
{
    ["Client"]      = 1,
    ["WindowedApp"] = 2,
    ["ConsoleApp"]  = 3,
}

GTargetName = {}

function GetCurrentTargetName()
    if GTargetName == nil then
        printf("ERROR: GTargetName is nil")
        return nil
    end

    return GTargetName
end

-- Target build rules
function FTargetBuildRules(InName)
    printf("Creating Target \'%s\'", InName)
    
    -- Init parent class
    local self = FBuildRules(InName)
    if self == nil then
        printf("ERROR: Failed to create BuildRule")
        return nil
    end

    -- Ensure that target does not already exist
    if IsTarget(InName) then
        printf("ERROR: Target is already created")
        return GetTarget(InName)
    end
       
    -- Setup global target name 
    GTargetName = InName 

    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- @brief - The type of target, decides if there should be a Standalone and DLL or if the application should be a ConsoleApp
    self.TargetType = ETargetType.Client
    
    -- @brief - Weather or not the build should be forced monolithic
    self.bIsMonolithic = IsMonolithic()

    -- @brief - Helper function for retrieving path
    self.GetPath = nil
    function self.GetPath()
        return GetEnginePath() .. "/" ..  self.Name
    end

    -- Generate target
    local BaseGenerate = self.Generate
    function self.Generate()
        printf("    Generating Target \'%s\'\n", self.Name)
  
        -- Setup monolithic builds
        if self.bIsMonolithic then
            printf("    Build is monolithic\n")
            SetIsMonlithic(true)
        else
            printf("    Build is NOT monolithic\n")
            SetIsMonlithic(false)
        end
        
        -- Generate the project based on type
        if self.TargetType == ETargetType.Client then
            printf("    TargetType=Client\n")

            -- Always add module name as a define
            self.AddDefines(
            {
                ("MODULE_NAME=" .. "\"" .. self.Name .. "\"")
            })

            local UpperCaseName = self.Name:upper()
            local ModuleApiName = UpperCaseName .. "_API"

            -- TODO: Should this be created as a module instead? 
            -- In a monolithic build then the client should be linked statically 
            if self.bIsMonolithic then                
                self.Kind            = "WindowedApp"
                self.bRuntimeLinking = false
                self.bIsDynamic      = false

                -- Defines
                self.AddDefines(
                { 
                    ("PROJECT_NAME=" .. "\"" .. self.Name .. "\""),
                    ("PROJECT_LOCATION=" .. "\"" .. FindWorkspaceDir() .. "/" .. self.Name .. "\""),
                    ModuleApiName,
                })

                -- Generate the project
                printf("    ---Generating target project")
                BaseGenerate()
                printf("    ---Finished generating target project")
            else
                self.Kind            = "SharedLib"
                self.bRuntimeLinking = true
                self.bIsDynamic      = true

                self.AddDefines(
                {
                    ModuleApiName .. "=MODULE_EXPORT"
                })
   
                -- Generate the project
                printf("    ---Generating target project")
                BaseGenerate()
                printf("    ---Finished generating target project")
                
                -- Standalone-executable
                printf("    ---Generating Standablone client executable\n")
                
                local Executeble = FBuildRules(self.Name .. "Standalone")
                Executeble.Kind = "WindowedApp"

                Executeble.AddLinkLibraries(
                {
                    self.Name
                })

                Executeble.AddModuleDependencies(self.ModuleDependencies)

                if IsPlatformMac() then
                    Executeble.AddFrameWorks(
                    {
                        "AppKit",
                    })
                end

                -- Setup Defines
                Executeble.AddDefines(
                { 
                    ModuleApiName,
                })
    
                -- Overwrite all exclude-files
                Executeble.ExcludeFiles = {}
        
                -- System includes can be included in a dependency header and therefore necessary in this module aswell
                Executeble.AddSystemIncludes(self.SystemIncludes)

                -- Generate Standalone executable
                Executeble.Generate()

                printf("    ---Finished generating client executable\n")
            end
        elseif self.TargetType == ETargetType.WindowedApp then
            printf("    TargetType=WindowedApp\n")
        elseif self.TargetType == ETargetType.ConsoleApp then
            printf("    TargetType=ConsoleApp\n")
        end
    end

    return self
end