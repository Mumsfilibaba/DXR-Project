include "BuildTool.lua"

-- Shared D3D12 Agility SDK location and availability.
-- The SDK is optional: without it the D3D12RHI compiles against the d3d12.h that ships with the
-- Windows SDK, see D3D12_HAS_AGILITY_SDK in Runtime/D3D12RHI/D3D12Configuration.h

-- Folder that the package folders are expected to sit in
local gSearchRootPath = CreateExternalThirdpartyPath("D3D12AgilitySDK")

-- Folder relative to the executable that the redistributables are copied to
local gRedistFolderName = "D3D12"

-- Redistributables copied next to the executable. Only D3D12Core.dll is required.
local gRedistFileNames = {
    "D3D12Core.dll",
    "D3D12Core.pdb",
    "d3d12SDKLayers.dll",
    "d3d12SDKLayers.pdb",
    "d3dconfig.exe",
    "d3dconfig.pdb",
}

-- Splits 'microsoft.direct3d.d3d12.1.619.4' into its comparable parts. Prerelease packages are
-- suffixed, as in 'microsoft.direct3d.d3d12.1.721.2-preview'.
local function ParsePackageFolderName(FolderName)
    local VersionString = FolderName:lower():match("^microsoft%.direct3d%.d3d12%.(%d+%.%d+%.%d+.*)$")
    if not VersionString then
        return nil
    end

    local Major, Minor, Patch = VersionString:match("^(%d+)%.(%d+)%.(%d+)")
    return {
        VersionString = VersionString,
        Major         = tonumber(Major),
        Minor         = tonumber(Minor),
        Patch         = tonumber(Patch),
        bIsPrerelease = VersionString:find("%-") ~= nil,
    }
end

-- Orders packages best-first
local function IsPackagePreferredOver(Lhs, Rhs)
    -- A stable release always wins, so an installed preview never silently replaces it
    if Lhs.bIsPrerelease ~= Rhs.bIsPrerelease then
        return Rhs.bIsPrerelease
    end

    if Lhs.Major ~= Rhs.Major then
        return Lhs.Major > Rhs.Major
    end

    if Lhs.Minor ~= Rhs.Minor then
        return Lhs.Minor > Rhs.Minor
    end

    if Lhs.Patch ~= Rhs.Patch then
        return Lhs.Patch > Rhs.Patch
    end

    return Lhs.VersionString > Rhs.VersionString
end

-- The package minor version tracks the D3D12SDKVersion, but the header is authoritative: its value
-- is what D3D12Core.dll validates the D3D12SDKVersion exported by the executable against.
local function ReadSdkVersionFromHeader(HeaderPath)
    local File = io.open(HeaderPath, "r")
    if not File then
        return nil
    end

    local Contents = File:read("*a")
    File:close()

    -- Matches '#define<tab>D3D12_SDK_VERSION<tab>( 619 )'. Requiring whitespace ahead of the name
    -- keeps D3D12_PREVIEW_SDK_VERSION, declared in the same header, from matching instead.
    return tonumber(Contents:match("#define%s+D3D12_SDK_VERSION%s*%(%s*(%d+)%s*%)"))
end

local function FindAgilitySDK()
    if not os.isdir(gSearchRootPath) then
        return nil
    end

    -- os.matchdirs only understands forward slashes
    local Candidates = {}
    for _, Directory in ipairs(os.matchdirs(path.translate(gSearchRootPath, '/') .. "/microsoft.direct3d.d3d12.*")) do
        local Candidate = ParsePackageFolderName(path.getname(Directory))
        if Candidate then
            Candidate.NativePath = JoinPath(CreateOsPath(Directory), "build/native")
            table.insert(Candidates, Candidate)
        end
    end

    table.sort(Candidates, IsPackagePreferredOver)

    for _, Candidate in ipairs(Candidates) do
        local IncludePath = JoinPath(Candidate.NativePath, "include")
        local BinaryPath  = JoinPath(Candidate.NativePath, "bin/x64")
        local HeaderPath  = JoinPath(IncludePath, "d3d12.h")

        if os.isfile(HeaderPath) and os.isfile(JoinPath(BinaryPath, "D3D12Core.dll")) then
            local SdkVersion = ReadSdkVersionFromHeader(HeaderPath)
            if not SdkVersion then
                SdkVersion = Candidate.Minor
                LogHighlightWarning("[D3D12RHI] No D3D12_SDK_VERSION in '%s', falling back to the package minor version (%d).", HeaderPath, SdkVersion)
            end

            Candidate.IncludePath = IncludePath
            Candidate.BinaryPath  = BinaryPath
            Candidate.SdkVersion  = SdkVersion
            return Candidate
        end

        LogHighlightWarning("[D3D12RHI] Agility SDK package '%s' is incomplete, skipping it.", Candidate.NativePath)
    end

    return nil
end

local gAgilitySDK = FindAgilitySDK()
if gAgilitySDK then
    LogHighlight("[D3D12RHI] Using D3D12 Agility SDK %s (D3D12SDKVersion %d) from '%s'.", gAgilitySDK.VersionString, gAgilitySDK.SdkVersion, gAgilitySDK.NativePath)
else
    LogWarning("[D3D12RHI] Agility SDK not found under '%s'. Building against the Windows SDK headers and the OS D3D12 runtime.", gSearchRootPath)
end

function HasD3D12AgilitySDK()
    return gAgilitySDK ~= nil
end

function GetD3D12AgilitySDKIncludePath()
    return gAgilitySDK and gAgilitySDK.IncludePath or nil
end

function GetD3D12AgilitySDKBinaryPath()
    return gAgilitySDK and gAgilitySDK.BinaryPath or nil
end

-- The D3D12SDKVersion that D3D12Core.dll from the detected package reports
function GetD3D12AgilitySDKVersion()
    return gAgilitySDK and gAgilitySDK.SdkVersion or nil
end

-- The nuget package version of the detected package, for logging
function GetD3D12AgilitySDKPackageVersion()
    return gAgilitySDK and gAgilitySDK.VersionString or nil
end

function GetD3D12AgilitySDKRedistFolderName()
    return gRedistFolderName
end

function GetD3D12AgilitySDKRedistFiles()
    local RedistFiles = {}
    if not gAgilitySDK then
        return RedistFiles
    end

    for _, FileName in ipairs(gRedistFileNames) do
        local FilePath = JoinPath(gAgilitySDK.BinaryPath, FileName)
        if os.isfile(FilePath) then
            table.insert(RedistFiles, FilePath)
        end
    end

    return RedistFiles
end

-- Defines for the exports in Runtime/Launch/Windows/WindowsMain.cpp. Exporting D3D12SDKVersion and
-- D3D12SDKPath without shipping D3D12Core.dll makes D3D12CreateDevice fail with
-- D3D12_ERROR_INVALID_REDIST, so the exports are turned off when the SDK is absent.
function GetD3D12AgilitySDKDefines()
    if not gAgilitySDK then
        return { "D3D12_AGILITY_SDK_EXPORTS=(0)" }
    end

    return {
        "D3D12_AGILITY_SDK_EXPORTS=(1)",
        ("D3D12_AGILITY_SDK_VERSION=(%d)"):format(gAgilitySDK.SdkVersion),
        ("D3D12_AGILITY_SDK_PATH=\".\\\\%s\\\\\""):format(gRedistFolderName)
    }
end
