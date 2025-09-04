-- Helper to safely format without breaking on stray '%' or Premake tokens
local function FormatLog(FmtStr, ...)
    if select("#", ...) > 0 and type(FmtStr) == "string" then
        return string.format(FmtStr, ...)
    end
    
    return tostring(FmtStr)
end

local function WriteLine(Stream, Text)
    if Stream and Stream.write then
        Stream:write(Text .. "\n")
    else
        print(Text)
    end
end

-- Color logger that writes to a specific stream (stdout/stderr)
local function LogWithColorToStream(Stream, Color, Prefix, FmtStr, ...)
    local Message  = FormatLog(FmtStr, ...)
    local UseColor = (term and term.pushColor and term.popColor and Color)

    if UseColor then
        term.pushColor(Color)
    end
    
    local Line = Prefix and (Prefix .. Message) or Message
    WriteLine(Stream or io.stdout, Line)
    
    if UseColor then
        term.popColor()
    end
end

function LogInfo(FmtStr, ...)
    WriteLine(io.stdout, FormatLog(FmtStr, ...))
end

function LogHighlight(FmtStr, ...)
    LogWithColorToStream(io.stdout, term and term.green, nil, FmtStr, ...)
end

function LogHighlightWarning(FmtStr, ...)
    LogWithColorToStream(io.stdout, term and term.yellow, nil, FmtStr, ...)
end

function LogWarning(FmtStr, ...)
    LogWithColorToStream(io.stderr, term and term.yellow, "Warning: ", FmtStr, ...)
end

function LogError(FmtStr, ...)
    LogWithColorToStream(io.stderr, term and term.red, "Error: ", FmtStr, ...)
end
