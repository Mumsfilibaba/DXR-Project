-- Helper function to handle colored logging with optional prefixes
local function LogWithColor(Color, Prefix, FormatStr, ...)
    term.pushColor(Color)
    if Prefix then
        print(Prefix .. string.format(FormatStr, ...))
    else
        print(string.format(FormatStr, ...))
    end
    term.popColor()
end

function LogInfo(FormatStr, ...)
    print(string.format(FormatStr, ...))
end

function LogHighlight(FormatStr, ...)
    LogWithColor(term.green, nil, FormatStr, ...)
end

function LogHighlightWarning(FormatStr, ...)
    LogWithColor(term.yellow, nil, FormatStr, ...)
end

function LogWarning(FormatStr, ...)
    LogWithColor(term.yellow, "Warning: ", FormatStr, ...)
end

function LogError(FormatStr, ...)
    LogWithColor(term.red, "Error: ", FormatStr, ...)
end
