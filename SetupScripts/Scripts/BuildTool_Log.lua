-- Helper function to handle colored logging with optional prefixes
local function LogWithColor(Color, Prefix, FmtStr, ...)
    term.pushColor(Color)
    
    local Message
    if select("#", ...) > 0 then
        Message = string.format(FmtStr, ...)
    else
        Message = tostring(FmtStr)  -- don't run format; safe for %{cfg.*}
    end

    if Prefix then
        print(Prefix .. Message)
    else
        print(Message)
    end

    term.popColor()
end

function LogInfo(FmtStr, ...)
    print(string.format(FmtStr, ...))
end

function LogHighlight(FmtStr, ...)
    LogWithColor(term.green, nil, FmtStr, ...)
end

function LogHighlightWarning(FmtStr, ...)
    LogWithColor(term.yellow, nil, FmtStr, ...)
end

function LogWarning(FmtStr, ...)
    LogWithColor(term.yellow, "Warning: ", FmtStr, ...)
end

function LogError(FmtStr, ...)
    LogWithColor(term.red, "Error: ", FmtStr, ...)
end
