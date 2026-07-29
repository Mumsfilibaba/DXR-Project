include "BuildTool.lua"

-- Shared D3D12 Agility SDK location and availability.
-- The SDK is optional: without it the D3D12RHI compiles against the d3d12.h that ships with the
-- Windows SDK, see D3D12_HAS_AGILITY_SDK in Runtime/D3D12RHI/D3D12Configuration.h

-- Version of the Agility SDK nuget package in the ThirdParty folder
local gPackageVersion = "1.619.3"

-- The D3D12SDKVersion that D3D12Core.dll from that package reports
local gSdkVersion = 619

-- Folder relative to the executable that the redistributables are copied to
local gRedistFolderName = "D3D12"

local gNativePath  = CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12." .. gPackageVersion .. "/build/native")
local gIncludePath = JoinPath(gNativePath, "include")
local gBinaryPath  = JoinPath(gNativePath, "bin/x64")

local gHasAgilitySDK = os.isdir(gIncludePath) and os.isfile(JoinPath(gBinaryPath, "D3D12Core.dll"))
if not gHasAgilitySDK then
    LogWarning("[D3D12RHI] Agility SDK not found at '%s'. Building against the Windows SDK headers and the OS D3D12 runtime.", gIncludePath)
end

function HasD3D12AgilitySDK()
    return gHasAgilitySDK
end

function GetD3D12AgilitySDKIncludePath()
    return gIncludePath
end

function GetD3D12AgilitySDKBinaryPath()
    return gBinaryPath
end

function GetD3D12AgilitySDKRedistFolderName()
    return gRedistFolderName
end

-- Defines for the exports in Runtime/Launch/Windows/WindowsMain.cpp. Exporting D3D12SDKVersion and
-- D3D12SDKPath without shipping D3D12Core.dll makes D3D12CreateDevice fail with
-- D3D12_ERROR_INVALID_REDIST, so the exports are turned off when the SDK is absent.
function GetD3D12AgilitySDKDefines()
    if not gHasAgilitySDK then
        return { "D3D12_AGILITY_SDK_EXPORTS=(0)" }
    end

    return {
        "D3D12_AGILITY_SDK_EXPORTS=(1)",
        ("D3D12_AGILITY_SDK_VERSION=(%d)"):format(gSdkVersion),
        "D3D12_AGILITY_SDK_PATH=\".\\\\D3D12\\\\\""
    }
end
